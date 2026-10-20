#!/usr/bin/env python3
"""Pack authored sword sheets; trim only transparent padding and align blade axes.
No alternate weapon artwork is synthesized here. Original PNGs remain intact.
"""
from pathlib import Path
from collections import deque
import hashlib,json
from PIL import Image

LEVELS=(0,3,6,10,20)

def source_regions(sheet):
    # Locate full, isolated weapon components instead of cutting a tall guard at
    # an approximate grid boundary. Only extraction rectangles are produced.
    width,height=sheet.size
    pixels=bytearray(sheet.getchannel('A').point(lambda v:1 if v>=100 else 0).tobytes())
    found=[]
    for index in range(width*height):
        if not pixels[index]:continue
        pixels[index]=0;queue=deque([index]);x0=x1=index%width;y0=y1=index//width;count=0
        while queue:
            p=queue.popleft();x=p%width;y=p//width;count+=1
            x0=min(x0,x);x1=max(x1,x);y0=min(y0,y);y1=max(y1,y)
            for n in (p-1 if x else -1,p+1 if x+1<width else -1,p-width if y else -1,p+width if y+1<height else -1):
                if n>=0 and pixels[n]:pixels[n]=0;queue.append(n)
        if x1-x0>width*.2 and count>1000:found.append([x0,y0,x1+1,y1+1])
    left=sorted((b for b in found if (b[0]+b[2])/2<width/2),key=lambda b:b[1])
    right=sorted((b for b in found if (b[0]+b[2])/2>=width/2),key=lambda b:b[1])
    assert len(left)==len(right)==5,('Expected exactly ten complete weapon sprites',found)
    return [b for pair in zip(left,right) for b in pair]

def build_swords(root,out,packer,write_changed):
    frames=[];resample=getattr(Image,'Resampling',Image).LANCZOS
    sizes=[(384,230)]+[(320,192)]*4
    cache_path=out/'sword-source-regions.json';cache=json.loads(cache_path.read_text()) if cache_path.exists() else {}
    for tier,level in enumerate(LEVELS):
        path=root/'assets_src'/('swords.png' if level==0 else f'swords-plus{level}.png')
        with Image.open(path) as opened:sheet=opened.convert('RGBA')
        assert sheet.getchannel('A').getextrema()[0]==0,f'Sword sheet must have real transparency: {path}'
        digest=hashlib.sha256(path.read_bytes()).hexdigest()
        if cache.get(path.name,{}).get('sha256')!=digest:cache[path.name]={'sha256':digest,'regions':source_regions(sheet)}
        width,height=sizes[tier]
        for family in range(10):
            box=cache[path.name]['regions'][family]
            raw=sheet.crop(tuple(box));mask=raw.getchannel('A').point(lambda v:255 if v>=100 else 0)
            section=mask.crop((round(raw.width*.62),0,round(raw.width*.88),raw.height))
            rows=[sum(section.crop((0,y,section.width,y+1)).getdata()) for y in range(raw.height)]
            total=sum(rows);assert total>0,(tier,family,'missing blade')
            accumulation=0;source_axis=0
            for source_axis,amount in enumerate(rows):
                accumulation+=amount
                if accumulation>=total/2:break
            desired_axis=round(height*150/230)
            scale=min(width*.94/raw.width,desired_axis/max(source_axis,1),(height-desired_axis)/max(raw.height-source_axis,1))
            image=raw.resize((round(raw.width*scale),round(raw.height*scale)),resample)
            mask=image.getchannel('A').point(lambda v:255 if v>=100 else 0)
            bounds=mask.getbbox();assert bounds is not None,(tier,family)
            image.putalpha(mask)
            # Median occupied blade row excludes the handle/guard. The offset is
            # metadata, rather than baking a translated/cut-off raster canvas.
            section=mask.crop((round(image.width*.62),0,round(image.width*.88),image.height))
            rows=[sum(section.crop((0,y,section.width,y+1)).getdata()) for y in range(image.height)]
            total=sum(rows);assert total>0,(tier,family,'missing blade')
            accumulation=0;axis=0
            for axis,amount in enumerate(rows):
                accumulation+=amount
                if accumulation>=total/2:break
            shift=desired_axis-axis
            cropped=image.crop(bounds)
            frames.append({'name':f'sword_{family}_t{tier}','tier':tier,'family':family,
                           'image':cropped,'left':round(width*.03)+bounds[0],'top':bounds[1]+shift,
                           'width':cropped.width,'height':cropped.height,'source_axis':axis})
    # First-fit decreasing-height shelves keep the 50 forms within the existing
    # application partition and PSRAM budget, without storing empty cell padding.
    def layout(width):
        rows=[];positions={}
        for frame in sorted(frames,key=lambda f:f['height'],reverse=True):
            available=[r for r in rows if r['height']>=frame['height'] and r['used']+frame['width']<=width]
            row=max(available,key=lambda r:r['used']) if available else None
            if row is None:
                row={'y':sum(r['height'] for r in rows),'height':frame['height'],'used':0};rows.append(row)
            positions[frame['name']]=(row['used'],row['y']);row['used']+=frame['width']
        return width,sum(r['height'] for r in rows),positions
    width,height,positions=min((layout(w) for w in range(736,1537,32)),key=lambda shape:shape[0]*shape[1])
    for frame in frames:frame['x'],frame['y']=positions[frame['name']]
    atlas=Image.new('RGBA',(width,height))
    records=[]
    for frame in frames:
        atlas.alpha_composite(frame['image'],(frame['x'],frame['y']))
        records.append(packer.ATLAS_FRAME.pack(packer.asset_id(frame['name']),frame['x'],frame['y'],
            frame['width'],frame['height'],-frame['left'],-frame['top']))
    rgb,alpha=packer._rgb565_a8(atlas)
    data=packer.ATLAS_HEADER.pack(b'MSA1',width,height,len(frames),1,len(rgb),len(alpha))+b''.join(records)+rgb+alpha
    assert len(data)<6_300_000,f'Sword atlas exceeds reserved flash/RAM budget: {len(data)}'
    write_changed(out/'swords.atlas',data)
    write_changed(cache_path,(json.dumps(cache,indent=2)+'\n').encode())
    header='// Generated sword form asset geometry; same metadata on Host/device.\n#pragma once\n#include <stdint.h>\n'
    header+=f'#define FORGE_SWORD_TIER_COUNT {len(LEVELS)}U\n'
    header+='static const uint8_t forge_sword_levels[]={'+','.join(map(str,LEVELS))+'};\n'
    header+='static const uint16_t forge_sword_canvas_w[]={'+','.join(str(s[0]) for s in sizes)+'};\n'
    header+='static const uint16_t forge_sword_canvas_h[]={'+','.join(str(s[1]) for s in sizes)+'};\n'
    write_changed(out/'forge_sword_assets.h',header.encode())
    report={'forms':len(frames),'levels':LEVELS,'canvas_sizes':sizes,'atlas_size':[width,height],'atlas_bytes':len(data),
            'frames':[{k:v for k,v in frame.items() if k!='image'} for frame in frames]}
    write_changed(out/'sword-forms.json',(json.dumps(report,indent=2)+'\n').encode())
    return report

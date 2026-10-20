#!/usr/bin/env python3
"""Compile authored PNGs, a declared Chinese glyph subset and original SFX.

This is the game's build-time asset pipeline. The device consumes MSA1/MSN1;
it never opens PNG, WAV, fonts, or Python. Host uses these exact same files.
"""
from pathlib import Path
import importlib.util
import json
import math
import random
import struct
import wave
from PIL import Image, ImageDraw, ImageFont
from prepare_swords import build_swords

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/generated'
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_asset_packer',ENGINE/'tools/pack_game_assets.py')
packer=importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)
OUT.mkdir(parents=True,exist_ok=True)

def write_changed(path,data):
    if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)

def atlas(name,image,frames,binary=False,opaque=False):
    im=image.convert('RGBA')
    if binary:im.putalpha(im.getchannel('A').point(lambda x:255 if x>=100 else 0))
    if opaque:im.putalpha(255)
    rgb,alpha=packer._rgb565_a8(im)
    flags=1 if binary else 0
    if opaque:flags=3;alpha=b''
    records=b''.join(packer.ATLAS_FRAME.pack(packer.asset_id(n),x,y,w,h,0,0) for n,x,y,w,h in frames)
    data=packer.ATLAS_HEADER.pack(b'MSA1',im.width,im.height,len(frames),flags,len(rgb),len(alpha))+records+rgb+alpha
    write_changed(OUT/name,data)

resample=getattr(Image,'Resampling',Image).LANCZOS
room=Image.open(ROOT/'assets_src/forge-room.png').convert('RGB').resize((480,480),resample)
atlas('room.atlas',room,[('room',0,0,480,480)],opaque=True)
hammer=Image.open(ROOT/'assets_src/hammer.png').convert('RGBA').resize((224,224),resample)
atlas('hammer.atlas',hammer,[('hammer',0,0,224,224)],binary=True)
# One bounded, smooth-alpha sprite; the tribute renderer animates it directly.
dragon=Image.open(ROOT/'assets_src/tribute-dragon.png').convert('RGBA').resize((416,178),resample)
atlas('dragon.atlas',dragon,[('dragon',0,0,416,178)])

# Regular sprite-sheet cells are authored separately by ImageGen. Packing and
# quantization here do not synthesize alternate artwork or render game scenes.
sword_forms=build_swords(ROOT,OUT,packer,write_changed)
source=Image.open(ROOT/'assets_src/relics.png').convert('RGBA')
items=Image.new('RGBA',(256,256));frames=[]
for i in range(16):
    box=(i%4*source.width//4,i//4*source.height//4,(i%4+1)*source.width//4,(i//4+1)*source.height//4)
    cell=source.crop(box).resize((64,64),resample);x=i%4*64;y=i//4*64;items.alpha_composite(cell,(x,y));frames.append((f'relic_{i}',x,y,64,64))
atlas('relics.atlas',items,frames,binary=True)

rules=json.loads((ROOT/'assets_src/rules.json').read_text())
all_text=json.dumps(rules,ensure_ascii=False)
for path in sorted((ROOT/'main').glob('*.[ch]')):all_text+=path.read_text()
chars=sorted(set(chr(i) for i in range(32,127))|{c for c in all_text if ord(c)>=128 and not c.isspace()}|set('□金币剑币今日贡额炉火强化入神进贡锻造遗物剑架返回暂停继续重开确认取消加热选择碎刀成功大成功损伤不变日万亿京'))
# Bake each UI size directly from outlines, retaining grayscale coverage and
# real glyph metrics. No runtime nearest-neighbor resizing or hard threshold.
font_root=Path('/usr/share/fonts/opentype/noto')
font_sizes=(14,16,17,18,19,20,21,22,23,24,25,26,28,30,34)
codepoints=','.join(str(ord(c)) for c in chars)
mask=bytearray();glyphs=[]
for size in font_sizes:
    font_path=font_root/('NotoSansCJK-Regular.ttc' if size<=22 else 'NotoSansCJK-Bold.ttc')
    if not font_path.exists():raise SystemExit(f'Required Simplified Chinese font is missing: {font_path}')
    font=ImageFont.truetype(str(font_path),size,index=2)
    baseline=round(size*.88)
    for c in chars:
        image=Image.new('L',(size+12,size+12))
        ImageDraw.Draw(image).text((6,baseline+6),c,font=font,fill=255,anchor='ls')
        box=image.getbbox();advance=max(1,round(font.getlength(c)))
        if box:
            cropped=image.crop(box);width,height=cropped.size
            glyphs.append((len(mask),box[0]-6,box[1]-6,width,height,advance))
            mask.extend(cropped.tobytes())
        else:glyphs.append((len(mask),0,0,0,0,advance))
header='// Generated native-size Simplified Chinese A8 glyphs.\n#pragma once\n#include <stdint.h>\n'
header+=f'#define FORGE_FONT_COUNT {len(chars)}U\n#define FORGE_FONT_SIZE_COUNT {len(font_sizes)}U\n#define FORGE_FONT_BYTES {len(mask)}U\n'
header+='typedef struct {uint32_t offset; int8_t x,y; uint8_t width,height,advance;} forge_font_glyph_t;\n'
header+='static const uint8_t forge_font_sizes[FORGE_FONT_SIZE_COUNT]={'+','.join(map(str,font_sizes))+'};\n'
header+='static const uint32_t forge_font_codepoints[FORGE_FONT_COUNT]={'+codepoints+'};\n'
header+='static const forge_font_glyph_t forge_font_glyphs[FORGE_FONT_COUNT*FORGE_FONT_SIZE_COUNT]={\n'
header+=''.join(' {'+','.join(map(str,glyph))+'},\n' for glyph in glyphs)+'};\n'
write_changed(OUT/'font.mask',bytes(mask))
write_changed(OUT/'forge_font.h',header.encode())
(OUT/'font.atlas').unlink(missing_ok=True)

def sound(name,duration,tones,noise=0.0):
    rate=24000;count=int(rate*duration);rng=random.Random(name);samples=[]
    for i in range(count):
        t=i/rate
        v=sum(amp*math.sin(2*math.pi*frequency*t)*math.exp(-decay*t) for frequency,amp,decay in tones)
        v+=noise*rng.uniform(-1,1)*math.exp(-18*t)
        samples.append(max(-32760,min(32760,int(v*14000))))
    p=ROOT/'assets_src'/f'{name}.wav'
    with wave.open(str(p),'wb') as wav:wav.setparams((1,2,rate,count,'NONE','not compressed'));wav.writeframes(struct.pack(f'<{count}h',*samples))
    packer.write_sound(p,OUT/f'{name}.sound')

sound('hit',.28,[(733,.6,22),(1717,.4,14),(2831,.22,28)],.30)
sound('great',.55,[(880,.55,9),(1320,.35,8),(2640,.22,11)],.15)
sound('damage',.25,[(143,.65,16),(401,.25,24),(1601,.18,31)],.45)
sound('break',.50,[(201,.6,12),(955,.2,20),(2971,.15,23)],.8)
sound('bell',.85,[(440,.6,4),(1056,.3,5),(1879,.2,7)])
sound('select',.09,[(660,.25,30),(990,.15,40)])
sound('heat',.50,[(83,.15,7),(127,.12,7)],.55)
sound('frenzy',.65,[(330,.35,6),(660,.4,5),(1320,.25,6)])
report={'format':'bailian-assets-v1','images':'Authored via built-in ImageGen; source PNGs retained.','sword_forms':sword_forms['forms'],'sword_levels':sword_forms['levels'],'glyphs':len(chars),'font':'Noto Sans CJK SC Regular/Bold, 15 native sizes, grayscale A8, SIL Open Font License','audio':'Original deterministic synthesized metallic/forge SFX.','files':{p.name:p.stat().st_size for p in OUT.iterdir() if p.suffix in ('.atlas','.sound','.mask')}}
write_changed(OUT/'asset-manifest.json',(json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode())
print(f'Prepared {len(chars)} glyphs and {sum(report["files"].values())} bytes of shared Host/device assets.')

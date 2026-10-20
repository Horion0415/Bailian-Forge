// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "forge_sword_assets.h"
#include "mosaico_raylib_fast.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clamp01(float x){return x<0?0:x>1?1:x;}
static float ease(float x){x=clamp01(x);return x*x*(3-2*x);}
unsigned forge_sword_stage(unsigned level)
{
    unsigned stage=0;
    for(unsigned i=1;i<FORGE_SWORD_TIER_COUNT;i++)if(level>=forge_sword_levels[i])stage=i;
    return stage;
}
const char *forge_sword_stage_name(unsigned level)
{
    static const char *const names[]={"素胚","初锻","精锻","名匠","传说"};
    return names[forge_sword_stage(level)];
}
void forge_sword_reset(game_handle_t g){memset(&g->shape,0,sizeof(g->shape));}
void forge_sword_update(game_handle_t g,uint32_t ms)
{
    forge_sword_shape_t *s=&g->shape;const forge_blade_t *b=forge_active(g);
    if(!b){s->blade_id=0;s->morphing=false;return;}
    if(s->blade_id==b->id){
        if(s->family!=b->family||forge_sword_stage(s->level)!=forge_sword_stage(b->level)){
            s->from_family=s->family;s->from_level=s->level;s->age_ms=0;s->morphing=true;
        }else if(s->morphing){
            unsigned age=s->age_ms+ms;s->age_ms=(uint16_t)(age>FORGE_SWORD_MORPH_MS?FORGE_SWORD_MORPH_MS:age);
            if(age>=FORGE_SWORD_MORPH_MS)s->morphing=false;
        }
    }else{s->blade_id=b->id;s->morphing=false;s->age_ms=0;}
    s->family=b->family;s->level=b->level;
}
static const MosaicoSpriteFrame *frame(game_handle_t g,unsigned family,unsigned stage)
{
    if(family>=10||stage>=FORGE_SWORD_TIER_COUNT)return NULL;
    char key[24];snprintf(key,sizeof(key),"sword_%u_t%u",family,stage);
    return MosaicoAtlasGetFrame(g->swords,mosaico_game_asset_id(key));
}
static void draw_form(game_handle_t g,unsigned family,unsigned level,Rectangle dest,Vector2 origin,float rotation,Color tint)
{
    unsigned stage=forge_sword_stage(level);const MosaicoSpriteFrame *f=frame(g,family,stage);if(!f||!tint.a)return;
    float width=forge_sword_canvas_w[stage],height=forge_sword_canvas_h[stage];
    unsigned start=forge_sword_levels[stage],end=stage+1<FORGE_SWORD_TIER_COUNT?forge_sword_levels[stage+1]:100;
    float progress=clamp01((float)(level-start)/(end-start));
    float grow_x=1+progress*.025f,grow_y=1+progress*.035f;
    float sx=dest.width/width,sy=dest.height/height;
    /* A negative atlas pivot records the crop offset inside the original canvas.
     * Keep the guard and blade axis anchored while fine shaping grows by level. */
    float left=width*.29f+(-f->pivot.x-width*.29f)*grow_x;
    float top=height*(150.0f/230)+(-f->pivot.y-height*(150.0f/230))*grow_y;
    dest.width=f->source.width*sx*grow_x;dest.height=f->source.height*sy*grow_y;
    origin.x-=left*sx;origin.y-=top*sy;
    DrawTexturePro(g->swords.texture,f->source,dest,origin,rotation,tint);
}
void forge_sword_draw(game_handle_t g,unsigned family,unsigned level,Rectangle dest,Vector2 origin,float rotation,Color tint,bool animate)
{
    const forge_sword_shape_t *s=&g->shape;
    if(!animate||!s->morphing){draw_form(g,family,level,dest,origin,rotation,tint);return;}
    float p=ease((float)s->age_ms/FORGE_SWORD_MORPH_MS);Color old=tint,next=tint;
    old.a=(uint8_t)(old.a*(1-p));next.a=(uint8_t)(next.a*p);
    draw_form(g,s->from_family,s->from_level,dest,origin,rotation,old);
    draw_form(g,family,level,dest,origin,rotation,next);
    float flash=sinf(p*3.141593f);
    int x=(int)(dest.x+dest.width*.61f),y=(int)(dest.y+dest.height*(150.0f/230));
    Mosaico2DDrawAlphaMask(g->fx.glow,FORGE_FX_GLOW_W,FORGE_FX_GLOW_W,FORGE_FX_GLOW_H,
        x-48,y-24,(Color){255,218,134,(uint8_t)(flash*175)});
    float scan=dest.x+dest.width*(.30f+p*.64f),line_y=dest.y+dest.height*(150.0f/230);
    DrawLine((int)scan,(int)line_y-9,(int)scan,(int)line_y+9,(Color){255,237,177,(uint8_t)(flash*230)});
}
void forge_sword_draw_piece(game_handle_t g,unsigned family,unsigned level,Rectangle window,Rectangle dest,Color tint)
{
    unsigned stage=forge_sword_stage(level);const MosaicoSpriteFrame *f=frame(g,family,stage);if(!f)return;
    float sx=forge_sword_canvas_w[stage]/384.0f,sy=forge_sword_canvas_h[stage]/230.0f;
    float left=-f->pivot.x/sx,top=-f->pivot.y/sy,right=left+f->source.width/sx,bottom=top+f->source.height/sy;
    float x=fmaxf(window.x,left),y=fmaxf(window.y,top),r=fminf(window.x+window.width,right),b=fminf(window.y+window.height,bottom);
    if(x>=r||y>=b)return;
    Rectangle source={f->source.x+(x-left)*sx,f->source.y+(y-top)*sy,(r-x)*sx,(b-y)*sy};
    Rectangle target={dest.x+(x-window.x)*dest.width/window.width,dest.y+(y-window.y)*dest.height/window.height,
        (r-x)*dest.width/window.width,(b-y)*dest.height/window.height};
    DrawTexturePro(g->swords.texture,source,target,(Vector2){0,0},0,tint);
}

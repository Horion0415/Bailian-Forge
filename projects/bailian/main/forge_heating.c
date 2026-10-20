// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "mosaico_raylib_fast.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float unit(float p){return p<0?0:p>1?1:p;}
static float ease(float p){p=unit(p);return p*p*(3-2*p);}
static float mix(float a,float b,float p){return a+(b-a)*p;}
static Color fade(Color c,float opacity){c.a=(uint8_t)(c.a*unit(opacity));return c;}
static Color tint(Color a,Color b,float p)
{
    p=unit(p);return(Color){(uint8_t)mix(a.r,b.r,p),(uint8_t)mix(a.g,b.g,p),(uint8_t)mix(a.b,b.b,p),255};
}
void forge_heat_reset(game_handle_t g)
{
    memset(&g->heating,0,sizeof(g->heating));g->heat_ms=0;
}
void forge_heat_start(game_handle_t g)
{
    forge_blade_t *b=forge_active(g);if(!b||b->heated||g->heat_ms)return;
    g->heating.blade_id=b->id;g->heating.ready_ms=0;
    g->heat_ms=FORGE_HEAT_DURATION_MS;
    g->fx.active=false;g->fx.frenzy_pulse_ms=0;
    memset(g->fx.particles,0,sizeof(g->fx.particles));
    g->feedback=FB_HEAT;forge_notice(g,"刀胚缓缓入炉，炉火正在升温");
}
void forge_heat_update(game_handle_t g,uint32_t ms)
{
    g->heating.ready_ms=(uint16_t)(g->heating.ready_ms>ms?g->heating.ready_ms-ms:0);
    if(!g->heat_ms)return;
    forge_blade_t *b=forge_active(g);
    if(!b||b->id!=g->heating.blade_id){g->heat_ms=0;return;}
    g->heat_ms=(uint16_t)(g->heat_ms>ms?g->heat_ms-ms:0);
    if(!g->heat_ms){
        b->heated=true;g->fx.heat=1;g->heating.ready_ms=FORGE_HEAT_READY_MS;
        forge_changed(g,true);g->feedback=FB_SELECT;
        forge_notice(g,"火候正好，点击铁砧开始锻造");
    }
}
forge_heat_phase_t forge_heat_phase(game_handle_t g)
{
    forge_blade_t *b=forge_active(g);if(!b)return HEAT_NONE;
    if(g->heat_ms){unsigned elapsed=FORGE_HEAT_DURATION_MS-g->heat_ms;
        if(elapsed<FORGE_HEAT_INSERT_MS)return HEAT_INSERT;
        if(elapsed<FORGE_HEAT_DURATION_MS-FORGE_HEAT_RETURN_MS)return HEAT_WARM;
        return HEAT_RETURN;
    }
    if(!b->heated)return HEAT_COLD;
    if(g->heating.ready_ms&&g->heating.blade_id==b->id)return HEAT_READY;
    return HEAT_HOT;
}
void forge_heat_read(game_handle_t g,forge_heat_view_t *v)
{
    *v=(forge_heat_view_t){.sword={27,110,384,230},.tint=WHITE,.value_alpha=1};
    v->phase=forge_heat_phase(g);
    forge_blade_t *b=forge_active(g);if(!b)return;
    if(g->heat_ms){
        float elapsed=FORGE_HEAT_DURATION_MS-g->heat_ms;
        float insertion=ease(elapsed/FORGE_HEAT_INSERT_MS);
        float returning=ease((elapsed-(FORGE_HEAT_DURATION_MS-FORGE_HEAT_RETURN_MS))/FORGE_HEAT_RETURN_MS);
        v->progress=elapsed/FORGE_HEAT_DURATION_MS;
        v->travel=insertion*(1-returning);
        v->sword=(Rectangle){mix(27,-78,v->travel),mix(110,72,v->travel)-sinf(v->travel*3.141593f)*12,
            mix(384,242,v->travel),mix(230,145,v->travel)};
        float warmth=ease((elapsed-FORGE_HEAT_INSERT_MS)/(FORGE_HEAT_DURATION_MS-FORGE_HEAT_INSERT_MS-FORGE_HEAT_RETURN_MS));
        v->tint=warmth<.55f?tint(WHITE,(Color){242,117,48,255},warmth/.55f):
            tint((Color){242,117,48,255},(Color){255,180,110,255},(warmth-.55f)/.45f);
        v->hammer_drop=82*v->travel;
        /* Keep the value card out of the furnace until the blade clears it. */
        v->value_alpha=1-ease(v->travel*1.6f);v->hud_alpha=1;
    }else if(b->heated){
        v->progress=1;
        v->tint=(Color){255,(uint8_t)(215-g->fx.heat*35),(uint8_t)(175-g->fx.heat*65),255};
        v->charge_alpha=1;
        if(v->phase==HEAT_READY){
            v->hud_alpha=unit(((float)g->heating.ready_ms-100)/120);
            v->charge_alpha=1-unit((float)g->heating.ready_ms/100);
        }
    }else v->hud_alpha=1;
}
void forge_heat_draw_light(game_handle_t g,const forge_heat_view_t *v,int dx,int dy)
{
    if(!g->heat_ms)return;
    float warmth=unit(v->progress*1.4f),strength=v->travel*warmth;
    float pulse=.9f+.1f*sinf((float)(g->fx.clock_ms%5000)*.02f);
    Color c={255,173,63,(uint8_t)(strength*pulse*145)};
    Mosaico2DDrawAlphaMask(g->fx.glow,FORGE_FX_GLOW_W,FORGE_FX_GLOW_W,FORGE_FX_GLOW_H,
        36+dx,140+dy,c);
}
void forge_heat_draw_hud(game_handle_t g,const forge_heat_view_t *v)
{
    if(v->hud_alpha<=0)return;
    float a=v->hud_alpha;
    const Color gold={245,187,96,255},muted={166,143,109,255};
    DrawRectangleGradientV(154,283,312,51,fade((Color){30,21,14,235},a),fade((Color){16,12,9,240},a));
    DrawLine(154,283,465,283,fade((Color){110,75,39,150},a));
    const char *label=v->phase==HEAT_INSERT?"刀胚入炉":v->phase==HEAT_WARM?"炉火升温":
        v->phase==HEAT_RETURN?"移回铁砧":v->phase==HEAT_READY?"火候正好":"刀胚未热";
    forge_text(g,label,168,289,20,fade(v->phase==HEAT_COLD?muted:gold,a));
    if(v->phase==HEAT_COLD)forge_text(g,"点炉火",400,291,16,fade(muted,a));
    else{
        unsigned percent=(unsigned)lroundf(v->progress*100);
        if(g->heat_ms&&percent>99)percent=99;
        snprintf(g->text_buffer,sizeof(g->text_buffer),"%u%%",percent);
        forge_text(g,g->text_buffer,412,290,18,fade(gold,a));
    }
    for(int i=0;i<3;i++){
        bool done=(int)v->phase>=HEAT_INSERT+i;
        if(i<2)DrawLine(290+i*34,299,316+i*34,299,fade((Color){93,65,35,255},a));
        DrawCircle(286+i*34,299,3,fade(done?gold:(Color){75,56,36,255},a));
    }
    DrawRectangle(168,317,283,5,fade((Color){65,44,28,255},a));
    int fill=(int)lroundf(v->progress*283);
    if(fill){
        DrawRectangleGradientH(168,317,fill,5,fade((Color){183,77,28,255},a),fade(gold,a));
        DrawLine(168,317,167+fill,317,fade((Color){255,218,149,255},a));
        if(g->heat_ms)DrawCircle(167+fill,319,3,fade((Color){255,224,156,255},a));
    }
}
bool forge_heat_draw_footer(game_handle_t g,const forge_heat_view_t *v)
{
    const char *text=NULL;
    if(v->phase==HEAT_COLD)text="点击炉火或铁砧，把刀胚送入炉膛";
    else if(v->phase==HEAT_INSERT)text="刀胚入炉，准备升温";
    else if(v->phase==HEAT_WARM)text="炉火升温，刀刃渐渐烧红";
    else if(v->phase==HEAT_RETURN)text="火候已足，正在移回铁砧";
    else if(v->phase==HEAT_READY)text="火候正好 · 点击铁砧落锤";
    if(!text)return false;
    forge_text(g,text,18,386,18,(Color){231,211,177,255});return true;
}

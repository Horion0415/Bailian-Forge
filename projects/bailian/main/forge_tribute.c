// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "mosaico_raylib_fast.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clamp01(float x){return x<0?0:x>1?1:x;}
static float ease(float x){x=clamp01(x);return x*x*(3-2*x);}
static Color tint(Color c,float a){c.a=(uint8_t)(255*clamp01(a));return c;}
static Color accent(const forge_tribute_fx_t *t){return t->paid?(Color){255,207,104,255}:(Color){255,110,62,255};}
static void glow(game_handle_t g,int x,int y,Color c)
{
    Mosaico2DDrawAlphaMask(g->fx.glow,FORGE_FX_GLOW_W,FORGE_FX_GLOW_W,FORGE_FX_GLOW_H,x-48,y-24,c);
}
void forge_tribute_reset(game_handle_t g){memset(&g->tribute,0,sizeof(g->tribute));}
bool forge_tribute_start(game_handle_t g,bool accept_short)
{
    forge_tribute_fx_t *t=&g->tribute;
    if(t->active||g->state.phase!=PHASE_PLAY||g->state.day>=999)return false;
    forge_tribute_reset(g);
    t->amount=forge_tribute(&g->state);t->need=forge_quota(&g->state);
    t->paid=forge_money_compare(&t->amount,&t->need)>=0;
    if(!t->paid&&!accept_short)return forge_settle(g,false);
    t->from_day=g->state.day;t->anger_before=g->state.anger;
    for(unsigned i=0;i<FORGE_BLADES;i++)if(g->state.blades[i].place==PLACE_TRIBUTE){
        if(t->sample_count<FORGE_TRIBUTE_SAMPLES){unsigned at=t->sample_count++;
            t->families[at]=g->state.blades[i].family;t->levels[at]=g->state.blades[i].level;}
        t->count++;
    }
    forge_money_t previous_coins=g->state.coins;
    unsigned previous_credits=g->state.credits;
    if(!forge_settle(g,accept_short)){forge_tribute_reset(g);return false;}
    t->reward=g->state.coins;forge_money_sub(&t->reward,&previous_coins);
    t->credit_gain=(uint16_t)(g->state.credits-previous_credits);
    t->to_day=g->state.day;t->anger_after=g->state.anger;t->phase_after=g->state.phase;
    t->active=true;
    g->pending_strike=false;g->animation_ms=0;g->heat_ms=0;forge_heat_reset(g);
    return true;
}
unsigned forge_tribute_stage(game_handle_t g)
{
    if(!g->tribute.active)return 0;
    return g->tribute.age_ms<FORGE_TRIBUTE_RESPONSE_MS?1:g->tribute.age_ms<FORGE_TRIBUTE_COINS_MS?2:3;
}
void forge_tribute_update(game_handle_t g,uint32_t ms)
{
    forge_tribute_fx_t *t=&g->tribute;if(!t->active)return;
    unsigned previous=t->age_ms,age=previous+ms;
    t->age_ms=(uint16_t)(age>FORGE_TRIBUTE_DURATION_MS?FORGE_TRIBUTE_DURATION_MS:age);
    if(previous<FORGE_TRIBUTE_RESPONSE_MS&&age>=FORGE_TRIBUTE_RESPONSE_MS&&!t->paid)g->feedback=FB_DAMAGE;
    if(previous<FORGE_TRIBUTE_COINS_MS&&age>=FORGE_TRIBUTE_COINS_MS&&!forge_money_zero(&t->reward))g->feedback=FB_SELECT;
    if(age>=FORGE_TRIBUTE_DURATION_MS)t->active=false;
}
void forge_tribute_pointer(game_handle_t g,int x,int y)
{
    if(g->paused){g->paused=false;return;}
    if(y<52&&x<90){g->paused=true;return;}
    if(g->tribute.age_ms>=FORGE_TRIBUTE_SKIP_MS&&y>=406)g->tribute.active=false;
}
static void dragon(game_handle_t g,float opacity)
{
    forge_tribute_fx_t *t=&g->tribute;Color dark=tint((Color){11,10,9,255},opacity*.97f),edge=tint(accent(t),opacity*.8f);
    float breath=sinf(t->age_ms*.006f)*2;
    /* A bounded shadow silhouette: spread wings, long horns, muzzle and eyes. */
    for(int side=-1;side<=1;side+=2){
        float x=240,sy=139+breath;
        DrawTriangle((Vector2){x+side*20,sy+23},(Vector2){x+side*158,sy-28},(Vector2){x+side*100,sy+54},dark);
        DrawTriangle((Vector2){x+side*37,sy+26},(Vector2){x+side*100,sy+54},(Vector2){x+side*65,sy+12},dark);
        DrawTriangle((Vector2){x+side*21,sy+9},(Vector2){x+side*52,sy-27},(Vector2){x+side*32,sy+35},dark);
        DrawLine((int)x+side*34,(int)sy+2,(int)x+side*52,(int)sy-27,tint(accent(t),opacity*.4f));
        DrawLine((int)x+side*52,(int)sy-27,(int)x+side*44,(int)sy-12,edge);
        DrawLine((int)x+side*56,(int)sy+11,(int)x+side*133,(int)sy-14,tint(accent(t),opacity*.3f));
    }
    DrawTriangle((Vector2){195,141+breath},(Vector2){285,141+breath},(Vector2){240,212+breath},dark);
    DrawTriangle((Vector2){215,137+breath},(Vector2){265,137+breath},(Vector2){240,116+breath},dark);
    float eyes=ease(((float)t->age_ms-FORGE_TRIBUTE_RESPONSE_MS)/220)*opacity;
    for(int side=-1;side<=1;side+=2){
        int x=240+side*23,y=159+(int)breath;
        glow(g,x,y,tint(accent(t),eyes*.8f));
        DrawLineEx((Vector2){x-side*8,y-3},(Vector2){x+side*5,y+1},3,tint(accent(t),eyes));
        DrawPixel(x,y,tint((Color){255,242,189,255},eyes));
    }
    DrawLine(231,184+(int)breath,240,188+(int)breath,edge);
    DrawLine(240,188+(int)breath,249,184+(int)breath,edge);
}
static void altar(game_handle_t g)
{
    forge_tribute_fx_t *t=&g->tribute;float time=t->age_ms*.001f;
    Color c=accent(t);float response=ease(((float)t->age_ms-550)/300);
    DrawEllipse(240,281,137,28,(Color){15,12,10,240});
    DrawEllipseLines(240,281,137,28,(Color){129,91,46,255});
    DrawEllipseLines(240,274,113,24,tint(c,.4f+response*.4f));
    DrawEllipseLines(240,274,100,20,tint(c,.35f));
    glow(g,210,269,tint(c,.45f));glow(g,267,269,tint(c,.45f));
    for(int i=0;i<20;i++){
        float angle=(float)i*6.2831853f/20+time*.45f;
        DrawLine((int)(240+cosf(angle)*105),(int)(274+sinf(angle)*21),
            (int)(240+cosf(angle)*113),(int)(274+sinf(angle)*24),tint(c,.65f));
    }
    for(int i=0;i<20;i++){
        float p=fmodf(time*.8f+i*.05f,1);
        int x=161+i*8+(int)(sinf(p*9+i)*8),y=278-(int)(p*(54+response*42));
        DrawLine(x,y,x+1,y+4,tint(c,sinf(p*3.14159f)*.75f));
    }
    if(t->age_ms>=650&&t->age_ms<1250){
        float p=(float)(t->age_ms-650)/600;
        DrawEllipseLines(240,274,35+ease(p)*169,7+ease(p)*36,tint(c,(1-p)*.85f));
    }
}
static void offerings(game_handle_t g)
{
    forge_tribute_fx_t *t=&g->tribute;
    if(t->age_ms>=1100)return;
    for(unsigned i=0;i<t->sample_count;i++){
        float delay=i*35,p=ease(((float)t->age_ms-delay)/850);
        float start=76+i*328.0f/(t->sample_count>1?t->sample_count-1:1);
        if(t->sample_count==1)start=240;
        float x=start+(240-start)*p,y=252-(i%2)*12-52*sinf(p*1.5707963f);
        float size=116*(1-p*.85f),opacity=1-ease((p-.68f)/.32f);
        glow(g,(int)x,(int)y+3,tint(accent(t),opacity*.65f));
        forge_sword_draw(g,t->families[i],t->levels[i],(Rectangle){x,y,size,size*.6f},
            (Vector2){size*.5f,size*.3f},(i%2?1:-1)*p*32,tint((Color){255,226,164,255},opacity),false);
    }
}
static void coins(game_handle_t g,float progress)
{
    forge_tribute_fx_t *t=&g->tribute;if(forge_money_zero(&t->reward))return;
    for(int i=0;i<12;i++){
        float p=clamp01((progress-i*.035f)/.58f);if(p<=0||p>=1)continue;
        float x=237+(344-237)*p+sinf(p*3.14159f)*(i-5.5f)*6;
        float y=272+(357-272)*p-sinf(p*3.14159f)*(88+i%3*12);
        int radius=3+(i%2);Color c={255,205,90,255};
        DrawLine((int)x-3,(int)y+6,(int)x,(int)y,tint(c,.45f));
        DrawCircle((int)x,(int)y,radius,(Color){123,73,23,255});
        DrawCircleLines((int)x,(int)y,radius,c);DrawLine((int)x,(int)y-2,(int)x,(int)y+2,(Color){255,240,166,255});
    }
}
void forge_tribute_draw(game_handle_t g)
{
    forge_tribute_fx_t *t=&g->tribute;float age=t->age_ms;
    Color c=accent(t);unsigned stage=forge_tribute_stage(g);
    DrawTexture(g->room.texture,0,0,WHITE);
    DrawRectangle(0,0,480,480,(Color){10,8,7,207});
    DrawRectangleLines(18,18,444,444,tint(c,.58f));
    DrawLine(32,76,448,76,tint(c,.35f));
    snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"第%u日 · 敲铃进贡"),t->from_day);
    forge_text_fit(g,g->text_buffer,35,30,301,25,(Color){245,225,185,255});
    char value[32],need[32];forge_money_text_locale(&t->amount,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);forge_money_text_locale(&t->need,need,sizeof(need),true,g->language==GAME_LANGUAGE_EN);
    snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"奉上 %s  /  贡额 %s"),value,need);
    forge_text_fit(g,g->text_buffer,36,86,408,19,(Color){210,187,148,255});
    dragon(g,.35f+ease((age-350)/400)*.65f);
    altar(g);offerings(g);
    if(stage<3){
        const char *label=stage==1?(t->count?"贡剑升起":"贡台点火"):t->paid?"巨龙收下了贡品":"贡额不足 · 巨龙震怒";
        forge_text_center(g,label,30,315,420,24,c);
        if(stage==1){snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"贡剑 %u 把 · 碎铁一并奉上"),t->count);
            forge_text_center(g,g->text_buffer,30,354,420,18,(Color){206,181,142,255});}
    }else{
        float p=clamp01((age-FORGE_TRIBUTE_COINS_MS)/900);coins(g,p);
        const char *result=t->paid?"贡额达成":t->phase_after==PHASE_LOST?"契约终结":t->anger_before==2&&t->anger_after==2?"巨龙宽恕":"欠贡已记";
        forge_text_center(g,result,30,305,420,26,c);
        forge_money_t tally=t->reward;forge_money_scale(&tally,(uint32_t)(ease(p)*1000),1000);
        forge_money_text_locale(&tally,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);
        DrawRectangle(122,338,236,41,(Color){27,19,11,230});
        DrawRectangleLines(122,338,236,41,(Color){130,94,47,255});
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"金币 +%s"),value);
        forge_text_center(g,g->text_buffer,127,345,226,24,(Color){255,221,141,255});
        if(t->credit_gain){snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"剑币 +%u"),t->credit_gain);
            forge_text_center(g,g->text_buffer,140,399,200,17,(Color){242,210,147,255});}
    }
    if(!t->paid&&stage>=2){
        for(unsigned i=0;i<3;i++){
            float pulse=i+1U==t->anger_after?.6f+.4f*sinf(age*.015f):1;
            DrawCircle(221+(int)i*19,391,5,i<t->anger_after?tint(c,pulse):(Color){69,49,37,255});
        }
    }else for(int i=0;i<3;i++)DrawCircle(221+i*19,391,3,i<(int)stage?c:(Color){76,59,38,255});
    if(g->paused)forge_text_center(g,"仪式已暂停 · 点击继续",30,425,420,19,(Color){245,225,185,255});
    else if(age>=FORGE_TRIBUTE_SKIP_MS){
        const char *next=t->phase_after==PHASE_CLEAR?"契约完成 · 点击继续":t->phase_after==PHASE_LOST?"炉火暂熄 · 点击继续":"新一日即将开炉 · 点击跳过";
        forge_text_center(g,next,30,425,420,18,(Color){197,171,132,255});
    }else forge_text_center(g,"敲铃  ·  献贡  ·  结算",30,425,420,18,(Color){171,147,115,255});
}

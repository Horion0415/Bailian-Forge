// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "mosaico_raylib_fast.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clamp01(float x){return x<0?0:x>1?1:x;}
static float smooth(float x){x=clamp01(x);return x*x*(3-2*x);}
static uint32_t visual_random(forge_fx_t *fx)
{
    fx->rng=fx->rng*1664525U+1013904223U;
    return fx->rng;
}
static Color alpha(Color c,float opacity)
{
    c.a=(uint8_t)(255*clamp01(opacity));return c;
}
static void glow(game_handle_t g,int x,int y,Color c)
{
    Mosaico2DDrawAlphaMask(g->fx.glow,FORGE_FX_GLOW_W,
        FORGE_FX_GLOW_W,FORGE_FX_GLOW_H,x-48,y-24,c);
}
void forge_fx_reset(game_handle_t g)
{
    memset(&g->fx,0,sizeof(g->fx));
    forge_tribute_reset(g);
    forge_sword_reset(g);
    forge_swing_reset(g);
    g->fx.rng=g->config.seed^UINT32_C(0xf17e9a31);
    g->fx.result=RESULT_NONE;
    for(int y=0;y<FORGE_FX_GLOW_H;y++)for(int x=0;x<FORGE_FX_GLOW_W;x++){
        float u=(x-47.5f)/48,v=(y-23.5f)/24,a=clamp01(1-u*u-v*v);
        g->fx.glow[y*FORGE_FX_GLOW_W+x]=(uint8_t)(a*a*210);
    }
}
/* The model calls this after resolving its result. All random numbers below
 * belong to presentation only; forge_state_t and its random stream are read-only. */
void forge_particles(game_handle_t g,int kind)
{
    forge_fx_t *fx=&g->fx;
    const forge_blade_t *old=NULL,*blade=forge_active(g);
    if(g->undo_valid&&g->undo->active>=0)old=&g->undo->blades[g->undo->active];
    fx->active=true;fx->result=(uint8_t)kind;fx->age_ms=0;
    fx->duration_ms=kind==RESULT_BREAK?1350:kind==RESULT_GREAT?1150:850;
    fx->shake=kind==RESULT_BREAK?7:kind==RESULT_GREAT?5:kind==RESULT_DAMAGE?3.5f:kind==RESULT_HOLD?1.2f:2.8f;
    fx->family=old?old->family:blade?blade->family:0;
    fx->old_level=old?old->level:0;fx->new_level=blade?blade->level:0;
    forge_money_t value=forge_value(&g->state,blade);
    fx->value=value;
    bool inside=(blade&&blade->frenzy_ms>0)||(old&&old->frenzy_ms>0);
    forge_frenzy_strike(g,inside,blade&&blade->frenzy_ms>0&&(!old||old->frenzy_ms<=0));
    fx->heat=1;
    unsigned count=kind==RESULT_GREAT?60:kind==RESULT_BREAK?44:kind==RESULT_HOLD?16:36;
    if(inside)count=FORGE_FX_PARTICLES;
    for(unsigned i=0;i<FORGE_FX_PARTICLES;i++){
        forge_fx_particle_t *p=&fx->particles[i];memset(p,0,sizeof(*p));
        if(i>=count)continue;
        float direction=(float)(visual_random(fx)%6283)/1000.0f;
        float speed=85+(float)(visual_random(fx)%270);
        if(kind==RESULT_GREAT||inside)speed*=1.25f;
        *p=(forge_fx_particle_t){.x=260,.y=262,.vx=cosf(direction)*speed,
            .vy=-fabsf(sinf(direction)*speed)-40,.life=.25f+(float)(visual_random(fx)%600)/1000,
            .size=1+(float)(visual_random(fx)%3),.spin=(float)(visual_random(fx)%628)/100,
            .kind=(uint8_t)(kind==RESULT_BREAK&&i<20?1:0)};
        p->total=p->life;
    }
}
unsigned forge_fx_particle_count(game_handle_t g)
{
    unsigned n=0;for(unsigned i=0;i<FORGE_FX_PARTICLES;i++)n+=g->fx.particles[i].life>0;
    return n;
}
void forge_fx_update(game_handle_t g,uint32_t ms)
{
    forge_fx_t *fx=&g->fx;fx->clock_ms+=ms;
    if(fx->active){unsigned age=fx->age_ms+ms;
        fx->age_ms=(uint16_t)(age>fx->duration_ms?fx->duration_ms:age);
        if(age>=fx->duration_ms)fx->active=false;
    }
    fx->frenzy_pulse_ms=(uint16_t)(fx->frenzy_pulse_ms>ms?fx->frenzy_pulse_ms-ms:0);
    forge_frenzy_update(g,ms);
    forge_tribute_update(g,ms);
    float dt=(float)(ms>80?80:ms)/1000;
    fx->heat-=dt*.32f;if(fx->heat<0)fx->heat=0;
    for(unsigned i=0;i<FORGE_FX_PARTICLES;i++){
        forge_fx_particle_t *p=&fx->particles[i];if(p->life<=0)continue;
        p->life-=(float)ms/1000;
        p->x+=p->vx*dt;p->y+=p->vy*dt;p->vy+=p->kind?600*dt:360*dt;
        p->vx*=1-.6f*dt;p->spin+=dt*7;
    }
}
void forge_fx_camera(game_handle_t g,int *x,int *y)
{
    forge_fx_t *fx=&g->fx;*x=*y=0;
    if(!fx->active||fx->age_ms>270)return;
    float t=(float)fx->age_ms/270,strength=fx->shake*(1-t)*(1-t);
    *x=(int)lroundf(sinf(fx->age_ms*.14f+.7f)*strength);
    *y=(int)lroundf(cosf(fx->age_ms*.19f)*strength*.65f);
}
float forge_fx_hammer_angle(game_handle_t g)
{
    float t=g->animation_ms;
    if(!t)return 0;
    float cycle=g->swing.cycle_ms?g->swing.cycle_ms:FORGE_SWING_LEGACY_MS;
    float contact=g->swing.contact_ms?g->swing.contact_ms:FORGE_SWING_LEGACY_CONTACT_MS;
    if(g->fx.active&&g->fx.age_ms<20)return-34;
    if(t<contact*.4f)return 12*smooth(t/(contact*.4f));
    if(t<contact){float p=(t-contact*.4f)/(contact*.6f);return 12-46*p*p;}
    float rest=cycle-contact,hold=rest*.12f;
    if(t<contact+hold)return-34;
    float p=smooth((t-contact-hold)/(rest-hold));
    return-34*(1-p)+sinf(p*3.141593f)*9;
}
static void heat_waves(game_handle_t g,int dx,int dy)
{
    float time=(float)(g->fx.clock_ms%10000)/1000;
    for(int line=0;line<4;line++)for(int i=0;i<9;i++){
        int x=115+i*29,y=238-line*8;
        int a=(int)(sinf(time*4+i*.8f+line)*2),b=(int)(sinf(time*4+(i+1)*.8f+line)*2);
        DrawLine(x+dx,y+a+dy,x+29+dx,y+b+dy,(Color){255,171,80,(uint8_t)(27-line*5)});
    }
}
void forge_fx_ambient(game_handle_t g,int dx,int dy)
{
    forge_fx_t *fx=&g->fx;forge_blade_t *b=forge_active(g);
    float time=(float)(fx->clock_ms%12000)/1000;
    glow(g,81+dx,145+dy,(Color){255,108,18,(uint8_t)(65+sinf(time*9)*15)});
    for(int i=0;i<14;i++){
        float p=fmodf(time*.14f+i*.07143f,1);
        int x=21+i*11+(int)(sinf(p*9+i)*7)+dx,y=188-(int)(p*100)+dy;
        DrawLine(x,y,x+1,y-3,(Color){255,170,56,(uint8_t)(sinf(p*3.14159f)*150)});
    }
    if(!b)return;
    if(g->heat_ms){
        float progress=1-(float)g->heat_ms/FORGE_HEAT_DURATION_MS;
        glow(g,108+dx,173+dy,alpha((Color){255,137,36,255},.35f+progress*.55f));
        return;
    }
    if(b->heated){
        float strength=.18f+fx->heat*.22f;
        for(int x=166;x<=358;x+=96)glow(g,x+dx,263+dy,alpha((Color){255,116,29,255},strength));
        heat_waves(g,dx,dy);
        DrawLine(141+dx,266+dy,392+dx,266+dy,(Color){255,168,70,(uint8_t)(60+fx->heat*70)});
    }
    bool frenzy=b->frenzy_ms>0,ready=b->charge>=10;
    if(frenzy||ready){
        float pulse=.5f+.5f*sinf(time*(frenzy?11:4));
        unsigned strength=(unsigned)((frenzy?85:34)+pulse*(frenzy?55:35));
        if(frenzy)strength=(unsigned)(strength*smooth((float)b->frenzy_ms/500));
        for(int band=0;band<5;band++){
            Color c={255,(uint8_t)(90+band*15),22,(uint8_t)(strength/(band+1))};
            DrawRectangle(band*4,103,4,234,c);DrawRectangle(476-band*4,103,4,234,c);
        }
        forge_frenzy_back(g,dx,dy);
    }
}
bool forge_fx_break_visible(game_handle_t g)
{
    return g->fx.active&&g->fx.result==RESULT_BREAK&&g->fx.age_ms<1050;
}
static void shards(game_handle_t g,int dx,int dy)
{
    forge_fx_t *fx=&g->fx;
    if(!forge_fx_break_visible(g))return;
    float t=(float)fx->age_ms/1000,fade=1-clamp01((t-.55f)/.5f);
    for(int i=0;i<4;i++){
        float x=139+i*62+(i-1.5f)*t*135,y=241-t*(105+i%2*65)+t*t*410;
        forge_sword_draw_piece(g,fx->family,fx->old_level,(Rectangle){112+i*62,126,60,44},(Rectangle){x+dx,y+dy,60,44},
            alpha((Color){230,213,187,255},fade));
    }
}
void forge_fx_impact(game_handle_t g,int dx,int dy)
{
    forge_fx_t *fx=&g->fx;shards(g,dx,dy);
    if(fx->active&&fx->age_ms<420){
        float p=(float)fx->age_ms/420,fade=(1-p)*(1-p);
        Color flash=fx->result==RESULT_DAMAGE?(Color){255,87,51,255}:fx->result==RESULT_BREAK?(Color){255,128,49,255}:(Color){255,224,147,255};
        glow(g,260+dx,262+dy,alpha(flash,fade*.95f));
        if(fx->age_ms<210){
            DrawLineEx((Vector2){243+dx,262+dy},(Vector2){277+dx,262+dy},3,alpha((Color){255,247,218,255},fade));
            DrawLine(260+dx,252+dy,260+dx,274+dy,alpha(flash,fade));
        }
        DrawEllipseLines(260+dx,264+dy,15+p*66,4+p*14,alpha(flash,fade*.7f));
        if(fx->result==RESULT_GREAT)for(int i=0;i<10;i++){
            float angle=(float)i*.628319f,r0=12+p*40,r1=r0+12*(1-p);
            DrawLine((int)(260+cosf(angle)*r0)+dx,(int)(262+sinf(angle)*r0*.55f)+dy,
                (int)(260+cosf(angle)*r1)+dx,(int)(262+sinf(angle)*r1*.55f)+dy,alpha(flash,fade));
        }
    }
    if(fx->active&&fx->result==RESULT_DAMAGE&&fx->age_ms<650){
        float fade=1-(float)fx->age_ms/650;
        for(int i=0;i<4;i++){
            int x=180+i*44+dx,y=257+dy;
            DrawLine(x-4,y-6,x+3,y,alpha((Color){255,98,59,255},fade));
            DrawLine(x+3,y,x-3,y+7,alpha((Color){255,98,59,255},fade));
        }
    }
    for(unsigned i=0;i<FORGE_FX_PARTICLES;i++){
        const forge_fx_particle_t *p=&fx->particles[i];if(p->life<=0)continue;
        float fade=clamp01(p->life/p->total);int x=(int)p->x+dx,y=(int)p->y+dy;
        if(p->kind){
            float radius=p->size+1;
            Vector2 a={x+cosf(p->spin)*radius,y+sinf(p->spin)*radius};
            Vector2 b={x+cosf(p->spin+2.3f)*radius,y+sinf(p->spin+2.3f)*radius};
            Vector2 c={x+cosf(p->spin+4.2f)*radius,y+sinf(p->spin+4.2f)*radius};
            DrawTriangle(a,b,c,alpha((Color){158,145,123,255},fade));
        }else{
            Color c=alpha(fx->result==RESULT_DAMAGE?(Color){247,130,60,255}:(Color){255,205,99,255},fade);
            DrawLine(x,y,(int)(x-p->vx*.022f),(int)(y-p->vy*.022f),c);
            if(fade>.45f)DrawPixel(x,y,alpha((Color){255,246,197,255},fade));
        }
    }
    forge_frenzy_front(g,dx,dy);
}
void forge_fx_labels(game_handle_t g)
{
    forge_fx_t *fx=&g->fx;
    if(fx->active&&fx->age_ms<850){
        float p=(float)fx->age_ms/850,fade=p<.65f?1:(1-p)/.35f;
        static const char *const labels[]={"破 碎","损 伤","未 变","成 功","大 成 功","","精 锻"};
        Color c=fx->result==RESULT_BREAK||fx->result==RESULT_DAMAGE?(Color){255,139,100,255}:(Color){255,218,136,255};
        int size=fx->result==RESULT_GREAT?30:24;
        int x=262-forge_text_width(g,labels[fx->result],size)/2,y=192-(int)(smooth(p)*28);
        forge_text(g,labels[fx->result],x+1,y+2,size,alpha((Color){18,12,8,255},fade*.9f));
        forge_text(g,labels[fx->result],x,y,size,alpha(c,fade));
        if(fx->result==RESULT_SUCCESS||fx->result==RESULT_GREAT){
            char value[32];forge_money_text_locale(&fx->value,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);
            snprintf(g->text_buffer,sizeof(g->text_buffer),"+%u   %s G",fx->new_level,value);
            forge_text_center(g,g->text_buffer,150,y+35,240,18,alpha((Color){247,229,195,255},fade));
        }
    }
    forge_frenzy_label(g);
}

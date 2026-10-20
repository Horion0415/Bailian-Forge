// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "mosaico_raylib_fast.h"
#include <math.h>

enum { ENTRY_MS=900, EXIT_MS=500, ORBIT_SEGMENTS=48, COMETS=3, TAIL_SEGMENTS=12 };
static const float TAU=6.2831853f;
static float clamp01(float x){return x<0?0:x>1?1:x;}
static float ease(float x){x=clamp01(x);return x*x*(3-2*x);}
static Color gold(float a){return (Color){255,209,102,(uint8_t)(255*clamp01(a))};}
static Color ember(float a){return (Color){255,107,24,(uint8_t)(255*clamp01(a))};}
static void light(game_handle_t g,int x,int y,float a)
{
    Mosaico2DDrawAlphaMask(g->fx.glow,FORGE_FX_GLOW_W,FORGE_FX_GLOW_W,FORGE_FX_GLOW_H,
        x-48,y-24,gold(a));
}
static void begin(game_handle_t g)
{
    const forge_blade_t *b=forge_active(g);
    if(!b||b->frenzy_ms<=0)return;
    g->fx.frenzy=(forge_frenzy_t){.blade_id=b->id,.total_ms=(uint16_t)b->frenzy_ms,.active=true};
    g->fx.frenzy_pulse_ms=ENTRY_MS;
}
void forge_frenzy_strike(game_handle_t g,bool inside,bool entering)
{
    if(entering)begin(g);
    g->fx.frenzy.hit=inside;
}
void forge_frenzy_update(game_handle_t g,uint32_t ms)
{
    forge_frenzy_t *f=&g->fx.frenzy;
    const forge_blade_t *b=forge_active(g);
    bool inside=b&&b->frenzy_ms>0;
    if(inside&&(!f->active||f->blade_id!=b->id))begin(g);
    else if(inside){unsigned age=f->age_ms+ms;f->age_ms=(uint16_t)(age>12000?12000:age);}
    else f->active=false;
}
static float strength(game_handle_t g)
{
    const forge_blade_t *b=forge_active(g);
    const forge_frenzy_t *f=&g->fx.frenzy;
    if(!f->active||!b||b->id!=f->blade_id||b->frenzy_ms<=0)return 0;
    return ease((float)b->frenzy_ms/EXIT_MS);
}
static Vector2 orbit(float a,float rx,float ry,int dx,int dy)
{
    return (Vector2){262+cosf(a)*rx+dx,251+sinf(a)*ry+dy};
}
static void ring(float radius,float rotation,float opacity,int dx,int dy)
{
    /* Segmented seal: fixed small geometry, without rotating a large texture. */
    for(int i=0;i<ORBIT_SEGMENTS;i++){
        if(i%4==3)continue;
        float a=rotation+TAU*i/ORBIT_SEGMENTS,b=a+TAU*.8f/ORBIT_SEGMENTS;
        DrawLineV(orbit(a,radius,radius*.43f,dx,dy),orbit(b,radius,radius*.43f,dx,dy),gold(opacity));
    }
    for(int i=0;i<12;i++){
        float a=rotation+TAU*i/12;
        DrawLineV(orbit(a,radius-4,(radius-4)*.43f,dx,dy),
            orbit(a,radius+4,(radius+4)*.43f,dx,dy),gold(opacity));
    }
}
static void ribbons(game_handle_t g,int dx,int dy,bool front,float opacity)
{
    float time=(float)g->fx.frenzy.age_ms/1000;
    for(int comet=0;comet<COMETS;comet++){
        float head=time*4.8f+TAU*comet/COMETS;
        for(int j=TAIL_SEGMENTS;j>0;j--){
            float a=head-j*.085f,b=a+.085f;
            if((sinf((a+b)*.5f)>0)!=front)continue;
            float tail=1-(float)j/(TAIL_SEGMENTS+1);
            Vector2 p=orbit(a,132,37,dx,dy),q=orbit(b,132,37,dx,dy);
            DrawLineEx(p,q,6,ember(opacity*tail*.22f));
            DrawLineEx(p,q,3,ember(opacity*tail*.75f));
            DrawLineV(p,q,(Color){255,240,175,(uint8_t)(255*opacity*tail)});
        }
        if((sinf(head)>0)==front){
            Vector2 p=orbit(head,132,37,dx,dy);
            light(g,(int)p.x,(int)p.y,opacity*.38f);
            DrawCircleV(p,5,ember(opacity*.5f));
            DrawCircleV(p,2,(Color){255,248,209,(uint8_t)(255*opacity)});
            DrawLine((int)p.x-6,(int)p.y,(int)p.x+6,(int)p.y,gold(opacity));
            DrawLine((int)p.x,(int)p.y-4,(int)p.x,(int)p.y+4,gold(opacity*.8f));
        }
    }
}
void forge_frenzy_back(game_handle_t g,int dx,int dy)
{
    float a=strength(g);if(a<=0)return;
    float age=(float)g->fx.frenzy.age_ms,time=age/1000;
    float pulse=.5f+.5f*sinf(time*9),entry=1-ease(age/ENTRY_MS);
    /* Warm backlight and tapering shafts stay inside the iron/anvil viewport. */
    for(int i=0;i<5;i++)light(g,166+i*48+dx,258+dy,a*(.22f+pulse*.13f));
    for(int i=0;i<7;i++){
        int x=166+i*31+dx,h=34+(int)(22*(.5f+.5f*sinf(time*5+i*1.7f)));
        DrawTriangle((Vector2){x-12,259+dy},(Vector2){x+12,259+dy},
            (Vector2){x+8*sinf(time*2+i),259-h-entry*55+dy},gold(a*(.05f+entry*.09f)));
    }
    ring(117+pulse*3,-time*.55f,a*.35f,dx,dy);
    ring(105+pulse*2,time*.38f,a*.21f,dx,dy);
    ribbons(g,dx,dy,false,a);
    for(int i=0;i<18;i++){
        float p=fmodf(time*.65f+i*.05556f,1);
        int x=134+i*14+(int)(sinf(p*8+i)*12)+dx,y=288-(int)(p*116)+dy;
        float fade=sinf(p*3.14159f)*a;
        DrawLine(x,y,x-2,y+5,gold(fade*.7f));
        if(i%3==0)DrawPixel(x,y-1,(Color){255,247,207,(uint8_t)(fade*255)});
    }
    if(age<ENTRY_MS){
        float p=age/ENTRY_MS,r=24+ease(p)*194,fade=(1-p)*a;
        DrawTriangle((Vector2){230+dx,260+dy},(Vector2){294+dx,260+dy},
            (Vector2){262+dx,126+dy},gold(fade*.15f));
        DrawEllipseLines(262+dx,251+dy,r,r*.49f,gold(fade*.9f));
        DrawEllipseLines(262+dx,251+dy,r*.78f,r*.38f,ember(fade*.65f));
        /* An upright, opening halo gives the transition a distinct silhouette. */
        float radius=42+ease(p)*58;
        for(int i=0;i<32;i++){
            float angle=TAU*i/32+time*1.2f,next=angle+TAU*.75f/32;
            Vector2 v={262+cosf(angle)*radius+dx,236+sinf(angle)*radius*.8f+dy};
            Vector2 w={262+cosf(next)*radius+dx,236+sinf(next)*radius*.8f+dy};
            DrawLineV(v,w,gold(fade*.85f));
            if(i%4==0)DrawLineV(v,(Vector2){v.x+cosf(angle)*6,v.y+sinf(angle)*6},gold(fade));
        }
        for(int i=0;i<16;i++){
            float angle=TAU*i/16+.1f,inner=28+p*100,outer=inner+42*(1-p);
            DrawLineV(orbit(angle,inner,inner*.6f,dx,dy),orbit(angle,outer,outer*.6f,dx,dy),gold(fade*.7f));
        }
    }
}
void forge_frenzy_front(game_handle_t g,int dx,int dy)
{
    float a=strength(g);
    if(a>0){
        ribbons(g,dx,dy,true,a);
        float time=(float)g->fx.frenzy.age_ms/1000;
        /* Small, sharp glints travel along the blade; the blade remains visible. */
        for(int i=0;i<3;i++){
            float p=fmodf(time*.72f+i*.33333f,1);
            int x=145+(int)(p*239)+dx,y=259+dy;
            float fade=sinf(p*3.14159f)*a;
            DrawLine(x-5,y,x+5,y,gold(fade));DrawLine(x,y-3,x,y+3,gold(fade*.8f));
        }
    }
    if(g->fx.active&&g->fx.frenzy.hit&&g->fx.age_ms<360){
        float p=(float)g->fx.age_ms/360,fade=(1-p)*(1-p);
        for(int i=0;i<16;i++){
            float angle=TAU*i/16+.15f,inner=16+p*91,outer=inner+23*(1-p);
            DrawLineV(orbit(angle,inner,inner*.62f,dx,dy),orbit(angle,outer,outer*.62f,dx,dy),gold(fade));
        }
        DrawEllipseLines(262+dx,259+dy,22+p*116,5+p*25,gold(fade*.8f));
    }
}
void forge_frenzy_label(game_handle_t g)
{
    float a=strength(g);unsigned age=g->fx.frenzy.age_ms;
    if(a<=0||age>=ENTRY_MS)return;
    float t=(float)age/ENTRY_MS,fade=a*ease(t/.1f)*(1-ease((t-.62f)/.38f));
    int y=118-(int)(ease(t)*6),spread=(int)(ease(t/.55f)*12);
    /* Native-size A8 text is never scaled or shaken. */
    int x=262-forge_text_width(g,"入 神",34)/2,width=forge_text_width(g,"入 神",34);
    DrawLine(x-25-spread,y+23,x-5-spread,y+23,gold(fade*.7f));
    DrawLine(x+width+5+spread,y+23,x+width+25+spread,y+23,gold(fade*.7f));
    forge_text(g,"入 神",x+1,y+2,34,(Color){20,12,6,(uint8_t)(fade*235)});
    forge_text(g,"入 神",x,y,34,(Color){255,223,142,(uint8_t)(fade*255)});
}

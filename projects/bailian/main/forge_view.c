// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "forge_font.h"
/* Native-size A8 glyphs retain full strokes and grayscale edge coverage. */
#include "mosaico_raylib_fast.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static const Color FORGE_GOLD={239,193,115,255},FORGE_CREAM={242,224,188,255},FORGE_MUTED={177,154,121,255},FORGE_DARK={20,15,12,255};
static uint32_t decode(const char **text)
{
    const unsigned char *p=(const unsigned char*)*text;
    uint32_t c=*p++;
    unsigned count=0;
    if(c>=0xf0){c&=7;
        count=3;
        }else if(c>=0xe0){c&=15;
        count=2;
        }else if(c>=0xc0){c&=31;
        count=1;
        }
    while(count--&&*p)c=(c<<6)|(*p++&63);
    *text=(const char*)p;
    return c;
}
static int glyph_index(uint32_t c)
{
    int lo=0,hi=(int)FORGE_FONT_COUNT-1;
    while(lo<=hi){int mid=(lo+hi)/2;
        if(forge_font_codepoints[mid]==c)return mid;
        if(forge_font_codepoints[mid]<c)lo=mid+1;
        else hi=mid-1;
        }return-1;
}
static unsigned font_size_index(int size)
{
    unsigned index=0;
    for(unsigned i=1;i<FORGE_FONT_SIZE_COUNT;i++){
        if(forge_font_sizes[i]>size)break;
        index=i;
    }
    return index;
}
static const forge_font_glyph_t *glyph(uint32_t c,int size)
{
    int index=glyph_index(c);
    if(index<0)index=glyph_index(0x25a1);
    return index<0?NULL:&forge_font_glyphs[font_size_index(size)*FORGE_FONT_COUNT+(unsigned)index];
}
static int advance(uint32_t c,int size)
{
    const forge_font_glyph_t *value=glyph(c,size);
    return value?value->advance:size;
}
void forge_text(game_handle_t g,const char *text,int x,int y,int size,Color color)
{
    if(!text||!g->font.data)return;
    text=forge_tr(g,text);
    while(*text){uint32_t c=decode(&text);
        const forge_font_glyph_t *value=glyph(c,size);
        if(value){size_t bytes=(size_t)value->width*value->height;
            if(value->offset<=g->font.size&&bytes<=g->font.size-value->offset)
                Mosaico2DDrawAlphaMask(g->font.data+value->offset,value->width,
                    value->width,value->height,x+value->x,y+value->y,color);
            x+=value->advance;
        }else x+=size;
        }
}
int forge_text_width(game_handle_t g,const char *text,int size)
{
    text=forge_tr(g,text);int width=0;
    while(text&&*text)width+=advance(decode(&text),size);
    return width;
}
static int fit_size(game_handle_t g,const char *text,int width,int size)
{
    while(size>14&&forge_text_width(g,text,size)>width)size--;
    return size;
}
void forge_text_fit(game_handle_t g,const char *text,int x,int y,int width,int size,Color color)
{
    forge_text(g,text,x,y,fit_size(g,text,width,size),color);
}
void forge_text_center(game_handle_t g,const char *text,int x,int y,int width,int size,Color color)
{
    size=fit_size(g,text,width,size);
    forge_text(g,text,x+(width-forge_text_width(g,text,size))/2,y,size,color);
}
/* One native layout path measures and draws both CJK and word-wrapped English. */
static unsigned flow_text(game_handle_t g,const char *text,int x,int y,int width,int size,unsigned lines,bool draw,Color color)
{
    int start=x;
    unsigned row=0;bool word_start=true;
    while(text&&*text){
        if(x==start&&*text==' '){text++;continue;}
        const char *before=text;
        uint32_t c=decode(&text);
        if(word_start&&c>32&&c<128){
            const char *word=before;int word_width=0;
            while((unsigned char)*word>32&&(unsigned char)*word<128)word_width+=advance((unsigned char)*word++,size);
            if(word_width<=width&&x>start&&x+word_width>start+width){x=start;y+=size+3;row++;}
        }
        int step=advance(c,size);
        if(x+step>start+width||c=='\n'){x=start;
            y+=size+3;
            row++;
            if(c=='\n'){word_start=true;continue;}
            }
        word_start=c<=32||c>=128;
        if(x==start&&c==' ')continue;
        char one[5]={0};
        size_t n=(size_t)(text-before);
        if(n>4)n=4;
        memcpy(one,before,n);
        if(draw&&row<lines)forge_text(g,one,x,y,size,color);
        x+=step;
    }
    return row+1;
}
static void wrap(game_handle_t g,const char *text,int x,int y,int width,int size,unsigned lines,Color color)
{
    text=forge_tr(g,text);
    while(size>14&&flow_text(g,text,x,y,width,size,lines,false,color)>lines)size--;
    (void)flow_text(g,text,x,y,width,size,lines,true,color);
}
static void frame(int x,int y,int w,int h,bool active)
{
    DrawRectangleGradientV(x,y,w,h,active?(Color){77,44,20,255}:(Color){43,31,23,255},FORGE_DARK);
    DrawRectangleLines(x,y,w,h,active?FORGE_GOLD:(Color){98,75,46,255});
    DrawLine(x+3,y+3,x+w-4,y+3,(Color){110,83,52,255});
    DrawCircle(x+6,y+7,2,(Color){155,127,85,255});
    DrawCircle(x+w-7,y+7,2,(Color){155,127,85,255});
}
static void button(game_handle_t g,int x,int y,int w,int h,const char *label,bool selected)
{
    frame(x,y,w,h,selected);
    label=forge_tr(g,label);
    int size=fit_size(g,label,w-16,21),width=forge_text_width(g,label,size);
    forge_text(g,label,x+(w-width)/2,y+(h-size)/2,size,selected?FORGE_GOLD:FORGE_CREAM);
}
void forge_audio_button(game_handle_t g,int x,int y,int width)
{
    bool muted=game_is_muted(g);frame(x,y,width,34,muted);
    forge_text_center(g,muted?"静音":"有声",x,y+9,width,14,muted?FORGE_GOLD:FORGE_CREAM);
}
void forge_language_button(game_handle_t g,int x,int y,int width)
{
    frame(x,y,width,34,g->language==GAME_LANGUAGE_EN);
    forge_text_center(g,g->language==GAME_LANGUAGE_EN?"ZH":"EN",x,y+9,width,14,FORGE_CREAM);
}
static void sprite(MosaicoAtlas atlas,const char *name,int x,int y,int w,int h,Color tint)
{
    const MosaicoSpriteFrame *f=MosaicoAtlasGetFrame(atlas,mosaico_game_asset_id(name));
    if(!f)return;
    DrawTexturePro(atlas.texture,f->source,(Rectangle){(float)x,(float)y,(float)w,(float)h},(Vector2){0,0},0,tint);
}
static void relic_icon(game_handle_t g,int id,int x,int y,int size)
{
    if(id<0||id>=R_COUNT)return;
    char name[20];
    snprintf(name,sizeof(name),"relic_%u",forge_relics[id].icon);
    sprite(g->relic_atlas,name,x,y,size,size,WHITE);
}
static void sword(game_handle_t g,int family,unsigned level,float x,float y,float w,float h,Color tint,bool animate)
{
    forge_sword_draw(g,(unsigned)family,level,(Rectangle){x,y,w,h},(Vector2){0,0},0,tint,animate);
}
static void amount(game_handle_t g,const forge_money_t *m,int x,int y,int size,Color color)
{
    char value[32];
    forge_money_text_locale(m,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);
    forge_text(g,value,x,y,size,color);
}
static void header(game_handle_t g)
{
    forge_state_t *s=&g->state;
    DrawRectangleGradientV(0,0,480,55,(Color){55,35,21,255},(Color){24,17,12,255});
    DrawLine(0,54,480,54,(Color){154,114,58,255});
    snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"第%u日"),s->day);
    forge_text_fit(g,g->text_buffer,12,11,76,s->day>=100?21:26,FORGE_CREAM);
    forge_audio_button(g,91,10,36);
    forge_language_button(g,131,10,46);
    forge_text(g,"贡额",184,4,16,FORGE_MUTED);
    forge_money_t q=forge_quota(s);
    char value[32];forge_money_text_locale(&q,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);
    forge_text_fit(g,value,184,24,108,22,FORGE_GOLD);
    forge_text(g,"金币",302,4,16,FORGE_MUTED);
    forge_money_text_locale(&s->coins,value,sizeof(value),true,g->language==GAME_LANGUAGE_EN);
    forge_text_fit(g,value,302,25,84,20,FORGE_CREAM);
    snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"剑币 %u"),s->credits);
    forge_text_fit(g,g->text_buffer,376,4,99,16,FORGE_GOLD);
    for(unsigned i=0;i<3;i++)DrawCircle(402+(int)i*23,37,6,i<s->anger?(Color){220,69,34,255}:(Color){77,56,38,255});
}
static void navigation(game_handle_t g)
{
    static const char *const labels[4]={"锻造","剑架","遗物","贡台"};
    for(unsigned i=0;i<4;i++){int x=(int)i*120;
        bool active=g->screen==i||(i==2&&(g->screen==SCREEN_MERCHANT||g->screen==SCREEN_ITEMS));
        frame(x+2,416,116,62,active);
        forge_text_center(g,labels[i],x+2,441,116,23,active?FORGE_GOLD:FORGE_CREAM);
        if(i==0){DrawRectangle(x+50,426,19,6,FORGE_GOLD);
            DrawLine(x+58,431,x+58,439,FORGE_GOLD);
            }
        if(i==1){DrawLine(x+50,425,x+65,438,FORGE_GOLD);
            DrawLine(x+65,425,x+50,438,FORGE_GOLD);
            }
        if(i==2){DrawCircle(x+59,431,6,FORGE_GOLD);
            DrawCircle(x+59,431,3,(Color){46,33,22,255});
            }
        if(i==3){DrawRectangleLines(x+51,425,17,14,FORGE_GOLD);
            DrawLine(x+49,438,x+70,438,FORGE_GOLD);
            }
    }
}
static void blade_value(game_handle_t g,const forge_blade_t *b,float opacity)
{
    if(opacity<.01f)return;
    if(opacity>.995f)frame(10,113,126,88,false);
    else{
        DrawRectangleGradientV(10,113,126,88,Fade((Color){43,31,23,255},opacity),Fade(FORGE_DARK,opacity));
        DrawRectangleLines(10,113,126,88,Fade((Color){98,75,46,255},opacity));
    }
    snprintf(g->text_buffer,sizeof(g->text_buffer),"+%u",b->level);
    forge_text(g,g->text_buffer,24,117,30,Fade(FORGE_GOLD,opacity));
    forge_money_t value=forge_value(&g->state,b);
    amount(g,&value,20,155,21,Fade(FORGE_CREAM,opacity));
    const char *stage=forge_tr(g,forge_sword_stage_name(b->level));
    if(g->shape.morphing){
        snprintf(g->text_buffer,sizeof(g->text_buffer),"%s · %s",stage,
            forge_sword_stage(g->shape.from_level)>forge_sword_stage(b->level)?forge_tr(g,"回落"):forge_tr(g,"蜕变"));
        forge_text(g,g->text_buffer,20,181,14,Fade(FORGE_GOLD,opacity));
    }else forge_text(g,stage,20,181,14,Fade(FORGE_MUTED,opacity));
}
static void charge_hud(game_handle_t g,const forge_blade_t *b,float opacity)
{
    if(opacity<.01f)return;
    bool frenzy=b->frenzy_ms>0;
    if(frenzy)snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"入神 %u.%u秒  强化蓄能 %u%%"),(unsigned)b->frenzy_ms/1000,(unsigned)b->frenzy_ms%1000/100,forge_energy_percent(&g->state,b));
    else snprintf(g->text_buffer,sizeof(g->text_buffer),b->charge>=10?forge_tr(g,"入神就绪 %u/10"):forge_tr(g,"入神 %u/10"),b->charge);
    forge_text_fit(g,g->text_buffer,169,300,296,frenzy?17:20,Fade(frenzy?(Color){255,200,89,255}:FORGE_CREAM,opacity));
    for(unsigned i=0;i<10;i++){
        Color c=frenzy?(Color){255,153,46,255}:i<b->charge?(Color){228,151,47,255}:(Color){49,40,31,255};
        int x=169+(int)i*28;
        if(frenzy){
            unsigned total=g->fx.frenzy.total_ms?g->fx.frenzy.total_ms:4000;
            float fill=(float)b->frenzy_ms*10/total-i;
            if(fill<0)fill=0;else if(fill>1)fill=1;
            DrawRectangle(x,326,24,5,Fade((Color){49,40,31,255},opacity));
            DrawRectangle(x,326,(int)(24*fill),5,Fade(c,opacity));
            if(fill>0)DrawLine(x,326,x+(int)(23*fill),326,Fade((Color){255,225,145,255},opacity));
        }else DrawRectangle(x,326,24,5,Fade(c,opacity));
    }
}
static void forge_scene(game_handle_t g)
{
    forge_state_t *s=&g->state;
    forge_blade_t *b=forge_active(g);
    int dx=0,dy=0;forge_fx_camera(g,&dx,&dy);
    forge_heat_view_t heat;forge_heat_read(g,&heat);
    BeginScissorMode(0,103,480,234);
    forge_fx_ambient(g,dx,dy);
    if(b){
        sword(g,b->family,b->level,heat.sword.x+dx,heat.sword.y+dy,heat.sword.width,heat.sword.height,heat.tint,true);
        forge_heat_draw_light(g,&heat,dx,dy);
    }
    if(b||g->animation_ms||forge_fx_break_visible(g))
        DrawTexturePro(g->hammer.texture,(Rectangle){0,0,224,224},
            (Rectangle){440+dx,333+dy+heat.hammer_drop,224,224},(Vector2){202,202},forge_fx_hammer_angle(g),WHITE);
    forge_fx_impact(g,dx,dy);
    EndScissorMode();
    if(b){
        uint16_t odds[5];
        forge_tickets(s,b,odds);
        bool refine=b->frenzy_ms>0&&forge_energy_percent(s,b)<100;
        if(refine){memset(odds,0,sizeof(odds));odds[0]=1000;}
        static const char *const names[5]={"破碎","损伤","不变","成功","大成功"};
        static const Color colors[5]={{221,101,76,255},{230,161,91,255},{178,166,149,255},{154,190,137,255},{242,202,112,255}};
        for(unsigned i=0;i<5;i++){int x=(int)i*96;
            DrawRectangle(x+2,61,92,42,(Color){21,17,14,230});
            /* This is the refining action, distinct from the Tempered sword tier. */
            forge_text(g,refine&&i==0?(g->language==GAME_LANGUAGE_EN?"Refine":"精锻"):names[i],x+8,63,16,refine&&i==0?FORGE_GOLD:colors[i]);
            snprintf(g->text_buffer,sizeof(g->text_buffer),"%u.%u%%",odds[i]/10,odds[i]%10);
            forge_text(g,g->text_buffer,x+13,82,17,FORGE_CREAM);
            }
        blade_value(g,b,heat.value_alpha);
        if(!b->family&&b->level>=6){button(g,397,111,75,38,"进化",true);
            }
        charge_hud(g,b,heat.charge_alpha);
        forge_heat_draw_hud(g,&heat);
    }else if(!forge_fx_break_visible(g)){
        frame(93,156,292,73,false);
        forge_text_center(g,"铁砧空着",101,166,276,28,FORGE_CREAM);
        forge_text_center(g,"到剑架选择一把剑",101,202,276,20,FORGE_MUTED);
    }
    forge_fx_labels(g);
    button(g,12,339,210,36,g->heat_ms?"取出放入贡台":"放入贡台",false);
    button(g,258,339,210,36,"发条回溯",false);
    if(forge_heat_draw_footer(g,&heat))return;
    if(s->scale||s->chalk||s->powder){
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"已准备：%s%s%s"),s->scale?forge_tr(g,"龙鳞 "):"",s->chalk?forge_tr(g,"粉笔 "):"",s->powder?forge_tr(g,"火药"):"");
        forge_text(g,g->text_buffer,13,379,17,FORGE_GOLD);
        if(g->notice_ms)wrap(g,g->notice,13,398,454,14,1,FORGE_CREAM);
    }else if(g->notice_ms)wrap(g,g->notice,13,379,454,17,2,FORGE_CREAM);
    else forge_text_fit(g,b?(g->heat_ms?"入炉加热中…":!b->heated?"点击炉火或铁砧，先给剑胚加热":"点击铁砧落锤，随时可以收手") :"每日送来三把剑，先留够贡品",18,386,444,18,FORGE_MUTED);
}
static void page_title(game_handle_t g,const char *title)
{
    DrawRectangle(0,57,480,357,(Color){18,14,12,242});
    forge_text_fit(g,title,14,60,452,24,FORGE_GOLD);
}
static void inventory(game_handle_t g,bool tribute)
{
    page_title(g,tribute?"巨龙贡台":"留给下一锤的剑");
    forge_money_t offered=forge_tribute(&g->state),need=forge_quota(&g->state);
    if(tribute){forge_text(g,"已摆",211,64,17,FORGE_MUTED);
        amount(g,&offered,257,64,22,forge_money_compare(&offered,&need)>=0?(Color){167,206,135,255}:FORGE_CREAM);
        }
    button(g,12,87,88,33,"上一页",false);
    button(g,380,87,88,33,"下一页",false);
    snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"第%u页"),g->page+1);
    forge_text(g,g->text_buffer,196,93,20,FORGE_MUTED);
    if(g->state.backlog){snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"待领%u把 · 空位自动补发"),g->state.backlog);forge_text_fit(g,g->text_buffer,211,113,257,14,FORGE_GOLD);}
    unsigned n=0,shown=0;
    for(unsigned i=0;i<FORGE_BLADES;i++){
        forge_blade_t *b=&g->state.blades[i];
        if(!b->place||(tribute?b->place!=PLACE_TRIBUTE:b->place==PLACE_TRIBUTE))continue;
        if(n++<g->page*6)continue;
        if(shown>=6)break;
        unsigned col=shown%2,row=shown/2;
        int x=10+(int)col*240,y=130+(int)row*74;
        frame(x,y,220,68,g->selected==i);
        sword(g,b->family,b->level,x+3,y+9,98,58,WHITE,false);
        snprintf(g->text_buffer,sizeof(g->text_buffer),"#%lu +%u",(unsigned long)b->id,b->level);
        forge_text(g,g->text_buffer,x+105,y+5,20,FORGE_GOLD);
        forge_money_t v=forge_value(&g->state,b);
        amount(g,&v,x+105,y+32,18,FORGE_CREAM);
        shown++;
    }
    if(!shown)wrap(g,tribute?"贡台还没有摆剑。去剑架选好贡品，再敲响铃铛。":"剑架暂时空了。可敲铃结束本日，次日会送来新剑；第三次欠贡会失败。",32,168,418,22,5,FORGE_MUTED);
    if(tribute){button(g,12,363,210,41,"取回所选剑",false);
        button(g,258,363,210,41,"敲铃进贡",true);
        }
    else{button(g,8,363,148,41,"选择锻造",true);
        button(g,166,363,148,41,"摆入贡台",false);
        button(g,324,363,148,41,"换剑币",false);
        }
}
static void gift(game_handle_t g)
{
    DrawRectangle(0,55,480,425,(Color){14,11,9,224});
    forge_text_center(g,"开 炉 赠 礼",12,63,456,30,FORGE_GOLD);
    forge_text_center(g,"三选一，决定这一局的起手",12,103,456,19,FORGE_MUTED);
    for(unsigned i=0;i<3;i++){
        int id=g->state.offers[i];
        if(id<0)continue;
        int x=12+(int)i*156;
        const forge_relic_def_t *r=&forge_relics[id];
        frame(x,133,144,265,false);
        relic_icon(g,id,x+38,146,68);
        wrap(g,r->name,x+10,225,126,20,2,FORGE_GOLD);
        wrap(g,r->effect,x+10,273,126,16,5,FORGE_CREAM);
        forge_text_center(g,"免费选择",x+8,373,128,20,FORGE_GOLD);
    }
    forge_text_center(g,"只拿一件，其余两件随炉烟消失",12,431,456,20,FORGE_MUTED);
}
static void relics(game_handle_t g)
{
    page_title(g,"遗物机与八格遗物架");
    button(g,8,86,149,36,"遗物售卖机",!g->shop_page);
    button(g,165,86,149,36,"已持有",g->shop_page!=0);
    button(g,322,86,150,36,"金币刷新",false);
    if(!g->shop_page){
        for(unsigned i=0;i<3;i++){int x=9+(int)i*158,id=g->state.offers[i];
            frame(x,132,146,202,false);
            if(id<0){forge_text_center(g,g->state.day==1?"明日营业":"已售出",x+8,214,130,20,FORGE_MUTED);
                continue;
                }
            const forge_relic_def_t *r=&forge_relics[id];
            relic_icon(g,id,x+29,148,88);
            wrap(g,r->name,x+10,242,126,20,2,FORGE_GOLD);
            snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"%u 剑币"),r->cost);
            forge_text(g,g->text_buffer,x+35,291,20,FORGE_CREAM);
            }
    }else{
        for(unsigned i=0;i<g->state.owned_count;i++){int id=g->state.owned[i],x=(int)(i%4)*120+8,y=133+(int)(i/4)*103;
            frame(x,y,104,95,false);
            relic_icon(g,id,x+25,y+4,55);
            wrap(g,forge_relics[id].name,x+5,y+60,95,14,2,FORGE_GOLD);
            }
    }
    button(g,12,359,210,44,"幽灵商人",false);
    button(g,258,359,210,44,"消耗品背包",false);
    if(g->notice_ms)wrap(g,g->notice,15,336,450,17,1,FORGE_CREAM);
}
static const unsigned item_icons[ITEM_COUNT]={2,7,1,3,15,10,3};
static void merchant(game_handle_t g)
{
    page_title(g,"幽灵商人 · 每三日来访");
    button(g,12,87,88,33,"上一页",false);
    button(g,380,87,88,33,"下一页",false);
    if(!g->state.merchant_count)wrap(g,"今日商人未到。每逢第3、6、9日等日子，他会带来新剑和消耗品。",35,154,410,23,5,FORGE_CREAM);
    for(unsigned row=0;row<4;row++){unsigned index=g->page*4+row;
        if(index>=g->state.merchant_count)break;
        forge_offer_t *o=&g->state.merchant[index];
        int y=130+(int)row*60;
        frame(12,y,456,55,false);
        if(o->kind<2)sword(g,o->family,o->level,18,y,87,52,WHITE,false);
        else{char key[20];
            snprintf(key,sizeof(key),"relic_%u",item_icons[o->id]);
            sprite(g->relic_atlas,key,34,y+3,48,48,WHITE);
            }
        forge_text_fit(g,o->kind<2?forge_family_names[o->family]:forge_item_names[o->id],111,y+3,214,20,FORGE_GOLD);
        forge_money_t p=forge_merchant_price(&g->state,index);
        amount(g,&p,111,y+28,18,FORGE_CREAM);
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"剩%u · 详情"),o->stock);
        forge_text_fit(g,g->text_buffer,335,y+18,123,18,o->stock?FORGE_CREAM:FORGE_MUTED);
    }
    button(g,12,376,456,28,"返回遗物机",false);
}
static void items(game_handle_t g)
{
    page_title(g,"铁砧旁的消耗品");
    forge_text_fit(g,"点击查看效果与使用条件",32,85,424,16,FORGE_MUTED);
    for(unsigned i=0;i<5;i++){int y=104+(int)i*60;
        frame(12,y,456,55,false);
        char key[20];
        snprintf(key,sizeof(key),"relic_%u",item_icons[i]);
        sprite(g->relic_atlas,key,24,y+3,48,48,WHITE);
        forge_text_fit(g,forge_item_names[i],93,y+7,203,22,FORGE_GOLD);
        bool armed=i==ITEM_SCALE?g->state.scale:i==ITEM_CHALK?g->state.chalk:i==ITEM_POWDER?g->state.powder:false;
        if(armed)forge_text(g,"已准备",309,y+12,14,FORGE_GOLD);
        snprintf(g->text_buffer,sizeof(g->text_buffer),"× %u",g->state.items[i]);
        forge_text(g,g->text_buffer,366,y+10,22,FORGE_CREAM);
        }
}
static void book(game_handle_t g)
{
    page_title(g,"锻造契约与手记");
    button(g,8,87,149,34,"玩法规则",g->book_tab==0);
    button(g,165,87,149,34,"传承进度",g->book_tab==1);
    button(g,322,87,150,34,"遗物图鉴",g->book_tab==2);
    forge_state_t *s=&g->state;
    if(g->book_tab==0){
        wrap(g,"先加热再落锤。成功升1级，大成功升2级；损伤通常降1级，破碎失去整把剑。旧剑+6可选有不同特长的剑系。",22,137,436,19,4,FORGE_CREAM);
        wrap(g,"10点充能开启4秒入神。快速连击精锻加值，强化蓄能满才进行完整判定；精锻不触发遗物或消耗已备道具。速度遗物也加快蓄能。",22,236,436,18,4,FORGE_CREAM);
        wrap(g,"贡台剑值用于缴贡，金币不能代替。怒气累计3点失败，足贡不清零；第21日存活通关。满仓供剑腾出空位后自动补发。",22,332,436,17,4,FORGE_MUTED);
    }else if(g->book_tab==1){
        unsigned found=0;for(unsigned i=0;i<94;i++)found+=(s->found[i/64]>>(i%64))&1U;
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"已发现遗物 %u/94 · 本局最高 +%u"),found,s->max_level);
        forge_text(g,g->text_buffer,22,137,20,FORGE_GOLD);
        forge_text(g,"剑系达到+20，将专属遗物加入后续商店",22,168,17,FORGE_MUTED);
        for(unsigned i=1;i<10;i++){
            int x=12+(int)((i-1)%3)*158,y=199+(int)((i-1)/3)*48;
            bool unlocked=(s->unlocks&(1U<<i))!=0;frame(x,y,150,42,unlocked);
            forge_text_fit(g,forge_family_names[i],x+7,y+2,136,17,unlocked?FORGE_GOLD:FORGE_MUTED);
            forge_text(g,unlocked?"遗物已解锁":"尚未解锁",x+7,y+23,14,FORGE_MUTED);
        }
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"传说+99：%s   契约通关：%s"),s->achievements&1?forge_tr(g,"达成"):forge_tr(g,"未达成"),s->achievements&4?forge_tr(g,"达成"):forge_tr(g,"未达成"));
        forge_text(g,g->text_buffer,22,356,17,FORGE_CREAM);
        forge_text(g,s->unlocks&1024?"终身契约：已解锁":"终身契约：无尽模式足额进贡3次后解锁",22,383,17,FORGE_MUTED);
    }else{
        button(g,12,130,88,32,"上页",false);button(g,380,130,88,32,"下页",false);
        snprintf(g->text_buffer,sizeof(g->text_buffer),"%u / 16",g->page+1);forge_text(g,g->text_buffer,198,135,18,FORGE_MUTED);
        for(unsigned cell=0;cell<6;cell++){
            unsigned id=g->page*6+cell;if(id>=94)break;
            int x=10+(int)(cell%2)*240,y=170+(int)(cell/2)*74;
            bool found=(s->found[id/64]&(UINT64_C(1)<<(id%64)))!=0;frame(x,y,220,66,false);
            if(found)relic_icon(g,(int)id,x+6,y+8,48);
            wrap(g,found?forge_relics[id].name:"尚未发现",x+63,y+10,150,18,2,found?FORGE_GOLD:FORGE_MUTED);
        }
    }
}
static void modal(game_handle_t g)
{
    DrawRectangle(0,55,480,359,(Color){0,0,0,170});
    frame(32,102,416,276,false);
    if(g->modal==MODAL_FAMILY){
        frame(32,102,416,304,false);forge_text_center(g,"选择一条进化路线",48,111,384,25,FORGE_GOLD);
        for(int i=0;i<6;i++)button(g,48+(i%2)*196,143+(i/2)*50,184,43,forge_family_names[i+1],g->detail_item==i+1);
        wrap(g,forge_family_effects[g->detail_item],54,300,370,17,2,FORGE_CREAM);
        button(g,48,351,178,43,"取消",false);button(g,254,351,178,43,"确认进化",true);
        return;
        }
    if(g->modal==MODAL_ITEM||g->modal==MODAL_OFFER){
        const char *title,*effect;
        if(g->modal==MODAL_ITEM){title=forge_item_names[g->detail_item];effect=forge_item_effects[g->detail_item];}
        else{forge_offer_t *offer=&g->state.merchant[g->detail_offer];
            title=offer->kind<2?forge_family_names[offer->family]:forge_item_names[offer->id];
            effect=offer->kind<2?forge_family_effects[offer->family]:forge_item_effects[offer->id];
        }
        forge_text_fit(g,title,54,128,370,25,FORGE_GOLD);wrap(g,effect,54,200,370,19,4,FORGE_CREAM);
        if(g->modal==MODAL_OFFER){forge_money_t price=forge_merchant_price(&g->state,g->detail_offer);forge_text(g,"金币",54,167,18,FORGE_MUTED);amount(g,&price,115,166,20,FORGE_GOLD);}
        else{snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"持有 %u 份"),g->state.items[g->detail_item]);forge_text(g,g->text_buffer,54,168,18,FORGE_MUTED);}
        button(g,48,320,178,43,"返回",false);button(g,254,320,178,43,g->modal==MODAL_ITEM?"使用1份":"确认购买",true);
        if(g->notice_ms)wrap(g,g->notice,22,384,436,16,1,FORGE_MUTED);
        return;
    }
    if(g->modal==MODAL_RELIC){int id=g->detail_relic;
        const forge_relic_def_t *r=&forge_relics[id];
        relic_icon(g,id,49,119,62);
        forge_text_fit(g,r->name,126,131,306,25,FORGE_GOLD);
        wrap(g,r->effect,54,204,370,20,4,FORGE_CREAM);
        button(g,48,320,178,43,"返回",false);
        if(g->detail_readonly)button(g,254,320,178,43,"已发现",false);
        else if(forge_has(&g->state,id))button(g,254,320,178,43,"丢弃遗物",false);
        else{snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"%u剑币购买"),r->cost);
            button(g,254,320,178,43,g->text_buffer,true);
            }return;
        }
    const char *title=g->modal==MODAL_SHORT?"贡品尚不足":g->modal==MODAL_TRADE?"把这把好剑换成剑币？":g->modal==MODAL_DISCARD?"确认丢弃遗物？":"重新开炉？";
    forge_text_fit(g,title,64,129,368,26,FORGE_GOLD);
    const char *detail=g->modal==MODAL_SHORT?"金币不会代替贡品。本次敲铃将增加1点怒气，第三次欠贡会结束契约。":g->modal==MODAL_TRADE?"高等级剑通常仍只换1枚剑币，此剑随后不能进贡。":g->modal==MODAL_DISCARD?"丢弃没有返还；已获得的签池改造会保留。":"当前契约进度会被替换，已发现图鉴和名匠解锁保留。";
    wrap(g,detail,57,193,370,21,4,FORGE_CREAM);
    button(g,48,308,178,48,"取消",false);
    button(g,254,308,178,48,"确认",true);
}
void forge_render(game_handle_t g)
{
    if(!g->assets_loaded){ClearBackground(FORGE_DARK);
        DrawText("ASSETS NOT READY",48,180,24,WHITE);
        return;
        }
    if(g->tribute.active){forge_tribute_draw(g);forge_language_button(g,344,25,48);forge_audio_button(g,400,25,44);return;}
    int dx=0,dy=0;
    if(g->state.phase==PHASE_PLAY&&g->screen==SCREEN_FORGE)forge_fx_camera(g,&dx,&dy);
    if(dx||dy){
        DrawRectangle(0,0,8,480,FORGE_DARK);DrawRectangle(472,0,8,480,FORGE_DARK);
        DrawRectangle(0,0,480,6,FORGE_DARK);DrawRectangle(0,474,480,6,FORGE_DARK);
    }
    DrawTexture(g->room.texture,dx,dy,WHITE);
    header(g);
    if(g->state.phase==PHASE_GIFT){gift(g);
        return;
        }
    if(g->state.phase==PHASE_CLEAR||g->state.phase==PHASE_LOST){DrawRectangle(0,55,480,425,(Color){15,11,9,218});
        forge_text_center(g,g->state.phase==PHASE_CLEAR?"契 约 完 成":"炉 火 暂 熄",20,110,440,34,FORGE_GOLD);
        snprintf(g->text_buffer,sizeof(g->text_buffer),forge_tr(g,"最高强化 +%u  落锤 %lu 次"),g->state.max_level,(unsigned long)g->state.hits);
        forge_text_center(g,g->text_buffer,30,185,420,23,FORGE_CREAM);
        forge_text(g,"最高剑值",82,237,22,FORGE_MUTED);
        amount(g,&g->state.peak,225,237,24,FORGE_GOLD);
        if(g->state.phase==PHASE_CLEAR)button(g,30,315,202,49,"继续无尽",true);
        button(g,249,315,202,49,"再开一炉",false);
        return;
        }
    if(g->screen==SCREEN_FORGE)forge_scene(g);
    else if(g->screen==SCREEN_RACK)inventory(g,false);
    else if(g->screen==SCREEN_TRIBUTE)inventory(g,true);
    else if(g->screen==SCREEN_RELICS)relics(g);
    else if(g->screen==SCREEN_MERCHANT)merchant(g);
    else if(g->screen==SCREEN_ITEMS)items(g);
    else book(g);
    navigation(g);
    if(g->paused){DrawRectangle(0,55,480,359,(Color){12,10,8,235});
        forge_text_center(g,"炉火已暂停",20,83,440,28,FORGE_GOLD);
        button(g,72,141,336,56,"继续锻造",true);
        button(g,72,221,336,56,"玩法手记",false);
        button(g,72,301,336,56,"重新开炉",false);
        button(g,72,371,228,32,game_is_muted(g)?"音效：已静音 · 点击开启":"音效：开启中 · 点击静音",game_is_muted(g));
        button(g,306,371,102,32,g->language==GAME_LANGUAGE_ZH?"English":"简体中文",true);
        }
    if(g->modal)modal(g);
}

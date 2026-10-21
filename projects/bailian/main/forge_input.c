// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "game_config.h"
void game_action(game_handle_t g,int action)
{
    if(!g)return;
    if(action==GAME_ACTION_MUTE){game_set_muted(g,!game_is_muted(g));return;}
    if(action==GAME_ACTION_LANGUAGE){game_set_language(g,game_get_language(g)==GAME_LANGUAGE_ZH?GAME_LANGUAGE_EN:GAME_LANGUAGE_ZH);return;}
    if(action==4){game_reset(g);
        return;
        }
    if(action==3){game_set_paused(g,!g->paused);
        return;
        }
    if(g->tribute.active)return;
    if(action>=510&&action<513){forge_choose_gift(g,action-510);
        return;
        }
    if(action==590){forge_continue(g);
        return;
        }
    if(g->state.phase!=PHASE_PLAY)return;
    if(action==501)forge_swing_request(g);
    else if(action==502){forge_swing_cancel(g,true);g->screen=SCREEN_RACK;
        g->page=0;
        g->undo_valid=false;
        }
    else if(action==503){forge_swing_cancel(g,true);g->screen=SCREEN_RELICS;
        g->page=0;
        g->undo_valid=false;
        }
    else if(action==504){forge_swing_cancel(g,true);g->screen=SCREEN_TRIBUTE;
        g->page=0;
        g->undo_valid=false;
        }
    else if(action==505)forge_tribute_start(g,false);
    else if(action>=520&&action<520+FORGE_BLADES){forge_swing_cancel(g,true);forge_select(g,action-520);}
    else if(action>=630&&action<630+ITEM_COUNT)forge_use_item(g,action-630);
    else if(action>=640&&action<643)forge_buy_relic(g,action-640);
}
static int list_index(game_handle_t g,unsigned row)
{
    unsigned n=0,target=g->page*6+row;
    for(unsigned i=0;i<FORGE_BLADES;i++){
        unsigned place=g->state.blades[i].place;
        if(!place)continue;
        if(g->screen==SCREEN_TRIBUTE?place!=PLACE_TRIBUTE:place==PLACE_TRIBUTE)continue;
        if(n++==target)return(int)i;
    }
    return-1;
}
static unsigned list_count(game_handle_t g)
{
    unsigned n=0;
    for(unsigned i=0;i<FORGE_BLADES;i++){unsigned p=g->state.blades[i].place;
        if(p&&(g->screen==SCREEN_TRIBUTE?p==PLACE_TRIBUTE:p!=PLACE_TRIBUTE))n++;
        }return n;
}
void forge_pointer(game_handle_t g,int x,int y)
{
    bool language_button=g->tribute.active?(x>=344&&x<392&&y>=25&&y<59):(x>=131&&x<177&&y>=10&&y<44);
    if(language_button){game_action(g,GAME_ACTION_LANGUAGE);return;}
    bool sound_button=g->tribute.active?(x>=400&&x<444&&y>=25&&y<59):(x>=91&&x<127&&y>=10&&y<44);
    if(sound_button){game_set_muted(g,!game_is_muted(g));return;}
    if(g->tribute.active){forge_tribute_pointer(g,x,y);return;}
    forge_state_t *s=&g->state;
    if(s->phase==PHASE_GIFT){if(y>=120&&y<394&&x>=12&&x<468)forge_choose_gift(g,(x-12)/156);
        return;
        }
    if(s->phase==PHASE_CLEAR||s->phase==PHASE_LOST){if(y>=310&&y<365){if(x<235&&s->phase==PHASE_CLEAR)forge_continue(g);
            else game_reset(g);
            }return;
        }
    if(g->modal){
        if(g->modal==MODAL_FAMILY){if(y>=143&&y<293){int col=x<240?0:1,row=(y-143)/50;
                g->detail_item=(uint8_t)(row*2+col+1);
                }else if(y>=350&&y<400){if(x>=240)forge_family(g,g->detail_item);g->modal=MODAL_NONE;}
            return;
            }
        if(g->modal==MODAL_RELIC){
            if(y>=316&&y<365){
                if(x<240){g->modal=MODAL_NONE;
                    return;
                    }
                int id=g->detail_relic;
                if(g->detail_readonly){g->modal=MODAL_NONE;return;}
                if(forge_has(s,id)){g->modal=MODAL_DISCARD;
                    return;
                    }
                for(int i=0;i<3;i++)if(s->offers[i]==id){forge_buy_relic(g,i);
                    break;
                    }
                g->modal=MODAL_NONE;
            }
            return;
        }
        if(g->modal==MODAL_ITEM||g->modal==MODAL_OFFER){
            if(y>=316&&y<365){
                if(x<240){g->modal=MODAL_NONE;return;}
                bool done=g->modal==MODAL_ITEM?forge_use_item(g,g->detail_item):forge_buy_merchant(g,g->detail_offer);
                if(done){if(g->modal==MODAL_ITEM)g->screen=SCREEN_FORGE;g->modal=MODAL_NONE;}
            }
            return;
        }
        if(y>=300&&y<365){
            int modal=g->modal;
            g->modal=MODAL_NONE;
            if(x>=240){if(modal==MODAL_SHORT)forge_tribute_start(g,true);
                else if(modal==MODAL_NEW)game_reset(g);
                else if(modal==MODAL_TRADE)forge_trade(g,g->selected);
                else if(modal==MODAL_DISCARD)forge_discard(g,g->detail_relic);
                }
        }
        return;
    }
    if(g->paused){if(y>=140&&y<200){g->paused=false;
            g->screen=SCREEN_FORGE;
            }else if(y>=220&&y<280){g->paused=false;
                g->screen=SCREEN_BOOK;
                g->book_tab=0;g->page=0;
            }else if(y>=300&&y<360){g->modal=MODAL_NEW;
            g->paused=false;
            }else if(y>=371&&y<404){
                if(x>=72&&x<300)game_set_muted(g,!game_is_muted(g));
                else if(x>=306&&x<408)game_action(g,GAME_ACTION_LANGUAGE);
            }
        return;
        }
    if(y>=414){unsigned tab=(unsigned)x/120;
        if(tab==g->screen)return;
        bool bag_return=g->screen==SCREEN_ITEMS&&tab==SCREEN_FORGE;
        forge_swing_cancel(g,tab!=g->screen);
        g->screen=(uint8_t)tab;
        g->page=0;
        g->selected=255;
        if(!bag_return)g->undo_valid=false;
        return;
        }
    if(y<52){if(x<90){game_set_paused(g,true);
            return;
            }if(x>360){forge_swing_cancel(g,true);g->screen=SCREEN_ITEMS;
            return;
            }}
    if(g->screen==SCREEN_FORGE){
        if(y>=337&&y<378){if(x<224){forge_swing_cancel(g,true);if(s->active>=0)forge_move(g,s->active,true);
                else g->screen=SCREEN_RACK;
                }else if(x>256)forge_rewind(g);
            return;
            }
        if(y>=95&&y<330){forge_blade_t *b=forge_active(g);
            if(x>393&&y<153&&b&&!b->family&&b->level>=6){forge_swing_cancel(g,true);g->detail_item=1;g->modal=MODAL_FAMILY;}
            else forge_swing_request(g);
            }
        return;
    }
    if(g->screen==SCREEN_RACK||g->screen==SCREEN_TRIBUTE){
        if(y>=83&&y<122){if(x<100&&g->page)g->page--;
            else if(x>380&&((unsigned)g->page+1)*6<list_count(g))g->page++;
            return;
            }
        if(y>=130&&y<352){unsigned row=(unsigned)(y-130)/74,col=(unsigned)x/240;
            int index=list_index(g,row*2+col);
            if(index>=0)g->selected=(uint8_t)index;
            return;
            }
        if(y>=360&&y<406){
            if(g->screen==SCREEN_TRIBUTE){if(x<220&&g->selected<FORGE_BLADES)forge_move(g,g->selected,false);
                else if(x>=240)forge_tribute_start(g,false);
                return;
                }
            if(g->selected>=FORGE_BLADES){forge_notice(g,"先点击要操作的剑");
                return;
                }
            if(x<160)forge_select(g,g->selected);
            else if(x<320)forge_move(g,g->selected,true);
            else if(s->blades[g->selected].level>=4)g->modal=MODAL_TRADE;
            else forge_trade(g,g->selected);
        }
        return;
    }
    if(g->screen==SCREEN_RELICS){
        if(y>=82&&y<123){if(x<160)g->shop_page=0;
            else if(x<320)g->shop_page=1;
            else forge_reroll(g);
            return;
            }
        if(y>=128&&y<335){
            if(!g->shop_page){int slot=x/160;
                if(slot<3&&s->offers[slot]>=0){g->detail_relic=s->offers[slot];
                    g->detail_readonly=false;
                    g->modal=MODAL_RELIC;
                    }}
            else{int col=x/120,row=(y-128)/103,index=row*4+col;
                if(index<s->owned_count){g->detail_relic=s->owned[index];
                    g->detail_readonly=false;
                    g->modal=MODAL_RELIC;
                    }}
        }
        if(y>=355&&y<405){if(x<240){g->screen=SCREEN_MERCHANT;
                g->page=0;
                }else g->screen=SCREEN_ITEMS;
            }
        return;
    }
    if(g->screen==SCREEN_MERCHANT){
        if(y>=82&&y<123){if(x<100&&g->page)g->page--;
            else if(x>380&&(g->page+1)*4<s->merchant_count)g->page++;
            return;
            }
        if(y>=130&&y<370){unsigned index=g->page*4+(unsigned)(y-130)/60;
            if(index<s->merchant_count){g->detail_offer=(uint8_t)index;g->modal=MODAL_OFFER;}
            }
        if(y>=375){g->screen=SCREEN_RELICS;
            g->page=0;
            }
        return;
    }
    if(g->screen==SCREEN_ITEMS){if(y>=104&&y<404){unsigned item=(unsigned)(y-104)/60;
            if(item<5){g->detail_item=(uint8_t)item;g->modal=MODAL_ITEM;}
            }return;
        }
    if(g->screen==SCREEN_BOOK){
        if(y>=84&&y<123){g->book_tab=(uint8_t)(x/160);g->page=0;return;}
        if(g->book_tab==2){
            if(y>=127&&y<165){if(x<100&&g->page)g->page--;else if(x>380&&(g->page+1)*6<94)g->page++;return;}
            if(y>=170&&y<392){unsigned id=g->page*6+(unsigned)(y-170)/74*2+(unsigned)x/240;
                if(id<94&&(s->found[id/64]&(UINT64_C(1)<<(id%64)))){
                    g->detail_relic=(int16_t)id;g->detail_readonly=true;g->modal=MODAL_RELIC;
                }
            }
        }
    }
}

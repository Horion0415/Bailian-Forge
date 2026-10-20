// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static bool bit(const uint64_t bits[2],int id){return id>=0&&id<R_COUNT&&(bits[id/64]&(UINT64_C(1)<<(id%64)));
    }
static void setbit(uint64_t bits[2],int id){bits[id/64]|=UINT64_C(1)<<(id%64);
    }
bool forge_has(const forge_state_t *s,int id){for(unsigned i=0;i<s->owned_count;i++)if(s->owned[i]==id)return true;
    return false;
    }
forge_blade_t *forge_active(game_handle_t g){return g->state.active>=0&&g->state.active<FORGE_BLADES?&g->state.blades[g->state.active]:NULL;
    }
uint32_t forge_random(forge_state_t *s){uint32_t t=(s->rng+=UINT32_C(0x6d2b79f5));
    t=(t^(t>>15))*(t|1U);
    t^=t+(t^(t>>7))*(t|61U);
    return t^(t>>14);
    }
static bool chance(forge_state_t *s,unsigned per100){return forge_random(s)%100U<per100;
    }
void forge_notice(game_handle_t g,const char *text){snprintf(g->notice,sizeof(g->notice),"%s",forge_tr(g,text));
    g->notice_ms=4500;
    }
void forge_changed(game_handle_t g,bool invalidate){g->state.revision++;
    if(invalidate)g->undo_valid=false;
    }
static void add_small(forge_money_t *m,unsigned n){forge_money_t a=forge_money_u64(n);
    forge_money_add(m,&a);
    }
static void credit(forge_state_t *s,const forge_money_t *n){forge_money_add(&s->coins,n);
    forge_money_add(&s->earned,n);
    }
static forge_money_t base_part(const forge_state_t *s,const forge_blade_t *b){
    forge_money_t n=b->base;
    if(forge_has(s,R_CHARCOAL))add_small(&n,80);
    if(forge_has(s,R_HEARTH))add_small(&n,s->paid*20);
    for(unsigned i=0;i<b->level;i++)forge_money_small(&n,3);
    if(b->level){forge_money_t half=forge_money_u64(1);
        for(unsigned i=1;i<b->level;i++)forge_money_small(&half,2);
        forge_money_add(&n,&half);
        }
    for(unsigned i=0;i<b->level;i++)forge_money_div(&n,2);
    return n;
}
forge_money_t forge_value(const forge_state_t *s,const forge_blade_t *b){
    if(!b||b->place==PLACE_FREE)return forge_money_u64(0);
    forge_money_t v=base_part(s,b);
    forge_money_add(&v,&b->flat);
    forge_money_mul(&v,&b->perm);
    add_small(&v,FORGE_SCALE/2);
    forge_money_div(&v,FORGE_SCALE);
    if(forge_has(s,R_SIGNET))forge_money_scale(&v,115,100);
    if(forge_has(s,R_THORN))forge_money_scale(&v,110,100);
    if(b->level>=8&&forge_has(s,R_IRONSTONE))forge_money_scale(&v,130,100);
    if(b->level>=12&&forge_has(s,R_DRAGONSIGNET))forge_money_scale(&v,150,100);
    if(b->family==4&&b->level>=10)forge_money_scale(&v,110,100);
    return v;
}
forge_money_t forge_base_quota(unsigned day){forge_money_t q=forge_money_u64(forge_quotas[day<=21?day-1:20]);
    for(unsigned i=21;i<day;i++)forge_money_scale(&q,6,5);
    return q;
    }
forge_money_t forge_quota(const forge_state_t *s){forge_money_t q=forge_base_quota(s->day);
    if(forge_has(s,R_REBATE))forge_money_scale(&q,9,10);
    forge_money_sub(&q,&s->quota_credit);
    return q;
    }
forge_money_t forge_tribute(const forge_state_t *s){forge_money_t q=s->scrap;
    for(unsigned i=0;i<FORGE_BLADES;i++)if(s->blades[i].place==PLACE_TRIBUTE){forge_money_t v=forge_value(s,&s->blades[i]);
        forge_money_add(&q,&v);
        }return q;
    }
static int new_blade(forge_state_t *s,int family,unsigned level){
    for(int i=0;i<FORGE_BLADES;i++)if(s->blades[i].place==PLACE_FREE){
        forge_blade_t *b=&s->blades[i];
        memset(b,0,sizeof(*b));
        b->id=s->next_id++;
        b->family=(uint8_t)family;
        b->place=PLACE_RACK;
        b->level=(uint8_t)level;
        b->base=forge_money_u64(family>=7?640:320);
        b->perm=forge_money_u64(FORGE_SCALE);
        b->heated=forge_has(s,R_LIFETIME);
        b->last=RESULT_NONE;
        b->peak=forge_value(s,b);
        s->frenzy_energy[i]=0;s->frenzy_fraction[i]=0;
        return i;
    }
    return -1;
}
static void supply(forge_state_t *s,unsigned count,unsigned min,unsigned first){
    for(unsigned i=0;i<count;i++){
        unsigned level=i==0&&first>min?first:min;
        if(new_blade(s,0,level)<0&&s->backlog<UINT16_MAX){s->pending_levels[level]++;s->backlog++;}
    }
}
void forge_claim_supply(game_handle_t g){
    forge_state_t *s=&g->state;
    if(!s->backlog)return;
    /* Deliver inherited/high-level supplies first; never lose their level. */
    for(int level=99;level>=0;level--)while(s->pending_levels[level]){
        if(new_blade(s,0,(unsigned)level)<0)return;
        s->pending_levels[level]--;s->backlog--;forge_changed(g,false);
    }
    }
static unsigned blade_count(const forge_state_t *s){unsigned n=0;
    for(unsigned i=0;i<FORGE_BLADES;i++)if(s->blades[i].place)n++;
    return n;
    }
static void add_ticket(forge_state_t *s,int i,int n){int v=(int)s->deck[i]+n;
    s->deck[i]=(uint16_t)(v<0?0:v>60000?60000:v);
    }
static void convert(forge_state_t *s,int from,int to,unsigned count){if(count>s->deck[from])count=s->deck[from];
    add_ticket(s,from,-(int)count);
    add_ticket(s,to,(int)count);
    }
static void acquire(game_handle_t g,int id){
    forge_state_t *s=&g->state;
    s->owned[s->owned_count++]=(int8_t)id;
    setbit(s->acquired,id);
    setbit(s->found,id);
    if(id==R_NAIL)add_ticket(s,3,4);
    if(id==R_LUCKY)add_ticket(s,4,1);
    if(id==R_PATIENT)add_ticket(s,2,3);
    if(id==R_SEALING)add_ticket(s,2,2);
    if(id==R_MALLET)convert(s,1,3,1);
    if(id==R_WHETSTONE)convert(s,2,3,2);
    if(id==R_THORN)convert(s,2,1,2);
    if(id==R_HOOP)add_ticket(s,0,-2);
    if(id==R_GOLDNAIL)add_ticket(s,4,2);
    if(id==R_SPIKE)add_ticket(s,3,6);
    if(id==R_BUTTER){add_ticket(s,0,3);
        add_ticket(s,4,5);
        }if(id==R_SICKLE)for(unsigned i=0;i<5;i++)s->deck[i]=5;
    if(id==R_PEARL)add_ticket(s,1,3);
    if(id==R_SEED)add_ticket(s,2,6);
    if(id==R_ORACLE)add_ticket(s,4,4);
    snprintf(g->notice,sizeof(g->notice),forge_tr(g,"获得：%s"),forge_tr(g,forge_relics[id].name));
    g->notice_ms=4000;
    g->feedback=FB_SELECT;
}
void forge_new(game_handle_t g,uint32_t seed,bool preserve){
    uint64_t found0=preserve?g->state.found[0]:0,found1=preserve?g->state.found[1]:0;
    uint16_t unlocks=preserve?g->state.unlocks:0,achievements=preserve?g->state.achievements:0;
    uint32_t revision=g->state.revision+1;
    memset(&g->state,0,sizeof(g->state));
    forge_state_t *s=&g->state;
    s->rng=seed;
    s->next_id=1;
    s->day=1;
    s->active=-1;
    s->phase=PHASE_GIFT;
    s->revision=revision;
    s->found[0]=found0;
    s->found[1]=found1;
    s->unlocks=unlocks;
    s->achievements=achievements;
    memset(s->owned,-1,sizeof(s->owned));
    memset(s->offers,-1,sizeof(s->offers));
    const uint16_t deck[5]={9,2,5,12,2};
    memcpy(s->deck,deck,sizeof(deck));
    supply(s,3,0,0);
    s->active=0;
    s->blades[0].place=PLACE_ANVIL;
    for(unsigned i=0;i<3;i++){int id;
        bool duplicate;
        do{id=forge_starters[forge_random(s)%8];
            duplicate=false;
            for(unsigned j=0;j<i;j++)if(s->offers[j]==id)duplicate=true;
            }while(duplicate);
        s->offers[i]=(int8_t)id;
        }
    g->screen=SCREEN_FORGE;
    g->page=0;
    g->modal=MODAL_NONE;
    g->undo_valid=false;
    g->heat_ms=0;
    g->cooldown_ms=0;
    g->pending_strike=false;
    g->animation_ms=0;
    g->last_result=RESULT_NONE;
    forge_notice(g,"开炉赠礼：选一件遗物，开始21日契约");
}
bool forge_choose_gift(game_handle_t g,int slot){if(g->state.phase!=PHASE_GIFT||slot<0||slot>=3||g->state.offers[slot]<0)return false;
    acquire(g,g->state.offers[slot]);
    memset(g->state.offers,-1,3);
    g->state.phase=PHASE_PLAY;
    forge_changed(g,true);
    return true;
    }
bool forge_select(game_handle_t g,int index){
    forge_state_t *s=&g->state;
    if(s->phase!=PHASE_PLAY||index<0||index>=FORGE_BLADES||!s->blades[index].place||s->blades[index].place==PLACE_TRIBUTE)return false;
    forge_blade_t *old=forge_active(g);
    if(old)old->place=PLACE_RACK;
    s->active=(int16_t)index;
    forge_blade_t *b=&s->blades[index];
    b->place=PLACE_ANVIL;
    if(!b->hits&&!b->level){b->level=(uint8_t)s->inherit_level;
        b->charge=(uint8_t)s->inherit_charge;
        b->success_tickets=s->inherit_molar;
        s->inherit_level=0;
        s->inherit_charge=0;
        s->inherit_molar=0;
        }
    g->heat_ms=0;
    g->pending_strike=false;
    forge_changed(g,true);
    g->screen=SCREEN_FORGE;
    g->feedback=FB_SELECT;
    return true;
}
bool forge_move(game_handle_t g,int index,bool put){
    forge_state_t *s=&g->state;
    if(s->phase!=PHASE_PLAY||index<0||index>=FORGE_BLADES||!s->blades[index].place)return false;
    forge_blade_t *b=&s->blades[index];
    if((put&&b->place==PLACE_TRIBUTE)||(!put&&b->place!=PLACE_TRIBUTE))return false;
    b->place=put?PLACE_TRIBUTE:PLACE_RACK;
    if(s->active==index)s->active=-1;
    forge_claim_supply(g);
    g->pending_strike=false;
    g->heat_ms=0;
    forge_changed(g,true);
    g->feedback=FB_SELECT;
    g->selected=255;
    forge_notice(g,put?"已摆上贡台；敲铃前仍可取回":"已从贡台取回剑架");
    return true;
}
bool forge_trade(game_handle_t g,int index){
    forge_state_t *s=&g->state;
    if(index<0||index>=FORGE_BLADES||!s->blades[index].place||s->blades[index].place==PLACE_TRIBUTE||s->phase!=PHASE_PLAY)return false;
    forge_blade_t *b=&s->blades[index];
    unsigned amount=1;
    if(forge_has(s,R_HOOK)&&s->rerolls&&!s->hook_used&&!b->hits&&!b->level){amount=2;
        s->hook_used=true;
        }
    s->credits=(uint16_t)(s->credits+amount);
    b->place=PLACE_FREE;
    if(s->active==index)s->active=-1;
    forge_claim_supply(g);
    forge_changed(g,true);
    g->selected=255;
    forge_notice(g,amount==2?"收藏家之钩：换得2枚剑币":"换得1枚剑币；此剑不能再进贡");
    g->feedback=FB_SELECT;
    return true;
}
void forge_tickets(const forge_state_t *s,const forge_blade_t *b,uint16_t out[5]){
    if(!b){memset(out,0,5*sizeof(*out));
        return;
        }
    double d[5];
    for(unsigned i=0;i<5;i++)d[i]=s->deck[i];
    d[3]+=b->success_tickets;
    d[4]+=b->great_tickets+s->one_great;
    if(b->family==2)d[4]+=1;
    if(b->family==3&&b->last==RESULT_DAMAGE)d[3]+=3;
    if(forge_has(s,R_BLIND)&&(b->last==RESULT_DAMAGE||b->last==RESULT_HOLD))d[4]+=3;
    if(forge_has(s,R_EYE)&&b->last==RESULT_HOLD)d[3]+=4;
    if(forge_has(s,R_OBSIDIAN)&&b->level>=8){double n=d[3]<2?d[3]:2;
        d[3]-=n;
        d[4]+=n;
        }
    const bool frenzy=b->frenzy_ms>0||b->charge>=10;
    if(forge_has(s,R_CORE)&&frenzy)d[4]+=6;
    if(s->chalk)d[4]*=2;
    double q=forge_break_permille[b->level<30?b->level:30]/1000.0;
    d[0]*=q>0?(21*q/(1-q))/9:0;
    double total=0;
    for(unsigned i=0;i<5;i++)total+=d[i];
    if(total<=0){d[2]=1;
        total=1;
        }for(unsigned i=0;i<5;i++)d[i]/=total;
    double factor=1;
    if(b->family==1)factor*=.9;
    if(b->family==6)factor*=1.1;
    if(forge_has(s,R_KNOT))factor*=.9;
    if(forge_has(s,R_TONGS)&&b->level>=6)factor*=.75;
    if(forge_has(s,R_SPLICE))factor*=1-fmin(.6,b->scars*.15);
    if(forge_has(s,R_CHARM))factor*=1-fmin(.18,b->charm*.03);
    if(forge_has(s,R_THREAD)&&b->last==RESULT_HOLD)factor*=.8;
    if(forge_has(s,R_LIGHTNING)&&b->last==RESULT_GREAT)factor*=.6;
    double non=1-d[0],new_break=fmin(.99,d[0]*factor);
    if(non>0)for(unsigned i=1;i<5;i++)d[i]*=(1-new_break)/non;
    d[0]=new_break;
    if(s->powder){d[0]+=d[1]+d[2];
        d[4]+=d[3];
        d[1]=d[2]=d[3]=0;
        }
    if(frenzy){non=1-d[0];
        d[0]=0;
        if(non>0)for(unsigned i=1;i<5;i++)d[i]/=non;
        else d[2]=1;
        }
    double rem[5];
    unsigned count=0;
    for(unsigned i=0;i<5;i++){double x=d[i]*1000;
        out[i]=(uint16_t)x;
        rem[i]=x-out[i];
        count+=out[i];
        }
    while(count<1000){unsigned best=0;
        for(unsigned i=1;i<5;i++)if(rem[i]>rem[best])best=i;
        out[best]++;
        rem[best]=-1;
        count++;
        }
}
static bool eligible(const forge_state_t *s,int id){const forge_relic_def_t *r=&forge_relics[id];
    return !bit(s->acquired,id)&&r->day<=s->day&&(!r->extension||s->expanded)&&(!r->unlock||(s->unlocks&r->unlock));
    }
unsigned forge_energy_percent(const forge_state_t *s,const forge_blade_t *b){
    if(!b)return 0;
    unsigned energy=s->frenzy_energy[b-s->blades];
    return energy>=FORGE_ENERGY_COST?100:energy*100/FORGE_ENERGY_COST;
}
void forge_energy_update(forge_state_t *s,forge_blade_t *b,unsigned ms){
    size_t index=(size_t)(b-s->blades);
    unsigned speed=100+(forge_has(s,R_GRIP)?15:0)+(forge_has(s,R_BELLOWS)?30:0)+(b->family==5?10:0);
    uint32_t amount=ms*speed*FORGE_ENERGY_UNIT+s->frenzy_fraction[index];
    unsigned energy=s->frenzy_energy[index]+amount/100;
    s->frenzy_fraction[index]=(uint8_t)(amount%100);
    if(energy>=FORGE_ENERGY_MAX){energy=FORGE_ENERGY_MAX;s->frenzy_fraction[index]=0;}
    s->frenzy_energy[index]=(uint16_t)energy;
}
static void shop_roll(forge_state_t *s){
    memset(s->offers,-1,3);
    unsigned common=s->day<4?85:s->day<6?75:s->day==6?70:s->day<9?60:s->day<13?53:s->day<17?36:28;
    unsigned rare=s->day<6?100-common:s->day==6?25:s->day<13?35:42;
    for(unsigned slot=0;slot<3;slot++){
        unsigned roll=forge_random(s)%100,tier=roll<common?0:roll<common+rare?1:2;
        for(unsigned pass=0;pass<2;pass++){
            unsigned count=0;
            int chosen=-1;
            for(int id=0;id<R_COUNT;id++)if(eligible(s,id)&&(pass||forge_relics[id].rarity==tier)){
                bool dup=false;
                for(unsigned k=0;k<slot;k++)if(s->offers[k]==id)dup=true;
                if(dup)continue;
                unsigned weight=s->day<=6&&(id==R_KNOT||id==R_HOOP||id==R_CHALICE||id==R_WIRE||id==R_SPLICE)?2:1;
                for(unsigned w=0;w<weight;w++){count++;
                    if(forge_random(s)%count==0)chosen=id;
                    }
            }
            if(chosen>=0){s->offers[slot]=(int8_t)chosen;
                break;
                }
        }
    }
}
forge_money_t forge_reroll_price(const forge_state_t *s){forge_money_t p=forge_base_quota(s->day);
    if(forge_has(s,R_VOUCHER)&&!s->rerolls)return forge_money_u64(0);
    forge_money_scale(&p,10+s->rerolls*5,100);
    return p;
    }
bool forge_reroll(game_handle_t g){
    forge_state_t *s=&g->state;
    if(s->phase!=PHASE_PLAY||s->day==1){forge_notice(g,"首日赠礼不能刷新；次日遗物机营业");
        return false;
        }
    forge_money_t p=forge_reroll_price(s);
    if(forge_money_compare(&s->coins,&p)<0){forge_notice(g,"金币不足，无法刷新");
        return false;
        }
    forge_money_sub(&s->coins,&p);
    s->rerolls++;
    shop_roll(s);
    forge_changed(g,true);
    forge_notice(g,"遗物机已刷新");
    return true;
}
bool forge_buy_relic(game_handle_t g,int slot){
    forge_state_t *s=&g->state;
    if(s->phase!=PHASE_PLAY||s->day==1||slot<0||slot>=3||s->offers[slot]<0)return false;
    int id=s->offers[slot];
    if(s->owned_count>=8){forge_notice(g,"遗物架已满8格，请先丢弃一件");
        return false;
        }
    if(s->credits<forge_relics[id].cost){forge_notice(g,"剑币不足；在剑架可用备用剑换币");
        return false;
        }
    s->credits-=forge_relics[id].cost;
    acquire(g,id);
    s->offers[slot]=-1;
    forge_changed(g,true);
    return true;
}
bool forge_discard(game_handle_t g,int id){
    forge_state_t *s=&g->state;
    for(unsigned i=0;i<s->owned_count;i++)if(s->owned[i]==id){for(unsigned j=i+1;j<s->owned_count;j++)s->owned[j-1]=s->owned[j];
        s->owned[--s->owned_count]=-1;
        forge_changed(g,true);
        forge_notice(g,"遗物已丢弃；已取得的签池改造保留");
        return true;
        }return false;
}
static const unsigned item_ratios[ITEM_COUNT]={42,153,56,90,70,216,91};
forge_money_t forge_merchant_price(const forge_state_t *s,unsigned slot){
    if(slot>=s->merchant_count)return forge_money_u64(0);
    const forge_offer_t *o=&s->merchant[slot];
    forge_money_t p=forge_base_quota(s->day);
    if(o->kind==0){forge_money_scale(&p,40,100);
        forge_money_t min=forge_money_u64(700);
        if(forge_money_compare(&p,&min)<0)p=min;
        }
    else if(o->kind==1){forge_money_scale(&p,11375,10000);
        for(unsigned i=0;i<o->level;i++)forge_money_scale(&p,3,2);
        }
    else{forge_money_scale(&p,item_ratios[o->id],100);
        add_small(&p,5);
        forge_money_div(&p,10);
        forge_money_small(&p,10);
        }
    for(unsigned i=0;i<o->bought;i++)forge_money_small(&p,3);
    return p;
}
static void merchant_stock(forge_state_t *s){
    s->merchant_count=0;
    if(s->day%3)return;
    unsigned visit=s->day/3,n=(visit==1?2:visit<=3?3:visit<=7?4:5)+(forge_has(s,R_SUPPLYWRIT)?2:0);
    s->merchant[s->merchant_count++]=(forge_offer_t){.kind=0,.stock=(uint8_t)n};
    s->merchant[s->merchant_count++]=(forge_offer_t){.kind=1,.stock=1,.family=(uint8_t)(7+forge_random(s)%3),.level=s->day<9?0:s->day<15?2:4};
    unsigned count=s->day<6?2:s->day<15?3:4;
    if(forge_has(s,R_LEDGER))count+=2;
    bool picked[ITEM_COUNT]={0};
    for(unsigned i=0;i<count;i++){
        unsigned id=i;
        if(s->day>3){unsigned valid=0;
            for(unsigned k=0;k<ITEM_COUNT;k++)if(!picked[k]&&(k<2||s->day>=6)&&(k!=ITEM_INCENSE||(s->day>=9&&s->anger&&!s->incense_used))){valid++;
                if(forge_random(s)%valid==0)id=k;
                }if(!valid)break;
            }
        picked[id]=true;
        s->merchant[s->merchant_count++]=(forge_offer_t){.kind=2,.id=(uint8_t)id,.stock=(id==ITEM_CLOCK||id==ITEM_POWDER||id==ITEM_INCENSE)?1:3};
    }
}
bool forge_buy_merchant(game_handle_t g,unsigned slot){
    forge_state_t *s=&g->state;
    if(slot>=s->merchant_count||!s->merchant[slot].stock)return false;
    forge_offer_t *o=&s->merchant[slot];
    if(o->kind<2&&blade_count(s)>=FORGE_BLADES){forge_notice(g,"剑架已满，请先处理库存");
        return false;
        }
    if(o->kind==2&&o->id==ITEM_INCENSE&&(!s->anger||s->incense_used))return false;
    forge_money_t p=forge_merchant_price(s,slot);
    if(forge_money_compare(&s->coins,&p)<0){forge_notice(g,"金币不足");
        return false;
        }
    forge_money_sub(&s->coins,&p);
    o->stock--;
    o->bought++;
    if(o->kind<2)new_blade(s,o->family,o->level);
    else if(o->id==ITEM_INCENSE){s->anger--;
        s->incense_used=true;
        }
    else if(o->id==ITEM_DELIVERY)s->extra_supply+=2;
    else s->items[o->id]++;
    forge_changed(g,true);
    forge_notice(g,"购买成功，物品已放入背包或剑架");
    g->feedback=FB_SELECT;
    return true;
}
bool forge_use_item(game_handle_t g,int item){
    forge_state_t *s=&g->state;
    if(item==ITEM_CLOCK)return forge_rewind(g);
    forge_blade_t *b=forge_active(g);
    if(!b||item<0||item>=ITEM_COUNT||!s->items[item]){forge_notice(g,"先选择一把剑，并确认背包中有此物品");
        return false;
        }
    if((b->frenzy_ms>0||b->charge>=10)&&(item==ITEM_SCALE||item==ITEM_POWDER||item==ITEM_STAR)){forge_notice(g,"入神或充能已满时不能使用此物品");
        return false;
        }
    if((item==ITEM_SCALE&&s->scale)||(item==ITEM_CHALK&&s->chalk)||(item==ITEM_POWDER&&s->powder))return false;
    if(item==ITEM_STAR)b->charge=(uint8_t)(b->charge+3>10?10:b->charge+3);
    else if(item==ITEM_SCALE)s->scale=true;
    else if(item==ITEM_CHALK)s->chalk=true;
    else if(item==ITEM_POWDER)s->powder=true;
    else return false;
    s->items[item]--;
    forge_changed(g,true);
    forge_notice(g,"物品效果已准备");
    return true;
}
bool forge_rewind(game_handle_t g){
    if(!g->undo_valid||!g->state.items[ITEM_CLOCK]){forge_notice(g,"需要悔恨发条，且尚未离开上一锤的铁砧");
        return false;
        }
    uint16_t left=g->state.items[ITEM_CLOCK]-1;
    uint32_t rng=g->state.rng,rev=g->state.revision+1;
    memcpy(&g->state,g->undo,sizeof(g->state));
    g->state.items[ITEM_CLOCK]=left;
    g->state.rng=rng;
    g->state.revision=rev;
    g->undo_valid=false;
    g->pending_strike=false;
    g->cooldown_ms=0;
    g->animation_ms=0;
    g->last_result=RESULT_NONE;
    forge_notice(g,"回溯上一锤，消耗1份悔恨发条");
    g->feedback=FB_SELECT;
    return true;
}
void forge_remember(game_handle_t g,forge_blade_t *b){
    forge_state_t *s=&g->state;
    if(forge_has(s,R_IDOL))while(b->milestone<b->level/4){
        forge_money_scale(&b->perm,115,100);b->milestone++;
    }
    forge_money_t v=forge_value(s,b);
    if(forge_money_compare(&v,&b->peak)>0)b->peak=v;
    if(forge_money_compare(&v,&s->peak)>0)s->peak=v;
    if(b->level>s->max_level)s->max_level=b->level;
    if(b->level>=20&&b->family){uint16_t bitmask=(uint16_t)(1U<<b->family);
        if(!(s->unlocks&bitmask)){s->unlocks|=bitmask;
            forge_notice(g,"名匠解锁：该武器系遗物加入后续商店");
            }}
    if(b->level==99)s->achievements|=1;
}
static void broken(game_handle_t g,forge_blade_t *b){
    forge_state_t *s=&g->state;
    s->breaks++;
    if(forge_has(s,R_ASH)){forge_money_t p=forge_money_u64(500);
        credit(s,&p);
        }
    if(forge_has(s,R_CHALICE)){forge_money_t p=b->peak;
        forge_money_scale(&p,3,10);
        forge_money_add(&s->scrap,&p);
        }
    if(forge_has(s,R_X_INSURANCE)){forge_money_t p=b->peak;
        forge_money_scale(&p,1,5);
        credit(s,&p);
        }
    if(b->family==9){forge_money_t p=b->peak;forge_money_scale(&p,1,10);forge_money_add(&s->scrap,&p);}
    if(forge_has(s,R_HOLY)||forge_has(s,R_DAWN)){unsigned percent=(forge_has(s,R_HOLY)?30:0)+(forge_has(s,R_DAWN)?40:0);
        s->inherit_level=b->level*percent/100;
        s->inherit_charge=(forge_has(s,R_HOLY)?3:0)+(forge_has(s,R_DAWN)?4:0);
        }
    if(forge_has(s,R_MOLAR)&&b->level>=4)s->inherit_molar=4;
    if(forge_has(s,R_GRUDGE))s->one_great=b->level;
    if(forge_has(s,R_OSSUARY)&&b->level>=7&&b->last<5)add_ticket(s,b->last,1);
    if(forge_has(s,R_UNDERTAKER)&&b->level>=7&&s->undertaker_count<2){s->extra_supply++;
        s->undertaker_count++;
        }
    g->visual_family=b->family;
    b->place=PLACE_FREE;
    s->active=-1;
    forge_notice(g,"破碎！这把剑化作碎铁");
    g->feedback=FB_BREAK;
    forge_particles(g,RESULT_BREAK);
}
typedef struct {
    forge_money_t *target;
    forge_money_t amount;
    uint32_t numerator,denominator;
} value_event_t;
static void value_add(value_event_t *event,forge_money_t *target,const forge_money_t *amount){
    forge_money_add(target,amount);event->target=target;event->amount=*amount;event->denominator=0;
}
static void value_small(value_event_t *event,forge_money_t *target,unsigned amount){
    forge_money_t value=forge_money_u64(amount);value_add(event,target,&value);
}
static void value_scale(value_event_t *event,forge_money_t *target,uint32_t numerator,uint32_t denominator){
    forge_money_scale(target,numerator,denominator);event->target=target;
    event->numerator=numerator;event->denominator=denominator;
}
bool forge_strike(game_handle_t g,int forced){
    forge_state_t *s=&g->state;
    forge_blade_t *b=forge_active(g);
    if(s->phase!=PHASE_PLAY||!b||b->level>=99||!b->heated)return false;
    memcpy(g->undo,s,sizeof(*s));
    g->undo_valid=true;
    size_t index=(size_t)(b-s->blades);
    if(forced<0&&b->frenzy_ms>0&&s->frenzy_energy[index]<FORGE_ENERGY_COST){
        /* Every fast contact earns value; only charged contacts run relic and
         * level chains. Refine does not consume tickets, streaks or items. */
        forge_money_t extra=base_part(s,b);forge_money_div(&extra,500);
        if(forge_money_zero(&extra))extra=forge_money_u64(1);
        forge_money_add(&b->flat,&extra);s->hits++;
        g->last_result=RESULT_REFINE;g->feedback=FB_HIT;
        forge_remember(g,b);forge_particles(g,RESULT_REFINE);forge_changed(g,false);
        forge_notice(g,"连击精锻增值 · 蓄能满后触发完整强化");return true;
    }
    if(b->frenzy_ms>0&&s->frenzy_energy[index]>=FORGE_ENERGY_COST)s->frenzy_energy[index]-=FORGE_ENERGY_COST;
    forge_money_t old_part=base_part(s,b);
    unsigned old_level=b->level;
    bool first=!b->hits;
    value_event_t last_value={0};
    uint16_t tickets[5];
    forge_tickets(s,b,tickets);
    unsigned roll=forge_random(s)%1000,result=4;
    for(unsigned i=0;i<5;i++){if(roll<tickets[i]){result=i;
            break;
            }roll-=tickets[i];
        }
    if(forced>=0&&forced<5)result=(unsigned)forced;
    if(!b->frenzy_ms&&b->charge>=10){b->charge=0;
        s->frenzy_energy[index]=0;s->frenzy_fraction[index]=0;
        b->frenzy_ms=(int16_t)(4000+(forge_has(s,R_HOURGLASS)?1000:0)+(forge_has(s,R_WICK)?2000:0));
        }
    bool inside=b->frenzy_ms>0,spine_protected=false;
    if(result==RESULT_BREAK){
        if(s->scale)result=RESULT_HOLD;
        else if(forge_has(s,R_WIRE)&&!s->wire_used){s->wire_used=true;
            result=RESULT_DAMAGE;
            }
        else if(forge_has(s,R_CORNER)&&!b->corner_used){b->corner_used=true;
            result=RESULT_DAMAGE;
            }
        else if(forge_has(s,R_SPINE)&&b->spine_ready){b->spine_ready=false;
            spine_protected=true;
            result=RESULT_DAMAGE;
            }
        else if(forge_has(s,R_PHOENIX)&&!b->phoenix_used){b->phoenix_used=true;
            forge_money_scale(&b->perm,3,5);
            result=RESULT_HOLD;
            }
    }
    s->scale=s->chalk=s->powder=false;
    s->one_great=0;
    s->hits++;
    b->hits++;
    g->last_result=(uint8_t)result;
    if(result==RESULT_BREAK){broken(g,b);
        forge_changed(g,false);
        return true;
        }
    unsigned gain=0;
    bool good=result==RESULT_SUCCESS||result==RESULT_GREAT;
    if(good){
        b->success_streak++;
        b->hold_streak=0;
        gain=result==RESULT_GREAT?(inside&&forge_has(s,R_GLASS)?3:2):1;
        if(result==RESULT_GREAT&&b->family==6)gain++;
        if(result==RESULT_GREAT){forge_money_scale(&b->perm,10,9);
            gain+=b->heart+b->king+b->scholar;
            b->heart=b->king=b->scholar=0;
            s->greats++;
            if(!inside&&forge_has(s,R_FORK))gain++;
            }
        else{gain+=b->wind;
            b->wind=0;
            b->successes++;
            }
        if(first&&forge_has(s,R_BRAND))gain+=2;
        if(forge_has(s,R_INTERLOCK)&&b->last==RESULT_DAMAGE)gain+=2;
        if(forge_has(s,R_HAND)&&!(b->success_streak%2))gain++;
        if(result==RESULT_SUCCESS&&forge_has(s,R_SHAFT)&&!(b->successes%2))gain++;
        if(forge_has(s,R_ECHO)&&chance(s,25))gain++;
        b->level=(uint8_t)(b->level+gain>99?99:b->level+gain);
        if(forge_has(s,R_STAMP))value_small(&last_value,&b->flat,100);
        if(forge_has(s,R_DOUBLE)&&!(b->success_streak%2))value_scale(&last_value,&b->perm,110,100);
        if(result==RESULT_GREAT&&forge_has(s,R_BELL))value_scale(&last_value,&b->perm,120,100);
        if(result==RESULT_SUCCESS&&forge_has(s,R_GOLDCOG))value_small(&last_value,&b->base,30);
        if(result==RESULT_SUCCESS&&forge_has(s,R_BRONZE)&&b->success_tickets<60000)b->success_tickets++;
        if(forge_has(s,R_DRAGONHEART)&&old_level>=7)value_scale(&last_value,&b->perm,115,100);
        if(!inside&&forge_has(s,R_CHARM)&&b->charm<6)b->charm++;
    }else if(result==RESULT_HOLD){
        b->holds++;
        b->hold_streak++;
        b->success_streak=0;
        if(forge_has(s,R_PAPERWEIGHT))value_small(&last_value,&b->flat,180);
        if(forge_has(s,R_TEA))value_scale(&last_value,&b->perm,106,100);
        if(forge_has(s,R_SEED))value_small(&last_value,&b->base,40);
        if(forge_has(s,R_SEALING)&&!(b->hold_streak%2))gain++;
        if(forge_has(s,R_WIND)&&b->wind<2)b->wind++;
        if(forge_has(s,R_HEART)&&b->heart<4)b->heart++;
        if(forge_has(s,R_CUP)&&b->great_tickets<60000)b->great_tickets++;
        if(forge_has(s,R_SLEEP)&&chance(s,70))gain++;
        if(forge_has(s,R_X_SCHOLAR)&&!(b->holds%5))b->scholar+=2;
        if(b->family==7)add_small(&b->base,20);
        b->level=(uint8_t)(b->level+gain>99?99:b->level+gain);
    }else{
        b->damages++;
        if(b->scars<60000)b->scars++;
        b->success_streak=b->hold_streak=0;
        if(!(forge_has(s,R_PEARL)&&b->damages<=2)&&b->level)b->level--;
        if(forge_has(s,R_CROWN)){forge_money_t lost=old_part,after=base_part(s,b);
            forge_money_sub(&lost,&after);
            forge_money_scale(&lost,1,2);
            value_add(&last_value,&b->flat,&lost);
            }
        if(forge_has(s,R_COMB)){forge_money_t p=old_part;
            forge_money_scale(&p,1,20);
            value_add(&last_value,&b->flat,&p);
            }if(forge_has(s,R_APRON))value_small(&last_value,&b->flat,140);
        if(forge_has(s,R_KING)&&b->king<60000)b->king++;
        if(forge_has(s,R_SPINE)&&!spine_protected){if(++b->spine_count>=2){b->spine_count=0;
                b->spine_ready=true;
                }}
        if(forge_has(s,R_OATH)&&chance(s,70)&&b->level<99)b->level++;
        if(b->family==8)add_small(&b->flat,80);
    }
    b->same=b->last==result?(uint16_t)(b->same+1):1;
    if(forge_has(s,R_WAX)&&!(b->same%2))value_small(&last_value,&b->flat,150);
    if(forge_has(s,R_CENSER)&&b->last!=RESULT_NONE&&b->last!=result)value_small(&last_value,&b->base,20);
    if(forge_has(s,R_IDOL)){unsigned milestone=b->level/4;
        while(b->milestone<milestone){value_scale(&last_value,&b->perm,115,100);
            b->milestone++;
            }}
    if(inside&&forge_has(s,R_COLLAR))value_scale(&last_value,&b->perm,105,100);
    /* Replay the actual final relic event, including milestone/compound adds. */
    if(forge_has(s,R_MIRROR)&&!(b->same%2)&&last_value.target){
        if(last_value.denominator)forge_money_scale(last_value.target,last_value.numerator,last_value.denominator);
        else forge_money_add(last_value.target,&last_value.amount);
    }
    if(!inside){unsigned charge=1+(first&&forge_has(s,R_LOCKET)?2:0)+(result==RESULT_DAMAGE&&forge_has(s,R_WATCH)?1:0);
        if(result==RESULT_HOLD&&forge_has(s,R_CHAIN)&&chance(s,50))charge++;
        if(result==RESULT_HOLD&&b->family==5)charge++;
        b->charge=(uint8_t)(b->charge+charge>10?10:b->charge+charge);
        }
    b->last=(uint8_t)result;
    forge_remember(g,b);
    if(forge_has(s,R_X_SPARK)&&b->hits==10)s->items[ITEM_CHALK]++;
    if(forge_has(s,R_TWINS)&&chance(s,2)&&blade_count(s)<FORGE_BLADES){int index=new_blade(s,0,0);
        if(index>=0){uint32_t id=s->blades[index].id;
            memcpy(&s->blades[index],b,sizeof(*b));
            s->blades[index].id=id;
            s->blades[index].place=PLACE_RACK;
            s->frenzy_energy[index]=s->frenzy_energy[s->active];
            s->frenzy_fraction[index]=s->frenzy_fraction[s->active];
            }}
    static const char *const words[5]={"破碎","损伤","不变","成功","大成功"};
    snprintf(g->notice,sizeof(g->notice),forge_tr(g,"%s%s  强化 +%u"),inside?forge_tr(g,"入神 · "):"",forge_tr(g,words[result]),b->level);
    g->notice_ms=1800;
    g->feedback=result==RESULT_GREAT?FB_GREAT:result==RESULT_DAMAGE?FB_DAMAGE:FB_HIT;
    forge_particles(g,(int)result);
    forge_changed(g,false);
    return true;
}
static void next_day(game_handle_t g,bool paid,unsigned best){
    forge_state_t *s=&g->state;
    s->day++;
    s->phase=PHASE_PLAY;
    s->wire_used=s->hook_used=s->quest_used=false;
    s->undertaker_count=0;
    s->rerolls=0;
    s->scale=s->chalk=s->powder=false;
    unsigned min=forge_has(s,R_LIFETIME)?7:paid&&forge_has(s,R_BADGE)?2:0,first=paid&&forge_has(s,R_TESTAMENT)&&best>=10?best*3/10:0;
    supply(s,3+(forge_has(s,R_PERMIT)?1:0)+s->extra_supply,min,first);
    s->extra_supply=0;
    shop_roll(s);
    merchant_stock(s);
    s->quest_level=(uint16_t)(3+s->day/2>18?18:3+s->day/2);
    g->screen=SCREEN_FORGE;
    g->page=0;
    g->modal=MODAL_NONE;
    g->pending_strike=false;
    g->heat_ms=0;
    g->cooldown_ms=0;
    snprintf(g->notice,sizeof(g->notice),forge_tr(g,"第%u日开炉 · 三把新剑已送达"),s->day);
    g->notice_ms=4000;
}
bool forge_settle(game_handle_t g,bool accept){
    forge_state_t *s=&g->state;
    if(s->phase!=PHASE_PLAY||s->day>=999)return false;
    forge_money_t amount=forge_tribute(s),need=forge_quota(s);
    bool paid=forge_money_compare(&amount,&need)>=0;
    if(!paid&&!accept){g->modal=MODAL_SHORT;
        return false;
        }
    unsigned best=0;
    for(unsigned i=0;i<FORGE_BLADES;i++)if(s->blades[i].place==PLACE_TRIBUTE){if(s->blades[i].level>best)best=s->blades[i].level;
        s->blades[i].place=PLACE_FREE;
        }
    credit(s,&amount);
    forge_money_add(&s->tribute,&amount);
    s->scrap=forge_money_u64(0);
    s->quota_credit=forge_money_u64(0);
    forge_changed(g,true);
    g->feedback=FB_BELL;
    if(paid){
        s->paid++;
        if(s->endless)s->extended_paid++;
        if(forge_has(s,R_TALLY))s->credits++;
        if(forge_has(s,R_CHANGE)){forge_money_t extra=amount,limit=forge_base_quota(s->day+1);
            forge_money_sub(&extra,&need);
            forge_money_scale(&extra,3,10);
            forge_money_scale(&limit,3,10);
            s->quota_credit=forge_money_compare(&extra,&limit)<0?extra:limit;
            }
        if(s->anger==2){s->unlocks|=2048;
            s->achievements|=2;
            }
        if(s->extended_paid>=3)s->unlocks|=1024;
    }else{
        s->anger++;
        s->failed++;
        if(forge_has(s,R_MOURNING))add_ticket(s,4,1);
        if(s->anger>=3&&forge_has(s,R_PARDON)){forge_discard(g,R_PARDON);
            s->anger=2;
            }
        if(s->anger>=3){s->phase=PHASE_LOST;
            g->modal=MODAL_NONE;
            forge_notice(g,"第三次欠贡，契约终结");
            return true;
            }
    }
    if(s->day==21&&!s->endless){s->phase=PHASE_CLEAR;
        s->cleared=true;
        s->clear_paid=paid;
        s->clear_best=(uint8_t)best;
        s->achievements|=4;
        g->modal=MODAL_NONE;
        return true;
        }
    next_day(g,paid,best);
    return true;
}
bool forge_continue(game_handle_t g){if(g->state.phase!=PHASE_CLEAR)return false;
    g->state.endless=true;
    next_day(g,g->state.clear_paid,g->state.clear_best);
    forge_changed(g,true);
    return true;
    }
bool forge_family(game_handle_t g,int family){forge_blade_t *b=forge_active(g);
    if(!b||b->family||b->level<6||family<1||family>6)return false;
    b->family=(uint8_t)family;
    forge_remember(g,b);
    forge_changed(g,true);
    forge_notice(g,"进化路线已选定；达到+20解锁名匠遗物");
    return true;
    }

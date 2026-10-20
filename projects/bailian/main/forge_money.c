// SPDX-License-Identifier: Apache-2.0
#include "forge_money.h"
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(1000000000)
static void saturate(forge_money_t *a){for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++)a->v[i]=BASE-1;
    }
forge_money_t forge_money_u64(uint64_t n){forge_money_t a={{0}};
    for(unsigned i=0;i<FORGE_MONEY_LIMBS&&n;i++){a.v[i]=(uint32_t)(n%BASE);
        n/=BASE;
        }return a;
    }
bool forge_money_zero(const forge_money_t *a){for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++)if(a->v[i])return false;
    return true;
    }
int forge_money_compare(const forge_money_t *a,const forge_money_t *b){for(int i=FORGE_MONEY_LIMBS-1;i>=0;i--)if(a->v[i]!=b->v[i])return a->v[i]>b->v[i]?1:-1;
    return 0;
    }
void forge_money_small(forge_money_t *a,uint32_t n){uint64_t carry=0;
    for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++){uint64_t v=(uint64_t)a->v[i]*n+carry;
        a->v[i]=(uint32_t)(v%BASE);
        carry=v/BASE;
        }if(carry)saturate(a);
    }
uint32_t forge_money_div(forge_money_t *a,uint32_t d){if(!d)return 0;
    uint64_t r=0;
    for(int i=FORGE_MONEY_LIMBS-1;i>=0;i--){uint64_t v=r*BASE+a->v[i];
        a->v[i]=(uint32_t)(v/d);
        r=v%d;
        }return(uint32_t)r;
    }
void forge_money_scale(forge_money_t *a,uint32_t n,uint32_t d){forge_money_small(a,n);
    (void)forge_money_div(a,d);
    }
void forge_money_add(forge_money_t *a,const forge_money_t *b){uint64_t carry=0;
    for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++){uint64_t v=(uint64_t)a->v[i]+b->v[i]+carry;
        a->v[i]=(uint32_t)(v%BASE);
        carry=v/BASE;
        }if(carry)saturate(a);
    }
void forge_money_sub(forge_money_t *a,const forge_money_t *b){if(forge_money_compare(a,b)<0){memset(a,0,sizeof(*a));
        return;
        }int64_t borrow=0;
    for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++){int64_t v=(int64_t)a->v[i]-b->v[i]-borrow;
        borrow=v<0;
        if(borrow)v+=BASE;
        a->v[i]=(uint32_t)v;
        }}
void forge_money_mul(forge_money_t *a,const forge_money_t *b){
    /* 96 bytes of bounded temporary limbs, below the application's stack limit. */
    forge_money_t result={{0}};
    bool overflow=false;
    for(unsigned i=0;i<FORGE_MONEY_LIMBS;i++){
        uint64_t carry=0;
        for(unsigned j=0;j<FORGE_MONEY_LIMBS;j++){
            if(i+j>=FORGE_MONEY_LIMBS){if((a->v[i]&&b->v[j])||carry)overflow=true;
                continue;
                }
            uint64_t v=(uint64_t)a->v[i]*b->v[j]+result.v[i+j]+carry;
            result.v[i+j]=(uint32_t)(v%BASE);
            carry=v/BASE;
        }
        if(carry)overflow=true;
    }
    *a=result;
    if(overflow)saturate(a);
}
bool forge_money_parse(forge_money_t *a,const char *text){
    if(!text||!*text||strlen(text)>FORGE_MONEY_DIGITS)return false;
    *a=forge_money_u64(0);
    for(;*text;text++){if(*text<'0'||*text>'9')return false;
        forge_money_small(a,10);
        forge_money_t d=forge_money_u64((unsigned)(*text-'0'));
        forge_money_add(a,&d);
        }return true;
}
void forge_money_text(const forge_money_t *a,char *out,size_t cap,bool compact){
    if(!out||!cap)return;
    char text[FORGE_MONEY_DIGITS+1];
    int top=FORGE_MONEY_LIMBS-1;
    while(top>0&&!a->v[top])top--;
    int used=snprintf(text,sizeof(text),"%lu",(unsigned long)a->v[top]);
    for(int i=top-1;i>=0;i--)used+=snprintf(text+used,sizeof(text)-(size_t)used,"%09lu",(unsigned long)a->v[i]);
    const size_t len=strlen(text);
    if(!compact||len<=4){snprintf(out,cap,"%s",text);
        return;
        }
    if(len>20){snprintf(out,cap,"%c.%c%ce%u",text[0],text[1],text[2],(unsigned)len-1);
        return;
        }
    const char *unit=len>16?"京":len>12?"万亿":len>8?"亿":"万";
    const unsigned shift=len>16?16:len>12?12:len>8?8:4,whole=(unsigned)len-shift;
    snprintf(out,cap,"%.*s.%c%c%s",whole,text,text[whole],text[whole+1],unit);
}

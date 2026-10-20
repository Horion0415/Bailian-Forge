// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* 108 decimal digits; no floating point or target-dependent __int128. */
enum { FORGE_MONEY_LIMBS=12, FORGE_MONEY_DIGITS=108 };
typedef struct { uint32_t v[FORGE_MONEY_LIMBS]; } forge_money_t;
forge_money_t forge_money_u64(uint64_t value);
bool forge_money_parse(forge_money_t *out, const char *decimal);
int forge_money_compare(const forge_money_t *a,const forge_money_t *b);
bool forge_money_zero(const forge_money_t *a);
void forge_money_add(forge_money_t *a,const forge_money_t *b);
void forge_money_sub(forge_money_t *a,const forge_money_t *b);
void forge_money_mul(forge_money_t *a,const forge_money_t *b);
void forge_money_scale(forge_money_t *a,uint32_t numerator,uint32_t denominator);
void forge_money_small(forge_money_t *a,uint32_t multiplier);
uint32_t forge_money_div(forge_money_t *a,uint32_t divisor);
void forge_money_text(const forge_money_t *value,char *out,size_t capacity,bool compact);

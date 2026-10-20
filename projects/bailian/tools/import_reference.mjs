// Import our existing independently implemented reference data, not game assets/code.
import {createRequire} from 'node:module';
import {writeFile,mkdir} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const source=process.argv[2];if(!source)throw Error('Pass the absolute bailian.cjs reference path.');
const F=createRequire(import.meta.url)(path.resolve(source));
await mkdir(path.join(root,'assets_src'),{recursive:true});
await writeFile(path.join(root,'assets_src/rules.json'),JSON.stringify({relics:F.RELICS,items:F.ITEMS,quotas:F.QUOTAS,break:F.BREAK,starters:F.STARTERS,families:F.FAMILIES,sources:F.SOURCES},null,2));
const esc=s=>JSON.stringify(s);
const iconFor=r=>({nail:0,spike:0,goldNail:0,lucky:3,stamp:1,hoop:2,charcoal:3,bell:4,spine:5,king:6,crown:6,watch:7,chalice:8,tongs:9,tea:10,heart:11,holy:12,dawn:12,knot:13,glass:14,hourglass:14,core:15})[r.id]??({'成功':0,'大成功':1,'保命':2,'价值':3,'不变':10,'损伤':5,'入神':14,'继承':12,'碎刀':8,'经济':1,'连锁':13,'补给':3,'复制':11,'签池':15,'扩展':15})[r.tag]??1;
const fam=['plain','citadel','holy','saw','dragon','rune','split','world','fish','funerary'];
let h='// Generated from assets_src/rules.json; no original game code.\n#pragma once\n#include <stdint.h>\ntypedef enum {\n'+F.RELICS.map((r,i)=>`    R_${r.id.toUpperCase()}=${i},`).join('\n')+'\n    R_COUNT\n} forge_relic_id_t;\ntypedef struct { const char *id,*name,*effect; uint8_t day,cost,rarity,icon,extension; uint16_t unlock; } forge_relic_def_t;\nextern const forge_relic_def_t forge_relics[R_COUNT];\nextern const uint32_t forge_quotas[21];\nextern const uint16_t forge_break_permille[31];\nextern const uint8_t forge_starters[8];\nextern const char *const forge_family_names[10];\n';
let c='// Generated from independently implemented reference data.\n#include "forge_data.h"\nconst forge_relic_def_t forge_relics[R_COUNT]={\n'+F.RELICS.map(r=>` {${esc(r.id)},${esc(r.name)},${esc(r.effect)},${r.day},${r.cost},${r.rarity},${iconFor(r)},${r.extension?1:0},${r.unlock==='endless'?1024:r.unlock==='pardon'?2048:r.unlock?1<<fam.indexOf(r.unlock):0}},`).join('\n')+'\n};\n';
c+='const uint32_t forge_quotas[21]={'+F.QUOTAS.join(',')+'};\nconst uint16_t forge_break_permille[31]={'+F.BREAK.map(x=>Math.round(x*1000)).join(',')+'};\n';
c+='const uint8_t forge_starters[8]={'+F.STARTERS.map(id=>F.RELICS.findIndex(r=>r.id===id)).join(',')+'};\nconst char *const forge_family_names[10]={'+fam.map(id=>esc(F.FAMILIES[id])).join(',')+'};\n';
await writeFile(path.join(root,'main/forge_data.h'),h);await writeFile(path.join(root,'main/forge_data.c'),c);
console.log('Imported '+F.RELICS.length+' relics, five-result break curve and 21-day quota table.');

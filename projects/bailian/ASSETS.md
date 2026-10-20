# 素材与规则来源

## 图像

assets_src 下 forge-room.png、hammer.png、swords.png、relics.png 为本次实现使用内置 ImageGen 生成的原创游戏素材。背景、手持锤、十种剑与十六个类别图标分开生成，构建脚本负责缩放、切格和 RGB565 量化。未提取原版游戏的图片、模型、音频、标志或代码。

视觉方向为暗色中世纪石砌铁匠铺：左侧暖橙熔炉，右侧铁栅与龙眼，中央铁砧，金色/象牙色中文界面，固定第一人称。原图保留透明通道，刀剑与锤子由共享 C 渲染器独立移动/旋转。

## 字体

构建机来源为 Noto Sans CJK Regular/Bold TTC，采用 Simplified Chinese 字面（index 2），烘焙当前 C 源码与规则文本需要的字符。按 15 个实际 UI 字号分别生成 font.mask 灰度 A8 子集，采用真实字宽与基线，无运行时位图缩放。字体及子集适用 SIL Open Font License 1.1，声明见 licenses/Noto-OFL.txt。

小字号使用 Regular，大标题使用 Bold。文字保留灰度抗锯齿，RGB565 混色已按通道修复，不再将红色覆盖率带入蓝色。缺少字体会构建失败。

## 音频

hit/great/damage/break/bell/select/heat/frenzy.wav 及 .sound 为本项目确定性合成的原始音效。合成参数与随机种子在 assets_src/prepare_assets.py，不依赖下载的采样音源。

## 规则

规则数据来自本项目先前独立编写的终端版 bailian.cjs，通过 tools/import_reference.mjs 导入 assets_src/rules.json 和 C 数据表。日常构建无需运行导入器。终端版列出的参考入口如下，第三方说明不构成原版源码，也不能保证后续版本机制不变：

- [ForgeTax Steam 页面](https://store.steampowered.com/app/5245810/ForgeTax/)
- [锻造概率](https://www.forgetax.tools/guides/forging-odds)、[剑值](https://www.forgetax.tools/guides/sword-value)、[入神](https://www.forgetax.tools/guides/frenzy)
- [贡额](https://www.forgetax.tools/tribute)、[遗物](https://www.forgetax.tools/relics)、[商人](https://www.forgetax.tools/merchant)
- [中文玩法说明](https://forgetax.net/zh/guide)

实现采用固定视角和触摸交互，经典模式默认排除 4 件自定义扩展遗物。遗物连锁、镜像与成就等未获原版程序验证的细节按项目规则近似实现，游戏名称与界面为本项目的“百炼”。

## 特效

新增的光晕、热浪、粒子、镜头偏移、刀刃切片和飘字由 main/forge_effects.c 与共享原生 C 渲染器实时生成，复用已有刀剑图片，不新增提取的原版素材。设计方向参考 [原作 Steam 介绍](https://store.steampowered.com/app/5245810/ForgeTax/) 的第一人称加热、落锤、碎刀与短时入神爆发；具体光色、震幅和回弹曲线为本项目的小屏适配参数。

进贡仪式中的龙影、眼部光晕、贡台刻度环和金币轨迹由 main/forge_tribute.c 在同一 C 渲染器中生成，贡剑复用本项目剑图。它们是本项目的程序化视觉设计，不使用从原作提取的图像。

剑体形态新增四张透明图集 swords-plus3.png / swords-plus6.png / swords-plus10.png / swords-plus20.png，使用内置 ImageGen 参考本项目原始 swords.png 生成。每张保留十个剑系，分别强调初锻、精锻、名匠及传说阶段的刀刃、护手、柄首和纹饰变化，未使用提取的原作图像。生成规格记录在 assets_src/sword-forms.json。构建期仅进行独立精灵提取、尺寸适配、刃线对齐及透明留白打包，源 PNG 保持完整；五阶段共 50 个精灵。

## 加热 UI

入炉、升温、回砧的刀胚位置/尺寸与颜色由 forge_heating.c 连续插值，火候条和分阶段提示由共享 C 原生绘制，复用已有刀剑与炉火图片。无新增大贴图、网页专用动画或运行时字体依赖。

## 英文文本

英文名称与说明为本项目对独立实现规则的翻译，覆盖经典和预留扩展数据；不代表原作官方英文文本。

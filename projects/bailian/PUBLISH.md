# Bailian Forge · 百炼

在掌心铁匠铺里，将一块刀胚锻成名剑。每次落锤都可能强化、损伤或碎刀；
决定何时收手、搭配哪些遗物，在不断上涨的贡额中挑战 21 日契约。

![ESP-Mosaico 实机锻造画面](publish/cover.png)

- 480×480 触摸界面，固定第一人称铁匠铺。
- 10 个剑系与 50 种剑体形态，强化阶段改变刀刃、护手与纹饰。
- 94 件经典遗物、7 种商人道具，以及通关后的延长契约。
- 连贯加热、快速落锤、入神爆发与进贡仪式，支持音效、震动及静音设置。
- 主机预览与设备共用 C 规则及渲染器，设备自动保存游戏进度。

![剑体进化示意回放](publish/sword-evolution.gif)

上图为测试场景回放，用于展示剑体变化。当前界面为简体中文，适用于
ESP-Mosaico / ESP32-S31。玩法受 ForgeTax 启发，规则独立实现，图像通过
ImageGen 生成，音效由程序合成。

[源码与运行说明](https://github.com/Horion0415/Bailian-Forge)
· [素材与规则来源](https://github.com/Horion0415/Bailian-Forge/blob/main/projects/bailian/ASSETS.md)

版本 0.2.1 增加独立保存的静音开关。当前实机通过 app-update 验证，平台下载首次安装
尚待验证；此前 Vibe Mode 0.1.4 的一次 System Update 传输中重启，原因尚未确认。

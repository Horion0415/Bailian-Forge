# Bailian Forge · 百炼

在掌心铁匠铺里，将一块刀胚锻成名剑。每次落锤都可能强化、损伤或碎刀；
决定何时收手、搭配哪些遗物，在不断上涨的贡额中挑战 21 日契约。

![ESP-Mosaico 实机锻造画面](publish/cover.png)

- 480×480 触摸界面，固定第一人称铁匠铺，支持中英文切换。
- 10 个剑系与 50 种剑体形态，强化阶段改变刀刃、护手与纹饰。
- 94 件经典遗物、7 种商人道具，以及通关后的延长契约。
- 连贯加热、快速落锤、入神爆发与进贡仪式，支持音效、震动及静音设置。
- 主机预览与设备共用 C 规则及渲染器，设备自动保存游戏进度。

![剑体进化示意回放](publish/sword-evolution.gif)

![英文界面原生回放示例](publish/english.png)

![进贡巨龙原生回放示例](publish/tribute-dragon.png)

![碎刀回溯原生回放示例](publish/rewind.png)

剑体动画、英文界面、进贡巨龙及碎刀回溯图片来自原生测试场景。支持顶部 EN / ZH 和暂停菜单即时切换语言，适用于
ESP-Mosaico / ESP32-S31。玩法受 ForgeTax 启发，规则独立实现，图像通过
ImageGen 生成，音效由程序合成。

[源码与运行说明](https://github.com/Horion0415/Bailian-Forge)
· [素材与规则来源](https://github.com/Horion0415/Bailian-Forge/blob/main/projects/bailian/ASSETS.md)

版本 0.3.3 修复碎刀后查看背包、返回锻造及继续连点时的回溯入口，并显示发条数量与可用状态；包含 0.3.2 的巨龙重绘及中英文切换。

0.3.3 已在 ESP-Mosaico 上通过应用更新、固件校验、启动和存档恢复检查，静音设置可正常保留；碎刀回溯的操作路径通过共享原生 C 回放验证。
平台下载后的首次安装仍待验证；此前 Vibe Mode 0.1.4 的一次完整更新在传输中重启，原因尚未确认。

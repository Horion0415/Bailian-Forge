# Bailian Forge · 百炼

[English](README.md)

面向 ESP-Mosaico 的触摸锻刀游戏：加热、落锤、收手进贡，在碎刀风险中打造名剑，
搭配遗物与道具，挑战 21 日贡额。支持简体中文与英文即时切换。

![ESP-Mosaico 实机画面](projects/bailian/publish/cover.png)

- 10 个剑系、50 种实体形态，强化最高 +99。
- 94 件经典遗物、7 种商人消耗品，通关后可延长契约。
- 火星、碎刀、入神与进贡动画，配合音效和震动反馈。
- 设备独立保存语言与静音设置；浏览器与设备共用原生 C 规则及渲染器。

玩法受 ForgeTax 启发，采用独立实现、生成美术和本项目的平衡规则，属于改编作品。

## 在电脑运行

主机流程已在 Linux 验证，需要 Python 3.10+、C 编译器和 Noto CJK 字体。
音频测试另需 Node.js。字体脚本读取 `/usr/share/fonts/opentype/noto` 下的
NotoSansCJK-Regular.ttc 和 NotoSansCJK-Bold.ttc。

```sh
git clone https://github.com/Horion0415/Bailian-Forge.git
cd Bailian-Forge
python3 tools/bootstrap.py
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-host.txt
# Ubuntu/Debian 缺少工具或字体时安装 build-essential、fonts-noto-cjk。
python3 mosaico.py game sim --project projects/bailian --port 8476
```

打开 <http://127.0.0.1:8476/>，使用鼠标或触摸操作；F 或 Ctrl 加热/落锤，P 暂停。
首次点击或按键后才能播放声音，游戏内和工具栏均可静音。模拟器不需要开发板或 ESP-IDF。

## 切换语言

点击顶部 **EN / ZH**，或暂停菜单音效开关旁的语言按钮。遗物、道具、规则、提示与动画文字
均随之切换，英文金额采用 K/M/B/T/Qa/Qi。进贡期间也可切换，不影响进度、回溯或静音。
设备独立保存语言选择；浏览器选择在本次模拟器进程内保留。

![英文界面原生回放示例](projects/bailian/publish/english.png)

## 编译与安装

目标为 **ESP-Mosaico / ESP32-S31，480×480 触摸屏**。ESP-IDF 必须固定为
`7b9cc1ac79f865983f59bb8ff3ff43eb74ff1dbe`，并安装其 S31 工具链。
激活该 SDK 的 Python 环境后编译：

```sh
python mosaico.py game build --project projects/bailian --idf-path /path/to/pinned-esp-idf
```

保留整个仓库目录结构。仓库包含依赖版本、组件锁文件和必需的引擎补丁。
详细信息见[应用说明](projects/bailian/README.md)及[官方安装指南](https://github.com/esp-mosaico/esp-mosaico-vibe/blob/v26.0.2/docs/mosaico-cli_CN.md)。
首次安装通过保留的 Vibe Mode 执行 System Update；完整分区表和资源一致的应用更新
才适用 `app-update`，请通过产品 CLI 操作。

0.3.2 已通过 `app-update` 安装、固件校验及启动检查，并确认存档和静音设置恢复。
较早的一次 System Update 在传输阶段触发 Vibe Mode 0.1.4 重启，原因尚未确认；
mosaico.ai 网页下载安装仍待验证。
浏览器与设备存档独立，浏览器的游戏静音选项在当前模拟器进程内保留。

## 验证

```sh
python3 tools/bootstrap.py --check
python3 projects/bailian/tests/run_model_tests.py
python3 projects/bailian/tests/replay_gameplay.py
python3 projects/bailian/tests/replay_audio.py
python3 projects/bailian/tests/replay_i18n.py
python3 projects/bailian/tests/replay_rewind.py
```

测试直接执行共享原生 C 模型与渲染器。构建目录、测试输出、凭据和设备诊断记录不进入 Git。
初始化后，引擎子模块会显示补丁修改，这是预期状态。

## 许可与来源

代码采用 [Apache-2.0](LICENSE)，字体采用 [SIL OFL 1.1](projects/bailian/licenses/Noto-OFL.txt)，
各依赖保留原有许可。参阅 [NOTICE](NOTICE)、[素材与规则来源](projects/bailian/ASSETS.md)
及[版本说明](CHANGELOG.md)。

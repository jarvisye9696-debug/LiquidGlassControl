# Liquid Glass Control

Windows 11 x64 · [Downloads / 下载](https://github.com/jarvisyip9696/LiquidGlassControl/releases) · [Contribute / 参与开发](CONTRIBUTING.md)

License / 许可证: owner's and DK's original contributions use [GPL-3.0-only](LICENSE); see [licensing scope and third-party notices](LICENSING.md). / 所有者与 DK 的原创贡献采用 GPL-3.0-only，其他贡献的授权范围见说明。

[Code signing policy / 代码签名政策](CODE-SIGNING.md) — SignPath application preparation; current builds remain unsigned. / 正在准备 SignPath 申请，当前版本仍未签名。

## English

**0.11.1 experimental — live shell testing is still required.**

- Liquid Glass controls for taskbar, Start, notifications, Explorer, volume and brightness; no Windhawk runtime required.
- Separate Dock hover animation and smart taskbar auto-hide switches, off by default.
- Settings → Glass: independent blur, tint and corner controls; battery-saving fallback.
- Compact experimental taskbar capsules; animated controls with hover-only theme outlines. Settings contains General, Glass and About.
- English, 简体中文, 繁體中文, Polski and Tiếng Việt; preferences are saved.
- Close to tray; Restore and exit reverts changes. Competing Windhawk Stylers must be disabled for testing. Classic Explorer lists and custom app content may stay opaque.

## 中文

**0.11.1 实验版，系统界面仍需实机验证。**

- 任务栏、开始菜单、通知中心、资源管理器、音量和亮度玻璃开关，无需 Windhawk 运行时。
- 新增 Dock 悬停动画和任务栏智能隐藏，默认关闭、单独开启。
- 设置 → 玻璃效果：分别调节模糊、着色和圆角，支持电池省电回退。
- 紧凑实验性任务栏胶囊；控件支持悬停放大及同配色细边框。设置仅保留常规、玻璃效果和关于。
- 支持英文、简体中文、繁體中文、波兰语、越南语，自动保存设置。
- 关闭至托盘，“还原并退出”撤销效果。测试时请停用冲突的 Windhawk Styler；经典文件列表和自绘内容区可能仍不透明。

[Build & release / 构建发布](RELEASING.md) · [Architecture / 架构](ARCHITECTURE.md) · Imported engine/material code: GPLv3; see third_party/windhawk / 移植引擎与材质代码：GPLv3，详见 third_party/windhawk

Contributors / 贡献者：[DK (@domk6966)](https://github.com/domk6966)

0.11.1 imports four Styler LiquidGlass material presets and subscription compatibility hooks. Local builds, isolated self-tests and desktop deployment passed. Notification-center styling is still unconfirmed; capsule tray appearance and restoration need further live checks. CPU, power and memory improvements are not benchmarked. Builds remain unsigned.
0.11.1 已移植四组 Styler 的 LiquidGlass 材质规则及订阅兼容逻辑。本机构建、自测与桌面部署已通过；通知中心效果仍未确认，胶囊托盘外观及恢复仍需完整实测。CPU、功耗和内存优化尚未进行前后基准测量。当前构建未签名。

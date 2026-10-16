# Liquid Glass Control

Windows 11 x64 glass controls · Windows 11 x64 玻璃效果工具

[Stable downloads / 稳定版下载](https://github.com/jarvisyip9696/LiquidGlassControl/releases) · [Contribute / 参与开发](CONTRIBUTING.md)

## English

**0.10.1 is experimental; live shell validation is pending.**

- Standalone XAML material engine embedded in one EXE; no Windhawk installation required. The engine DLL is extracted beside the EXE at runtime.
- Independent volume, brightness, taskbar, Start, notification and Explorer XAML switches. Taskbar layout and size are preserved.
- Settings → Glass: tint strength, light/dark tint, corners and battery-friendly fallback. English/Chinese and preferences are saved.
- Event driven: no periodic scans or idle animation loop. Close to tray; Restore and exit reverts effects.
- Classic Explorer lists and custom app content can stay opaque. This is not the complete File Explorer Styler or optical refraction effect.
- Upgrade restores this app's saved Windhawk settings first; keep recovery files. Unsigned builds may be blocked by application-control policy.

## 中文

**0.10.1 是实验性开发版本，系统界面仍需实机验证。**

- 独立 XAML 材质引擎，单 EXE 分发，无需安装 Windhawk；运行时在程序旁释放引擎 DLL。
- 音量、亮度、任务栏、开始菜单、通知中心和资源管理器 XAML 区域独立开关，保留任务栏原布局与尺寸。
- 设置 → 玻璃效果：调整着色强度、深浅色、圆角及电池省电回退；中英文与偏好自动保存。
- 事件触发，无定时扫描和闲置动画循环。关闭至托盘，使用“还原并退出”撤销效果。
- 经典文件列表和其他应用自绘内容区可能不透明，尚非完整 File Explorer Styler 或光学折射效果。
- 升级先恢复本程序保存的旧 Windhawk 配置，完成前保留恢复文件。程序暂未签名，可能被应用程序控制策略拦截。

Contributors / 贡献者：[DK (@domk6966)](https://github.com/domk6966)

[Build & release / 构建与发布](RELEASING.md) · [Architecture / 架构](ARCHITECTURE.md) · No project license selected / 项目暂未选择许可证

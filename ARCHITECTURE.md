# Standalone materials (0.10.0 experimental)

`glass_controller.h` extracts the embedded DLL, publishes bounded preferences in a per-controller shared mapping and attaches temporary thread hooks only to selected shell/Explorer windows. Hooks are removed after bootstrap. New effects do not use Windhawk runtime, SDK, mod DLLs or registry settings.

`glass_engine.cpp` hosts COM XAML diagnostics sessions for system XAML and, where available, Explorer WinUI 3. A small allowlist identifies material surfaces. No dimensions, margins, padding, alignment, taskbar position or work area are written. Original local properties are retained; style/theme sources are restored with ClearProperty. Binding-backed properties are skipped. Node tracking uses weak references, removed nodes are discarded, and diagnostics references are released in batches after callbacks.

No frame loop or periodic scan runs while idle. Window events are debounced and observed only when effects are enabled. Sliders publish after a drag, rather than constructing brushes for each pointer move. Power notifications select a solid fallback on battery. A process-handle wait triggers restoration if the controller exits. The small DLL remains pinned in the shell until that process exits so COM callbacks remain valid; include this resident cost in measurements.

`glass_features.h` owns settings and one-time migration. Old background/native/theme headers remain for recovery and isolated tests. Deleting these would strand users with old backup files. Old style-application CLI commands and their elevation/job polling paths have been removed.

Limitations: WinUI 3 Acrylic is an in-app material. Classic Explorer content uses native GDI/DirectUI rendering which this engine does not replace. It does not reproduce the entire File Explorer Styler, arbitrary themes or optical refraction. Windows updates, protected/elevated processes and competing diagnostics consumers can block attachment. COM factory tests do not establish live styling success.

Before release, verify without Windhawk installed/running: every switch's visible effect and restoration; taskbar bounds/work area unchanged; controller restart and Explorer open/close stress; property/resource restoration; light/dark/high contrast and reduced motion; battery/source transitions; idle CPU and resident memory in controller and injected processes. Do not call this stable until these checks pass.

References (original implementation; GPL mod sources are not bundled):

- https://learn.microsoft.com/windows/win32/api/xamlom/nf-xamlom-initializexamldiagnosticsex
- https://learn.microsoft.com/windows/apps/develop/ui/in-app-acrylic
- https://github.com/ramensoftware/windows-11-taskbar-styling-guide/tree/main/Themes/LiquidGlass2

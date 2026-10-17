# Standalone materials (0.10.1 experimental)

Current development work adds an opt-in taskbar capsule. Unlike material-only
switches, it changes the background rectangle's margins and radii and clips the
tray composition visual. It does not move icons, resize shell windows, or change
the work area. Layout notifications coalesce onto the existing message-thread
timer; unchanged geometry skips property writes. Disabling the effect restores
saved properties and the original tray clip. Desktop appearance, popups, and
restoration still require live validation on the target Windows build.

Packaged-shell bootstrap uses at most 32 pending thread hooks per batch, shared
between host processes, and at most five batches per discovery. Queue failures
are logged. This is a bounded attachment attempt, not proof that notification
center styling works. `--diagnose-shell` writes a read-only window inventory to
`shell-diagnostics.txt` beside the executable, without collecting window titles.

The controller now handles per-monitor DPI changes from stored logical control
bounds. Slider rendering retains native input and accessibility; control, page,
and palette timers stop after motion finishes. High contrast uses system colors
and disables motion. The self-test exercises control geometry and font recreation
at 96, 144, and 192 DPI across all five languages; this does not replace visual
checks on actual monitors or a live high-contrast desktop.

Efficiency changes in the current candidate: UI accessibility and power preferences refresh on system broadcasts
instead of every paint; battery animations use a 32 ms frame interval; unchanged
animation values skip invalidation. Language changes reuse the current window
and icon list. Translation lookup uses a sorted pointer index, while diagnostic
deduplication stores bounded hashes instead of whole log strings. Pending XAML
cache releases drain in bounded batches even during sustained mutations, and
material update messages coalesce per thread. These reduce specific sources of
work and retention; CPU, power, and private-byte deltas remain unmeasured.

Run `./verify-localization.ps1` for a desktop-free check of Polish/Vietnamese
coverage, empty entries, and duplicate keys. Traditional Chinese conversion also
normalizes common Taiwan UI terms. Coverage checks do not establish text fit;
five-language visual layout checks remain part of release validation.

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

0.10.1 fixes: query the diagnostics service's version 3 interface; enumerate material property chains rather than assuming GetPropertyIndex accepts unqualified names; use a bounded asynchronous message-thread bootstrap for packaged shell processes; acknowledge actual per-surface writes and show unconfirmed requests explicitly. Embedded DLL filenames include a content identifier for safe hot updates. On runtimes without the optional diagnostics-cache-release interface, observe at most 8192 reports; that compatibility path can retain diagnostics references and needs further memory validation. Unchanged material settings skip repeated writes.


0.11.0 experimental: composition_glass.h implements a shared Gaussian-blur and tint effect graph for system XAML and WinUI 3, with independent blur strength and explicit composition result reporting. Selected Liquid Glass surface names and layered-background behavior were studied from the user-provided Styler sources; their GPL implementations, theme engines, private-symbol hooks and Windhawk runtime are not copied or shipped. This is a focused standalone implementation, not a complete port of the mods. xaml_property_watch.h reapplies backgrounds after visual-state changes and unregisters callbacks on restoration. Inactive descriptors remain weak so enabling another switch does not require a fresh tree.

dock_animation.h uses taskbar pointer events and short compositor scale animations, with no idle frame loop. It restores original visual scales and centers on disable and is suppressed by battery fallback. Smart taskbar hide has been removed. src/taskbar_recovery.h only restores the original AppBar state recorded by earlier builds; it installs no hooks or timers.

prepare-winrt.ps1 downloads fixed, SHA256-verified Microsoft build packages and generates framework headers. No Windhawk installation is required for the CMake/Visual Studio build. The GPU self-test creates a dispatcher queue and real composition blur graphs; that still does not establish shell attachment, independent surface behavior, or idle power consumption. Live checks remain necessary, especially with competing XAML diagnostic consumers.

0.11.1 supersedes the no-copy statement above: GPLv3 material rules and registry compatibility hooks are now adapted from the supplied m417z source. See third_party/windhawk/README.md. Only static material rules are applied; shell geometry is excluded. MinHook replaces the earlier incomplete IAT shim. No live execution on the device is attempted while application-control policy blocks the engine.

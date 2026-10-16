# Liquid Glass Control v0.9

Development status: private collaboration; no open-source license has been selected. See [Contributing](CONTRIBUTING.md) for building, testing and pull requests, and [Releasing](RELEASING.md) for draft releases. 中文协作流程见贡献说明。

Native Windows application with app background controls, independent styling switches for the native volume/brightness indicators, and Chinese/English UI. Run the desktop **Liquid Glass Control** shortcut or `build/LiquidGlassControl.exe`.

First launch defaults to English. Language changes under Settings > General are saved to `settings.ini` beside the EXE and loaded on the next launch. Existing saved language preferences are preserved.

## Shareable download package

Run `./package.ps1` to build and test a clean release, then generate `dist/LiquidGlassControl-0.9-win11-x64.zip`, a standalone EXE and `SHA256SUMS.txt`. Use `-SkipBuild` to package the current `build/LiquidGlassControl.exe`. The ZIP contains only the EXE, bilingual quick start and optional desktop-shortcut script. Personal settings, recovery backups, logs and test outputs are excluded by an explicit allowlist. The shortcut script resolves the recipient's own Desktop folder and uses the embedded EXE icon.

Recipients extract the ZIP to a writable folder and double-click the EXE. This is a portable Windows 11 x64 release; specialized effects still require the optional Windhawk mods described in the included guide. Publishing to a public download host is a separate step; the packaging script does not upload anything. The executable is not code-signed.

## v0.9 UI

- **App glass:** each row shows the application's own icon, executable and window title. Icons are cached and released at exit. Unsupported icon extraction falls back to a generic app icon. Apply / Restore retain their previous behavior.
- **Volume and brightness:** two independent rows, each with an 88-logical-pixel-high **On** button on the left and **Off** on the right. Highlighting changes only after the backend succeeds. On means glass; an individual Off uses a plain opaque background. Both Off restores the original native styles.
- **Settings:** four subpages: General, Motion, Compatibility and About. Language selection is under **Settings > General**, alongside the startup animation preference. Program-owned captions, buttons, notes, status and tray strings are paired; OS-owned window menus follow the Windows language.
- **Startup:** one 180 ms opacity/12 px slide transition on this app's window, then the animation timer stops. It respects the Windows client-area animation preference and can be disabled in General. No extra volume/brightness overlay is created.

The final v0.9 executable compiled and its self-test ran successfully. The desktop executable was updated. App icons, the tall switches and settings subpages were viewed on this machine. Live configuration tests exercised volume-only and brightness-only styling, then restored the original settings exactly apart from the expected reload marker. Visual verification of the mixed native indicator states and frame-by-frame startup motion remains separate from those configuration tests.

## Native indicators

The backend uses the installed, enabled **Windows 11 Taskbar Styler** renderer, which also hosts `HWConfirmatorUI`. It does not select a new taskbar theme. Styles are restricted to `Grid#ConfirmatorMainGrid` and its `VolumeConfirmator` / `BrightnessConfirmator` children. The plate uses a transparent base, children use either blur or an opaque theme brush, and child corners are rounded. Native window position and size are retained; taskbar selectors and layout settings are untouched.

Optional XAML entrance/reposition transitions depend on the shell's control lifecycle. They do not guarantee animation for every repeated key press; Windows continues to control actual values and progress animation. Reduced-motion preferences are respected. Full optical refraction is not implemented.

Changes require administrator approval. Before writing, original registry types/data are recorded beside the EXE in `native-osd-originals.bin` (plate) and `native-osd-switches.bin` (child rules). Old plate backup formats remain supported. Keep these backups. Both Off, Restore, or Restore and exit undo the recorded changes. A failed operation keeps recovery records for retry. The two switch preferences are saved only after successful writes/readback.

Native target references: [official taskbar theme examples](https://github.com/ramensoftware/windows-11-taskbar-styling-guide/blob/main/Themes/Oversimplified%26Accentuated/README.md). Reload protocol: [Windhawk maintainer instructions](https://github.com/ramensoftware/windhawk/discussions/393).

## App glass and performance

Ordinary applications use DWM backdrop/corner requests. Chrome, Electron and other custom-rendered opaque content may remain unchanged; labels distinguish this from Explorer's dedicated backend. File Explorer content treatment requires installed File Explorer Styler and saves its original values in `explorer-background-originals.bin`. Existing custom Explorer themes/styles cause the first apply to stop rather than overwrite them.

Window create/show events are coalesced over 180 ms. Applied attributes are cached; a 15-second reconciliation runs only while rules are active. Idle rule/job timers are stopped. Repeatedly overriding an application's own material every frame is avoided. Parent painting/page changes are buffered, fonts are reused, and app icons are cached.

Closing the window hides it to the tray. **Restore and exit** attempts to restore ordinary app attributes, Explorer background and native indicator styles before exiting. Forced termination cannot guarantee restoration; keep backups for recovery. The program does not intercept media keys or change actual volume/brightness levels.

## Icon and build

The desktop, EXE, window and tray share the icon. Source: `assets/icon.svg`; multi-size icon: `assets/icon.ico`; embedded group resource: 101. `assets/generate-icon.ps1` regenerates the PNG, ICO and `.res` using Windows PowerShell 7/System.Drawing. `build.ps1` links the resource; CMake builds `assets/app.rc`.

Run `./build.ps1` with MinGW clang++ (the script detects Windhawk's bundled compiler), or CMake with Visual Studio and the Windows SDK. One EXE is produced. Ordinary window attributes do not require Windhawk, but the two specialized rendering backends do.

`--self-test` runs independently of the existing app. It checks bilingual actions, main/settings page visibility, language placement, switch ordering/heights, icon loading, v1/v2 plate backup compatibility, attribute caching, native target scope and local DWM restoration. It does not change live system styles. The report is `self-test.txt` beside the EXE. The old full-theme `--shell-apply` / `--shell-restore` arguments remain rejected.

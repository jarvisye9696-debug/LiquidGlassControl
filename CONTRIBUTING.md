# Contributing / 参与开发

This repository is currently private and has no open-source license. Repository access does not grant a general permission to redistribute the project. Agree on a license before a public release.

仓库目前为私有，暂未选择开源许可证。公开发布前，请维护者一起确认许可证与发布范围。

## Work together

1. Clone this repository using your own GitHub account. Each contributor keeps their own checkout.
2. Create an Issue to describe the problem or proposed behavior and agree who will work on it.
3. Update `main`, then create one branch per change, for example `fix/settings-tabs` or `feature/native-progress-motion`.
4. Make a focused change, build it and run the self-test.
5. Push the branch and open a pull request targeting `main`. Describe the behavior, validation and remaining limitations.
6. Ask the other maintainer to review. Merge after the Windows build passes, then delete the feature branch.

每人一个本地目录、每项改动一个分支，通过 PR 讨论和合并。不要共用账号或密码。合并前请另一位维护者检查并等待自动构建通过。

```powershell
git switch main
git pull --ff-only
git switch -c fix/describe-your-change
# Edit, build and test.
git add <changed-files>
git commit -m "Describe the behavior change"
git push -u origin HEAD
```

## Build on Windows

Requirements: Windows 11 x64, CMake 3.20+, Visual Studio 2022 Build Tools with Desktop development with C++ and the Windows SDK. Windhawk is optional for compiling and for the self-test, but is required for specialized live effects.

```powershell
cmake -S . -B build/cmake -A x64
cmake --build build/cmake --config Release
$test = Start-Process build/cmake/Release/LiquidGlassControl.exe -ArgumentList '--self-test' -PassThru -Wait
if ($test.ExitCode -ne 0) { throw 'Self-test failed' }
Get-Content build/cmake/Release/self-test.txt
./package.ps1 -ExecutablePath build/cmake/Release/LiquidGlassControl.exe
```

Alternatively, use `./build.ps1` with MinGW clang++ (the installed Windhawk compiler is detected), then `./package.ps1 -SkipBuild`.

## Project map

| File | Responsibility |
|---|---|
| `main.cpp` | Window lifecycle, settings, applying/restoring effects, self-test |
| `control_ui.h` | Controls and Chinese/English captions |
| `ui_style.h` | Fonts and GDI rendering |
| `native_osd.h`, `native_switches.h` | Native volume/brightness styles and restoration journals |
| `explorer_background.h` | Explorer-specific styles and restoration |
| `package.ps1`, `release/` | Clean portable distribution and user instructions |

## Validation

- Keep every user-facing caption paired in Chinese and English; first launch defaults to English, saved choices persist.
- Do not change taskbar layout, switch its theme, intercept media keys, or add a duplicate volume/brightness overlay.
- Native styling must remain scoped to the native indicators and support restoration. Keep users' recovery journals.
- The self-test uses local fixtures and temporary files; it does not apply live system styles. DWM assertions are skipped where unsupported.
- Manually verify UI changes on Windows 11. For live effects, test apply and restore, independent On/Off states and reduced-motion behavior. Record Windows build and relevant Windhawk mod versions.
- Do not describe entry transitions as smooth numeric progress animation. The latter is not implemented yet.
- Do not commit personal INI files, recovery backups, local logs, `build/` or `dist/`.

## Releases

See [RELEASING.md](RELEASING.md). Pull requests build/test/package on Windows. Version tags create a **draft** GitHub release; maintainers inspect its assets before publication. While this repository is private, release downloads also require access.

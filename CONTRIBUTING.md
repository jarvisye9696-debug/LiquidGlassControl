# Contributing / 参与开发

The project owner's and DK's original contributions are licensed under GPL-3.0-only. See LICENSE and docs/LICENSING.md for scope and third-party notices. The owner confirmed DK's agreement on 2026-10-16; other copyright holders must authorize any additional contributions.

项目所有者与 DK 的原创贡献采用 GPL-3.0-only，范围和第三方许可证见 docs/LICENSING.md。所有者已于 2026-10-16 确认 DK 同意；后续其他权利人的贡献仍需取得授权。新增贡献须遵守 docs/LICENSING.md。

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

Requirements: Windows 11 x64, CMake 3.20+, Visual Studio 2022 Build Tools with Desktop development with C++ and the Windows SDK. Windhawk is not required for compilation, self-tests or the experimental standalone engine.

```powershell
./prepare-winrt.ps1
cmake -S . -B build/cmake -A x64
cmake --build build/cmake --config Release
$test = Start-Process build/cmake/Release/LiquidGlassTests.exe -ArgumentList '--self-test' -PassThru -Wait
if ($test.ExitCode -ne 0) { throw 'Self-test failed' }
Get-Content build/cmake/Release/self-test.txt
./package.ps1 -ExecutablePath build/cmake/Release/LiquidGlassControl.exe
```

Alternatively, use `./build.ps1 -BuildTests` with MinGW clang++ (the installed Windhawk compiler is detected), then `./package.ps1 -SkipBuild`.

## Project map

| File | Responsibility |
|---|---|
| `src/main.cpp` | Window lifecycle, settings, applying/restoring effects, self-test |
| `src/control_ui.h` | Controls and five-language captions |
| `src/ui_style.h` | Fonts and GDI rendering |
| `src/native_osd.h`, `src/native_switches.h` | Native volume/brightness styles and restoration journals |
| `src/shell_materials.h`, `src/shell_materials_test.h` | Independent shell background rules, journals and isolated registry tests |
| `src/explorer_background.h` | Explorer-specific styles and restoration |
| `package.ps1`, `release/` | Clean portable distribution and user instructions |

## Validation

- Keep every user-facing caption in all five languages; first launch defaults to English, saved choices persist.
- Glass rules preserve taskbar geometry. Dock transforms icons visually. Smart hide has been removed; retain the legacy taskbar-state recovery path. Do not intercept media keys or add duplicate indicators.
- Preserve custom styles and recovery journals; restore original theme and module activation state when Off.
- The self-test uses local fixtures and temporary files; it does not apply live system styles. DWM assertions are skipped where unsupported.
- Manually verify UI changes on Windows 11. For live effects, test apply and restore, independent On/Off states and reduced-motion behavior. Record Windows build and whether other styling tools were running. Include memory in injected processes.
- Do not describe entry transitions as smooth numeric progress animation. The latter is not implemented yet.
- Do not commit personal INI files, recovery backups, local logs, `build/` or `dist/`.

## Releases

See [RELEASING.md](docs/RELEASING.md). Pull requests build/test/package on Windows. Version tags create a **draft** GitHub release; maintainers inspect its assets before publication. While this repository is private, release downloads also require access.

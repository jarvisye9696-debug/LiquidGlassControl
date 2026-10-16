# Releasing / 发布

The repository and v0.9 preview downloads are public. No open-source license has been selected. Workflows create drafts for maintainer review; publishing a release makes its attached downloads public.

1. Merge the intended changes to `main`; confirm Windows CI is green.
2. Update the version in `package.ps1`, app captions/About and release instructions together. Do not reuse an existing tag.
3. Build and verify on Windows 11. Test both UI languages and live effect restoration. CI self-tests do not verify Windhawk integration or every app's compatibility.
4. Commit version changes, then push a tag matching the package version, for example `v0.9`:

   ```powershell
   git switch main
   git pull --ff-only
   git tag -a v0.9 -m "Liquid Glass Control 0.9"
   git push origin v0.9
   ```

5. The Release workflow builds and tests the tagged source, packages only allowlisted files and creates a **draft** release with EXE, ZIP and SHA256SUMS.txt attached.
6. Open the draft, download/extract the ZIP, and inspect release notes, version, dependencies and limitations. Publish when ready. Downloads remain restricted while the repository is private.

For a manual package:

```powershell
./package.ps1
# Or use a CMake-built executable:
./package.ps1 -ExecutablePath build/cmake/Release/LiquidGlassControl.exe
```

Never upload your personal `settings.ini`, original-style backups or test logs as release assets. Windhawk is not bundled. Preserve the guide's dependency and compatibility notes. Packages are currently unsigned.

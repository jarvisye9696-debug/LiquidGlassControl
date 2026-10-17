# SignPath application preparation

Status: draft only. No application has been submitted and no signing service has been approved.

## Public project details

- Project: Liquid Glass Control
- Repository/homepage: https://github.com/jarvisyip9696/LiquidGlassControl
- Downloads: https://github.com/jarvisyip9696/LiquidGlassControl/releases
- Tagline: A portable Windows 11 utility for configurable glass effects on the desktop shell.
- Build system: GitHub Actions, Windows-hosted runners, CMake and MSVC.

Description:

Liquid Glass Control is an experimental Windows 11 x64 desktop customization utility. Users can enable glass effects for the taskbar, Start menu, notifications, File Explorer, volume and brightness surfaces, and restore changes through the application. It contains an embedded shell engine, adapted Windhawk Styler code and MinHook. We request review of the imported-code eligibility before signing the engine and controller.

## Required before submission

- The owner authorized GPL-3.0-only for their original contributions and confirmed DK's agreement on 2026-10-16. LICENSE and LICENSING.md record the scope; retain third-party licenses. Additional contributors must authorize their own work.
- Retain upstream notices and explain the adapted Windhawk code to SignPath. Its rules on signing modified upstream software require review; eligibility is not established.
- Supply actual reputation evidence (release download counts, community discussions or independent coverage). Do not invent adoption claims.
- Confirm authors, reviewers and release signing approvers, and enable MFA on every maintainer's repository and SignPath accounts.
- Publish a code signing policy on the homepage and release/download pages. Include the provider attribution only with an accurate statement of application/approval status.
- Review the privacy statement before publishing it. Current source inspection found no network transfer API in project-owned runtime code; XAML namespace URLs are identifiers. Build dependencies are downloaded by prepare-winrt.ps1, separately from application runtime.
- Confirm applicant name, contact email and maintainer type with the applicant. These fields were not copied from an unrelated signed-in account.
- Review the form's Code of Conduct and personal-data consent with the applicant before acceptance and submission. Optional marketing consent is not required.

## Signing integration after approval

The current engine is embedded as RCDATA resource 201. Signing the outer EXE alone leaves the extracted engine unsigned.

1. Add matching ProductName and ProductVersion metadata to both PE files; enforce these values in SignPath artifact restrictions.
2. Build GlassEngine.dll on the approved CI system and upload that artifact for a manually approved signing request.
3. Retrieve and verify the signed DLL, then embed its exact bytes into the controller EXE.
4. Upload the controller for a second manually approved signing request; retrieve and verify its signature.
5. Verify that the embedded engine bytes match the signed DLL. Package the signed EXE, then regenerate SHA256SUMS.txt.
6. Test Smart App Control and actual Windows shell effects on a live device. A signature is not evidence that the effects work.

Do not enable a production signing workflow with invented organization IDs, credentials or artifact configurations. The current workflows still produce unsigned packages.

## Official references

- Application: https://signpath.org/apply
- Conditions: https://signpath.org/terms
- Current build workflow: .github/workflows/windows.yml
- Current release workflow: .github/workflows/release.yml

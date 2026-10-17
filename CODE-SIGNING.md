# Code signing policy

## Status

An application has been submitted to SignPath Foundation; its receipt was acknowledged by email. Review is pending. No certificate or signing approval has been obtained, and the existing downloads remain unsigned. This page does not claim sponsorship or endorsement.

The requested service is free code signing provided by SignPath.io, with a certificate by SignPath Foundation, subject to approval. After approval, the attribution and release verification instructions will be updated to reflect the active service.

## Team and release approval

- Author and reviewer: [Jarvis Yip (@jarvisyip9696)](https://github.com/jarvisyip9696).
- Contributor: [DK (@domk6966)](https://github.com/domk6966). Contribution credit alone does not confer signing authority.
- Release signing approver: Jarvis Yip (@jarvisyip9696).

All team members must use MFA for repository and SignPath access. Changes from contributors without direct commit rights require maintainer review. Every production signing request must receive manual approval and must use artifacts built from the reviewed source and build scripts by the approved CI system. No unrelated upstream binaries may be signed under this project's identity.

## Privacy

Liquid Glass Control does not transfer user information to other networked systems unless the user or person operating it specifically requests this. Settings and restoration files are stored locally beside the executable. External tools and websites have their own privacy policies; the public preview uses separately installed Windhawk for some effects. See [Windhawk](https://windhawk.net/) for that separate product. Future runtime or dependency changes must be reviewed against this statement.

## System changes and removal

The program changes Windows appearance and can adjust supported Windhawk theme settings. Review the Quick Start supplied with the relevant release. Before removing the portable folder, use **Settings → About → Restore and exit**, then remove the folder and any user-created shortcut. Retain recovery files until restoration succeeds.

## Signing and verification plan

The public preview and the experimental standalone-engine branch have different build layouts. Signing the preview must not be presented as proof that an experimental engine DLL is signed or operational.

For a future build containing an embedded engine: sign and verify the engine DLL first, embed those exact bytes, then sign and verify the final EXE. Use matching product/version metadata and enforce it through the signing configuration. Package only approved files, preserve license notices and corresponding source, and calculate checksums after signing. Validate Smart App Control and actual shell effects on Windows 11 separately.

## References

- [SignPath Foundation conditions](https://signpath.org/terms)
- [Licensing scope](LICENSING.md)
- [Build and release instructions](RELEASING.md)

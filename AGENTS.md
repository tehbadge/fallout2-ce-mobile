# Engineering guidance

- Preserve Android and desktop functionality; isolate iOS-specific changes wherever practical.
- Reuse the shared C++/SDL2 engine and existing touch UI. Minimize divergence from `imbaner/fallout2-ce-mobile` and upstream merge conflicts.
- Avoid unrelated refactoring and unnecessary abstractions.
- Distinguish successful compilation from verified runtime functionality; state what was actually tested and on which platform.
- Preserve installed application data, saves, and app identity by default, including the bundle identifier and Documents directory during development deployments. Deletion or replacement of protected application data or saves, or changes to application identity, require explicit authorization for the affected target.
- Never commit copyrighted game assets, credentials, provisioning files, or generated build artifacts.
- Maintain concise, current project status in `docs/ios-port/STATUS.md`.

## Verification and current state

- Before relying on material technical facts or making changes, inspect relevant source, configuration, environment, or other authoritative evidence. Documentation, previous reports, prompts, and remembered configuration provide context, not automatic proof of current conditions.
- Before modifying files, establish the current working branch and existing changes; preserve work outside the authorized scope.
- Distinguish directly verified results from historical reports, assumptions, and unverified conditions. Investigate material discrepancies instead of silently resolving them through assumptions.
- Verify proportionally to risk, with particular scrutiny for publication, deployment, deletion, overwriting, credentials, and infrastructure operations.
- Stop and report when consequential targets cannot be verified, material evidence conflicts, or the environment differs unexpectedly. Do not improvise unauthorized corrections.

## Repository identity and publication safety

- Original public upstream: `imbaner/fallout2-ce-mobile`. Downstream contributors must distinguish their own development repositories from upstream and independently verify repository identity and visibility. Names and documented roles are not authorization to publish.
- Before pushing, independently verify the actual destination URL, repository visibility, intended branch/ref, and safety for the remote's current state. Never infer publication eligibility from a remote name, including `origin` or `backup`.
- Never use public upstream for routine development backups. Publishing there requires explicit authorization identifying that destination. A development backup also requires authorization and verification of its intended repository and visibility; a remote named `backup` is not permission to push.
- Do not force-push, rewrite remotes, or redirect publication to another repository without explicit authorization. Stop and report unexpected destinations or repository state instead of silently repairing them.

## Validation and reporting

- Prioritize iOS builds, relevant automated tests, and practical physical-device validation.
- Preserve Android compatibility, but do not routinely attempt Android APK builds or treat an unavailable Android toolchain as a blocker.
- Investigate Android regressions when changes present a material compatibility risk or when explicitly requested.
- Treat user-confirmed hands-on testing as valid physical-device functional acceptance.
- Do not require exact interaction counts, exhaustive test matrices, or repeated confirmations unless investigating intermittent problems or explicitly requested.
- Accurately distinguish automated tests, simulator results, and user-verified physical testing. Never invent results or imply unperformed tests were completed.
- Keep reports concise and focused on meaningful results, actual defects, regressions, unresolved risks, and next actions. Avoid repeatedly reporting routine optional tests that were not performed.

## Upstream compatibility

- Preserve shared Git history with `imbaner/fallout2-ce-mobile`.
- Keep iOS changes organized as focused, reviewable commits suitable for potential upstream contribution.
- Minimize divergence from the Android implementation.
- Treat development backups as backups, not replacements for the original upstream repository.

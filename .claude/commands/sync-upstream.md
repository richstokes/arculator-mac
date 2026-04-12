# Sync Fork with Upstream

Merge the latest changes from the upstream repo into this fork without breaking our local changes.

## Steps

1. Fetch the latest from `upstream` remote (`git fetch upstream`).
2. Check what commits upstream has that we don't (`git log --oneline master..upstream/master`).
3. Merge `upstream/master` into the current branch with a descriptive merge commit message.
4. If there are merge conflicts, resolve them — prefer our local changes for macOS-specific code (anything with `#ifdef __APPLE__` guards), and accept upstream changes for everything else.
5. Run `bash mac-install.sh` to build and verify nothing is broken.
6. If the build fails, fix the issue (common problem: upstream C code missing `#include <stdlib.h>` or similar headers that macOS clang requires but Linux gcc doesn't).
7. Rebuild and confirm success before finishing.

## Notes

- The `upstream` remote should already be configured pointing to `https://github.com/sarah-walker-pcem/arculator`. If it's not, add it.
- Do NOT force-push or rebase — use merge to preserve our commit history.
- Do NOT push to origin unless explicitly asked.
- When fixing build issues, wrap macOS-specific changes in preprocessor guards so the code still compiles on other platforms:
  ```c
  #ifdef __APPLE__
  #include <stdlib.h>
  #endif
  ```
  Only use these guards when the fix is truly macOS-specific. If the fix is universally correct (e.g. a missing include that all platforms need), apply it unconditionally.
- Do NOT push to origin unless explicitly asked.

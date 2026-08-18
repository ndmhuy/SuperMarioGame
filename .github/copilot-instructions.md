# Project Agent Rules

**All agent rules for this repository live in [AGENTS.md](AGENTS.md). Read that file before doing anything else.**

This file exists only so tools that look for a different filename still find the
rules. It is a pointer, not a second source of truth — never add rules here, and
never let this file and `AGENTS.md` disagree.

## The four that get broken most

1. **`git fetch --all` before you read or report on repository state.** A local
   branch is not evidence. Check `git rev-list --left-right --count dev...origin/dev`.
2. **Append a full entry to `logs/agent_history.log` every session**, including
   the `Git Fingerprint`, `Fetched Remotes`, `Reachable From Main` and
   `Verified By` fields. Report honestly; `build only` is a fine answer.
3. **Never discard uncommitted work to unblock a git operation.** Commit it —
   committing is reversible, discarding is not.
4. **"Complete" means reachable from `main()` and observed running**, not
   "compiles and a `verify_*` harness constructs it."

See [AGENTS.md](AGENTS.md) for the full set, the log template, and the incident
history behind each rule.

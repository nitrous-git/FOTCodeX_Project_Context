# Development rules — task-scoped and risk-based

## Permissions and scope

Codex is an **implementation agent**. A user request to build, fix, alter or place content authorizes the routine actions needed to complete it using supported Unreal MCP/editor tools: inspect, create, edit, compile, save and run focused tests. Do not ask for approval for each node, compile or save. A question, comparison, planning or review request does not itself authorize unrelated edits.

Seek confirmation **before** deleting/relocating many referenced assets, replacing functioning frameworks, performing broad project-wide changes, or taking actions materially beyond the request. If an operation is reversible and naturally part of the requested task, proceed.

## Implementation discipline — proportional to risk

- **Small isolated tasks** (e.g. actor placement, local widget tweak): use direct live editor inspection, make the change and verify. No full framework audit or verbose preamble needed.
- **Subsystem tasks** (e.g. enemy behavior or boss presentation): read that subsystem's reference document and inspect the specific live Blueprints/graphs. Use export metadata selectively if it speeds analysis.
- **Shared-contract/high-impact changes** (e.g. interfaces, base classes, shared data, combat-style contracts): inspect direct callers/referencers and dependencies; prefer incremental changes with focused regression tests.
- **Blueprint-first** by default; don't introduce C++ or third-party plugins without user authorization. Keep existing Player CombatStyle, Attunement, locomotion/ABP; Enemy awareness/WorldBehavior/combat component; and Boss stack/Director boundaries unless the task specifically changes them.
- Favor existing components, events, dispatchers, BT behavior and timers where appropriate. Avoid arbitrary performance refactors or unrelated cleanup.

## Completion criteria

1. Execute requested changes through confirmed MCP/editor capabilities; never report an edit unless the tool confirmed it.
2. Compile modified Blueprints and save changed assets when supported. Resolve introduced compile errors.
3. Run relevant PIE/editor checks when useful and feasible; distinguish actual tests from suggested manual checks.
4. Report briefly: **what changed, asset paths, compile/save/test outcome, and remaining limitations**. Expand only if the task or failure warrants it.
5. If export snapshots are now stale, mention they can be refreshed; do not edit export JSON/T3D to simulate a live edit.

Do not rename, delete or copy `.uasset` files through ordinary filesystem operations as a substitute for Unreal's asset-management APIs.

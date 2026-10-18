# Codex + Unreal MCP workflow

## Working environment

- **Real UE 5.8.3 project:** active Codex local workspace and running editor; current `.uasset` state is authoritative.
- **Native Unreal MCP:** editor query and supported asset/graph/level operations. Discover actual tools before relying on them.
- **FOTCodeX_Project_Context:** separate local Git clone with reusable Markdown and exported Blueprint JSON snapshots.
- **Bridge instructions:** `Templates/UnrealProject_AGENTS.md` is copied/renamed to `AGENTS.md` at the real Unreal project's root. It points to the context clone. Merely pointing to a path does not grant filesystem permissions.

The user validated live MCP inspection when Codex correctly identified selected actor `BP_Pickup_HPPotion_C_1`. This establishes live read connectivity, **not** a guarantee that every mutation tool or compile operation is available. Keep the already-working `.codex/config.toml`; do not register a duplicate MCP server without a concrete issue.

## Efficient task loop

1. Read the applicable `AGENTS.md`; interpret the user's goal (investigation vs development).
2. Read a **relevant** reference doc or exact export graph **only when needed**. Do not preload every subsystem or run initialization diagnostics repeatedly.
3. Inspect the corresponding live asset/level through MCP; search or fetch specific graphs/metadata instead of entire Blueprint dumps.
4. For implementation tasks, make required changes and verify MCP tool responses; do not ask for separate permission for routine operations.
5. Compile, save and run applicable targeted checks using available tools. If unavailable, state what remains unverified.
6. Summarize the result succinctly. Propose exporter refresh only when changes materially make the snapshot stale.

## Escalation by risk

- Straightforward editor operations: execute and check outcome.
- Unfamiliar graph editing tool: start with a controlled operation or disposable asset when prudent, then proceed.
- Changes to widely referenced Blueprints/interfaces: inspect dependency/referencer paths and test consumers.
- Destructive or out-of-scope actions: get confirmation first.

## Common failure modes

- **MCP present but no responses:** verify Unreal Editor server is running on the configured localhost URL, and check editor logs.
- **Cannot read context clone:** inspect actual path and Codex workspace permissions; don't confuse GitHub remote access with local sandbox access.
- **Live graph differs from snapshot:** favor live Unreal state; use the export to locate references, not overwrite the editor.
- **Unsupported write/compile tool:** report the exact limitation; use supported editor procedures or provide exact manual steps instead of claiming the work was completed.

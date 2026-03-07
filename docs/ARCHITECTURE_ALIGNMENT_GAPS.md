# Architecture Alignment Gaps and Next Steps

This document tracks remaining ambiguities after the layered consensus clarification pass.

## Unresolved ambiguities found

1. **Governance wording vs consensus wording**
   - Governance documentation still uses broad terms such as "all token holders" and "all layers" in some contexts.
   - These are governance participation terms, not settlement-security terms, but readers can still confuse them with consensus scope.

2. **Legacy PoW argument aliases in tooling**
   - Some relayer/operator surfaces retain legacy alias support (for backward compatibility).
   - Canonical language is validator/stake based, but mixed terminology can still appear in scripts or operational notes.

3. **Model coverage boundaries**
   - `configs/layer-model.json` is currently authoritative for layer naming, role hints, and checkpoint ordering.
   - It does not yet encode all higher-level assumptions (e.g., bridge trust windows, minimum confirmation policy guidance).

## Places where code behavior and docs may still diverge

1. **Backward compatibility flags vs canonical docs**
   - Code may accept legacy flags (e.g., PoW-era aliases), while docs recommend validator/stake terminology.
   - This is intentional for compatibility, but creates temporary dual semantics.

2. **Operational defaults vs policy recommendations**
   - Security docs recommend conservative TALANTON confirmation thresholds for high-value settlement.
   - Runtime/default configs do not yet enforce an explicit mandatory confirmation-depth policy across all operator scripts.

3. **Cross-doc consistency enforcement**
   - CI validates config/model consistency, but does not yet lint prose-level terminology across all Markdown files.
   - As a result, a future doc change could reintroduce ambiguous wording without failing CI.

## Recommended next implementation steps

### Documentation completed now

- Layer roles, settlement hierarchy, and finality semantics are documented and aligned across `README.md`, `WHITEPAPER.md`, architecture/consensus/security/threat docs, and settlement/finality reference docs.
- Canonical checkpoint route and trust assumptions are explicitly documented.
- A machine-readable layer model exists and is validated in CI.

### Code-level improvements for later

1. **Formal deprecation plan for legacy CLI aliases**
   - Introduce deprecation warnings for legacy PoW-era flags.
   - Remove aliases in a scheduled major release after migration window.

2. **Policy enforcement in code/config**
   - Add explicit, validated confirmation-depth settings for TALANTON settlement actions in operator configs/scripts.
   - Tie bridge unlock automation to configurable minimum L1 depth thresholds.

3. **Terminology linting in CI**
   - Add a lightweight docs terminology check (e.g., script that rejects known deprecated phrases in consensus-critical docs).
   - Scope it to consensus/security/architecture docs to reduce false positives.

4. **Single-source architecture metadata expansion**
   - Extend `configs/layer-model.json` (or companion model) with optional, non-consensus policy metadata:
     - recommended L1 confirmation windows,
     - checkpoint freshness SLOs,
     - relayer liveness thresholds.

5. **Relayer interface standardization**
   - Normalize relayer and CLI interfaces so operator-facing options and help text are fully aligned with `validator_set_hash` / `validator_signatures` semantics in every layer tool.

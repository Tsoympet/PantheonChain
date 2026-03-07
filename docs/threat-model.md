# Threat Model (MVP)

## Security Baseline

PantheonChain MVP uses layered checkpointing with the canonical path:

`OBOLOS -> DRACHMA -> TALANTON`

This model is **not** a trustless-validity bridge design in MVP.

## Consensus and Finality Assumptions

- **TALANTON (L1):** PoW chain with probabilistic finality and the strongest settlement assurances.
- **DRACHMA (L2):** PoS/BFT economic finality under validator honesty threshold assumptions.
- **OBOLOS (L3):** PoS/BFT economic finality under validator honesty threshold assumptions.
- **Relayers:** liveness infrastructure for checkpoint publication; not consensus roots.

## Trust Boundaries

- TALANTON is the root trust-minimized settlement layer.
- DRACHMA and OBOLOS can provide fast finality, but settlement assurances strengthen only after downstream checkpoint inclusion.
- Checkpoint and bridge safety depends on validator-signature correctness and honest-majority assumptions unless additional proof systems are implemented.

## Primary Risks

1. **Malicious OBOLOS validator supermajority**
   - Can finalize invalid or conflicting L3 state from the perspective of honest minority validators.
   - Damage can propagate to DRACHMA if not detected before checkpoint acceptance.
2. **Malicious DRACHMA validator supermajority**
   - Can publish unsafe L2 commitments and mis-anchor L3 references until challenged operationally.
   - Affects L2 safety and any L3 state relying on L2 checkpoint integrity.
3. **Relayer censorship or liveness failure**
   - Delays publication of otherwise valid checkpoints.
   - Does not directly rewrite TALANTON consensus, but extends economic-finality risk windows.
4. **TALANTON reorg risk**
   - Deep reorgs can delay certainty of upper-layer settlements that depend on recent L1 anchors.
   - Operators should enforce conservative confirmation windows for high-value exits.

## MVP Limitations

- No zk-proofs.
- No fraud proofs.
- No claim of trustless bridging.
- Commitment validation is signature/quorum and payload-check based.
- Bridge withdrawals remain optimistic and include configurable trust windows.

## Operator Guidance

- Monitor commitment freshness across both boundaries (OBOLOS->DRACHMA and DRACHMA->TALANTON).
- Use hardened validator key management and quorum monitoring for PoS/BFT layers.
- Apply conservative TALANTON confirmation policies for large settlements and bridge unlocks.

# Consensus Specification

PantheonChain uses a **hybrid layered consensus model**, not a single shared consensus engine across all tokens.

## TALANTON (L1 / PoW settlement)

- Consensus algorithm: SHA-256d Proof-of-Work.
- Finality model: probabilistic finality under heaviest-valid-chain rules.
- Role: root settlement chain and final anchoring destination for upper layers.
- Native token role: PoW mining rewards, L1 fees, and base settlement utility.
- TALANTON consensus is independent of DRACHMA/OBOLOS validator quorums.

### `TX_L2_COMMIT` validation rules on TALANTON

`TX_L2_COMMIT` must satisfy:
1. `source_chain == DRACHMA`
2. `finalized_height` is strictly monotonic relative to prior accepted L2 anchors.
3. Payload fields are correctly encoded.
4. Validator signatures represent >=2/3 of declared DRACHMA active stake for the commitment epoch.

## DRACHMA (L2 / PoS + BFT payments)

- Consensus algorithm: PoS validator set with BFT-style quorum finalization.
- Finality model: fast economic finality when quorum assumptions hold.
- Role: payments, liquidity, and intermediate checkpoint layer to TALANTON.
- Native token role: staking collateral, validator incentives, and L2 fee utility.
- Security caveat: upper-layer safety can fail before TALANTON settlement if L2 validator assumptions break.

All DRACHMA commitments anchored to TALANTON include a state commitment reflecting the latest finalized OBOLOS commitment hash (`upstream_commitment_hash`) to preserve transitive anchoring.

### `TX_L3_COMMIT` validation rules on DRACHMA

`TX_L3_COMMIT` must satisfy:
1. `source_chain == OBOLOS`
2. `finalized_height` is strictly monotonic against last accepted L3 commitment.
3. Payload fields are correctly encoded.
4. Validator signatures represent >=2/3 of declared OBOLOS active stake for the commitment epoch.

## OBOLOS (L3 / PoS + BFT + EVM)

- Consensus algorithm: PoS validator set with BFT-style quorum finalization.
- Finality model: fast economic finality for execution outcomes.
- Role: EVM execution, governance state transitions, and DeFi/application layer.
- Native token role: gas, staking, and governance/economic utility on L3.
- Finalized OBOLOS checkpoints are exported to DRACHMA as commitment payloads.

## Governance voting note

Governance weighting, if configured via one-address-one-vote in current deployments, is a governance-layer rule and is not equivalent to consensus safety assumptions. Consensus safety for DRACHMA/OBOLOS remains tied to validator quorum honesty, while TALANTON safety remains tied to PoW security.

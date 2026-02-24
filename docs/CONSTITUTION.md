# PantheonChain Governance Constitution

> *"The secret of happiness is freedom, and the secret of freedom is courage."*
> — Thucydides, *History of the Peloponnesian War*

---

## Preamble

We, the validators, stakers, and builders of PantheonChain, establish this Constitution to govern the protocol through transparent, rules-bound, and participatory mechanisms inspired by the democratic institutions of ancient Athens.

This Constitution encodes the following founding principles:

| Greek Term | Meaning | Blockchain Expression |
|------------|---------|----------------------|
| **Isonomia** (ἰσονομία) | Equality before the law | All governance parameters subject to hard-coded constitutional floors and ceilings |
| **Isegoria** (ἰσηγορία) | Equal right of speech | Any address meeting the minimum stake threshold may submit proposals |
| **Demokratia** (δημοκρατία) | Power of the people | Staker assembly (Ekklesia) is the sovereign decision-making body |
| **Sophrosyne** (σωφροσύνη) | Prudence and self-restraint | Veto threshold and supermajority requirements protect the minority |
| **Eunomia** (εὐνομία) | Good order and lawfulness | Proposal pipeline enforces mandatory review periods and execution delays |

These principles are not aspirational; they are enforced in code. The smart contracts implementing this Constitution reside in `layer3-obolos/governance/` and include `Boule.sol`, `VotingSystem.sol`, `Treasury.sol`, `Staking.sol`, `Ostracism.sol`, and `EmergencyCouncil.sol`.

---

## Article I: The Council — *Boule* (βουλή)

### Section 1.1 — Composition and Size

The Boule consists of **500 seats** distributed across validator nodes on Layer 3 (OBOLOS). The seat count may be adjusted only by a CONSTITUTIONAL proposal (Article III, Section 3.3) within the bounds set by Article V.

### Section 1.2 — Sortition (*Kleroteria*)

Council members are selected by **Verifiable Random Function (VRF) sortition** at the start of each epoch. The VRF seed is derived from the OBOLOS block hash of the last block of the preceding epoch. No validator may influence, predict, or manipulate selection outcomes.

- Eligible validators are placed in a weighted lottery where weight equals their effective staked balance.
- VRF output is published on-chain and verifiable by any party before the new epoch begins.
- Implementation: `Boule::selectCouncil(vrfSeed, epochNumber)` in `layer3-obolos/governance/Boule.sol`.

### Section 1.3 — Term Limits

- Each council term spans **one epoch** (configurable; default 14 days).
- A validator may serve in at most **4 consecutive terms** before a mandatory one-term rest period.
- Term history is tracked on-chain in `Boule::termHistory`.

### Section 1.4 — The Executive Committee (*Prytany*)

- At the start of each epoch, **50 Boule members** are randomly selected as the *Prytany* (πρυτανεία).
- The Prytany holds the keys to fast-track EMERGENCY proposals (see Article III, Section 3.4).
- A *Epistates* (ἐπιστάτης, presiding officer) is randomly chosen from the Prytany daily.
- The Epistates may not serve twice within the same Prytany term.

### Section 1.5 — Eligibility Screening (*Dokimasia*)

Before a validator may be added to the Boule pool, they must pass on-chain Dokimasia (δοκιμασία):

1. **Minimum stake**: At least `MIN_COUNCIL_STAKE` tokens staked (see Article V for the constitutional floor).
2. **Slashing record**: No slashing event in the past 4 epochs.
3. **Uptime**: Block production uptime ≥ 90 % over the past epoch.
4. **Declaration**: Submission of a signed attestation that the operator is not under active Ostracism (Article VIII).

Dokimasia is enforced by `Boule::dokimasia(validatorAddress)`.

### Section 1.6 — Removal

A Boule member may be removed mid-term by a supermajority (66 %) vote of the full assembly if:
- They are found to have submitted fraudulent VRF proofs.
- They are convicted via the Ostracism mechanism.
- They fall below the minimum stake threshold due to slashing.

---

## Article II: The Assembly — *Ekklesia* (ἐκκλησία)

### Section 2.1 — Membership

All addresses with a positive staked balance on Layer 3 (OBOLOS) at the snapshot block of a given proposal are members of the Ekklesia for that proposal. There is no minimum stake required to vote; the minimum stake requirement applies only to proposal submission (see Section 2.2).

### Section 2.2 — Proposal Submission Rights

Any address satisfying ALL of the following conditions may submit a governance proposal:

1. Staked balance ≥ `MIN_PROPOSAL_STAKE` at the time of submission.
2. Not under active Ostracism (Article VIII).
3. No pending unexecuted proposal from the same address (one-proposal-at-a-time rule).

Implementation: `VotingSystem::submitProposal(proposalType, calldata, description)`.

### Section 2.3 — Quorum

Each proposal type has its own quorum requirement expressed as a percentage of total staked supply at the snapshot block:

| Proposal Type | Quorum |
|--------------|--------|
| STANDARD | 10 % |
| CONSTITUTIONAL | 20 % |
| EMERGENCY | 5 % (Prytany-only initial vote) |
| PARAMETER_CHANGE | 10 % |
| TREASURY_SPENDING | 15 % |

Proposals that fail to reach quorum are marked `DEFEATED_QUORUM` and cannot be re-submitted for a 7-day cooling-off period.

### Section 2.4 — Assembly Sessions

The Ekklesia holds continuous asynchronous sessions on-chain. There is no scheduled meeting calendar; proposals may be submitted at any time, subject to a global rate limit of `MAX_CONCURRENT_PROPOSALS` (see Article V).

---

## Article III: Proposal Types

### Section 3.1 — STANDARD

- **Threshold**: Simple majority (> 50 % of non-abstaining votes).
- **Voting window**: 7 days.
- **Execution delay**: 2 days after passage (allows community to react before on-chain execution).
- **Use cases**: Minor parameter tweaks within existing bounds, cosmetic contract updates, non-binding signals.

### Section 3.2 — PARAMETER_CHANGE

- **Threshold**: Simple majority (> 50 %).
- **Voting window**: 7 days.
- **Execution delay**: 3 days.
- **Use cases**: Adjusting protocol parameters (fee rates, epoch length, slashing penalties) within the Isonomia bounds defined in Article V.
- All proposed values are validated against Article V bounds before the proposal is accepted.

### Section 3.3 — CONSTITUTIONAL

- **Threshold**: Supermajority (≥ 66 % of non-abstaining votes).
- **Voting window**: 14 days.
- **Execution delay**: 7 days.
- **Use cases**: Amending this Constitution, changing the Boule size, altering quorum thresholds, modifying constitutional bounds (Article V).
- CONSTITUTIONAL proposals may NOT move any Article V hard floor or ceiling beyond the absolute limits defined in `GovernanceConstants.sol`.

### Section 3.4 — EMERGENCY

- **Initial authorization**: Prytany supermajority (≥ 34 of 50 members).
- **Assembly ratification**: Full Ekklesia vote within 72 hours (simple majority required to sustain).
- **Execution TTL**: Action executes immediately upon Prytany authorization; if assembly vote rejects or times out, a rollback transaction is triggered where technically feasible.
- **Apophasis review**: The Apophasis board (Article IX, Section 9.3) must publish a post-action review within 7 days.
- **Use cases**: Critical security patches, halting compromised contracts, responding to oracle manipulation attacks.

### Section 3.5 — TREASURY_SPENDING

- **Threshold**: Simple majority (> 50 %).
- **Voting window**: 10 days.
- **Execution delay**: 3 days.
- **Use cases**: Disbursing funds from a Treasury track (Article VI).
- Proposals must specify: recipient address, amount, Treasury track, and a milestone schedule if the amount exceeds `LARGE_GRANT_THRESHOLD` (see Article V).

---

## Article IV: Voting

### Section 4.1 — Vote Options

Each voter may cast exactly one of the following:

| Option | Weight |
|--------|--------|
| **YES** | Counts toward passage |
| **NO** | Counts against passage |
| **ABSTAIN** | Counts toward quorum but not toward YES/NO ratio |
| **VETO** | Counts against passage; if veto votes exceed 33.34 % of all votes cast (including abstain), the proposal is unconditionally defeated and enters a 14-day re-submission blackout |

### Section 4.2 — Voting Power and Anti-Whale Protection

To prevent plutocratic capture, voting power is computed using **quadratic weighting**:

```
votingPower(address) = floor(sqrt(stakedBalance(address, snapshotBlock)))
```

The snapshot block is the block at which the proposal was created (`proposalBlock`). Tokens staked after `proposalBlock` do not count toward this proposal.

Implementation: `VotingSystem::getVotingPower(address, snapshotBlock)`.

### Section 4.3 — Delegation

A staker may delegate their voting power to another address via `VotingSystem::delegate(delegatee)`. Delegation:
- Is revocable at any time, effective at the next proposal snapshot.
- Does not transfer token custody.
- Is limited to one level (no transitive delegation).

### Section 4.4 — Vote Finality

Votes are final once cast and cannot be changed. The voting contract does not expose a `changeVote` method.

### Section 4.5 — Snapshot Integrity

The staking snapshot mechanism is implemented in `Staking::snapshotAt(address, blockNumber)` and relies on an EIP-712 signed checkpoint system. Flash-stake attacks are mitigated by the anti-flash-stake cooldown defined in Article VII, Section 7.4.

---

## Article V: Constitutional Limits — *Isonomia* (ἰσονομία)

The following bounds are hard-coded in `GovernanceConstants.sol` and enforced at the contract level. No proposal — including a CONSTITUTIONAL proposal — may move a parameter outside these absolute limits without a code-level upgrade (which itself requires a CONSTITUTIONAL proposal passing and a mandatory 30-day timelock).

| Parameter | Floor | Ceiling | Notes |
|-----------|-------|---------|-------|
| Boule size | 100 seats | 1 000 seats | |
| Council term length | 3 days | 90 days | |
| Prytany size | 10 members | 100 members | |
| Standard voting window | 3 days | 30 days | |
| Constitutional voting window | 7 days | 60 days | |
| Emergency execution TTL | 12 hours | 7 days | |
| Standard quorum | 5 % | 30 % | Of total staked supply |
| Constitutional quorum | 10 % | 40 % | |
| Supermajority threshold | 60 % | 80 % | For CONSTITUTIONAL proposals |
| Veto threshold | 20 % | 45 % | Fraction of total votes |
| Min proposal stake | 0.001 % | 1 % | Of total staked supply |
| Min council stake | 0.01 % | 5 % | Of total staked supply |
| Max concurrent proposals | 5 | 100 | Active at any one time |
| Execution delay (standard) | 1 day | 14 days | |
| Execution delay (constitutional) | 3 days | 30 days | |
| Large grant threshold | 0.1 % | 10 % | Of treasury balance |
| Slashing — double sign | 1 % | 30 % | Of validator stake |
| Slashing — downtime | 0.001 % | 5 % | Of validator stake |
| Anti-flash-stake cooldown | 1 block | 14 days | Before staked tokens earn voting power |
| Ostracism duration | 30 days | 365 days | |

---

## Article VI: Treasury

### Section 6.1 — Treasury Tracks

The PantheonChain Treasury is divided into five allocation tracks. Each track has an independent balance, governed by TREASURY_SPENDING proposals specifying the track:

| Track | Purpose |
|-------|---------|
| **CORE_DEVELOPMENT** | Protocol engineering, audits, formal verification |
| **GRANTS** | Ecosystem grants for dApps, tooling, and research built on PantheonChain |
| **OPERATIONS** | Infrastructure costs, validator incentives, bug bounties |
| **EMERGENCY** | Reserved for unforeseen protocol emergencies; requires EMERGENCY proposal to access |
| **UNCATEGORIZED** | Catch-all for incoming fees before explicit allocation |

### Section 6.2 — Fee Routing

See Article X for the fee distribution table that determines how transaction fees flow into Treasury tracks.

### Section 6.3 — Budget Periods

The Ekklesia may adopt annual budget resolutions allocating maximum spending per track for a 365-day period. Individual TREASURY_SPENDING proposals must stay within the remaining budget for their track or provide an explicit budget amendment.

### Section 6.4 — Milestone Grants

Any TREASURY_SPENDING proposal disbursing more than `LARGE_GRANT_THRESHOLD` must include:

1. A list of measurable milestones with on-chain verifiable completion criteria.
2. A disbursement schedule tied to milestone completion.
3. A refund mechanism if milestones are not completed within the agreed timeframe.

Implementation: `Treasury::createMilestoneGrant(recipient, milestones, schedule)`.

### Section 6.5 — Reserve Ratio

At least **20 %** of the EMERGENCY track balance must be maintained as an unencumbered reserve at all times. This floor is enforced in `Treasury::withdraw(track, amount)`.

### Section 6.6 — Transparency

All treasury inflows, outflows, and grant disbursements are emitted as on-chain events and queryable via the `treasury_getBalance` and `treasury_listGrants` RPC endpoints.

---

## Article VII: Staking and Voting Power

### Section 7.1 — Minimum Stake

The minimum stake to participate in the Ekklesia is **1 OBL** (1 OBOLOS token, the smallest indivisible unit that conveys voting power after the anti-flash-stake cooldown). There is no minimum stake to receive staking rewards, but voting power requires satisfying the cooldown.

### Section 7.2 — Lock Periods

Stakers may choose voluntary lock periods to increase their effective staking yield:

| Lock Period | Yield Multiplier |
|-------------|-----------------|
| No lock (liquid) | 1× |
| 30 days | 1.25× |
| 90 days | 1.5× |
| 180 days | 1.75× |
| 365 days | 2× |

Lock periods do **not** affect voting power (which uses raw quadratic staked balance) to prevent plutocratic lock-up strategies from disproportionately amplifying governance influence.

### Section 7.3 — Slashing

Validators are subject to slashing for:

| Offense | Default Penalty | Recovery |
|---------|----------------|---------|
| Double signing (equivocation) | 5 % of stake | Slashed tokens are burned; validator enters 2-epoch cooldown |
| Downtime (missed > 10 % of blocks in epoch) | 0.1 % of stake | Warning on first offense; full slashing on third offense within 4 epochs |
| Fraudulent VRF proof | 10 % of stake | Permanent removal from Boule pool |
| Governance manipulation (proven via Apophasis) | Up to 30 % of stake | Subject to Ostracism (Article VIII) |

Slashing penalties are configurable by PARAMETER_CHANGE proposals within the Isonomia bounds defined in Article V.

### Section 7.4 — Anti-Flash-Stake Cooldown

Tokens staked less than `ANTI_FLASH_STAKE_COOLDOWN` blocks before a proposal's snapshot block do not count toward voting power for that proposal. This prevents last-minute stake accumulation to influence a specific vote.

Implementation: `Staking::snapshotAt` filters tokens by their `stakedAtBlock` against `proposalSnapshotBlock - ANTI_FLASH_STAKE_COOLDOWN`.

### Section 7.5 — Vesting Grants

The Treasury may issue vesting grants to contributors:

- **Cliff period**: A minimum duration before any tokens vest.
- **Linear vesting**: After the cliff, tokens vest continuously block-by-block.
- Unvested tokens count toward voting power but are subject to clawback if the grant conditions are violated.

Implementation: `Treasury::createVestingGrant(recipient, amount, cliff, duration)`.

---

## Article VIII: Ostracism — *Ostrakismos* (ὀστρακισμός)

### Section 8.1 — Purpose

Ostracism is a community-driven mechanism to temporarily exclude actors who, while not necessarily criminal, pose a systemic risk to the protocol's health or governance integrity.

### Section 8.2 — Initiation

An Ostracism proposal may be submitted by any address satisfying the standard proposal submission requirements (Article II, Section 2.2). The proposal must identify:

1. The target address.
2. The alleged harm to the protocol.
3. The proposed duration (between `MIN_OSTRACISM_DURATION` and `MAX_OSTRACISM_DURATION` as defined in Article V).

### Section 8.3 — Vote and Threshold

Ostracism proposals use the CONSTITUTIONAL proposal type, requiring:
- ≥ 66 % supermajority of non-abstaining votes.
- ≥ 20 % quorum of total staked supply.
- No VETO threshold waiver (a veto >33.34 % blocks the ostracism).

This high bar prevents the mechanism from being weaponized against minority stakeholders.

### Section 8.4 — Effects of Ostracism

A successfully ostracized address:
- May not submit new governance proposals.
- May not serve on the Boule or Prytany.
- May not receive new Treasury grants.
- **May** continue to vote, stake, transact, and withdraw funds.

Ostracism is not a seizure of assets; it is a restriction on governance participation only.

### Section 8.5 — Duration Limits

Ostracism has a maximum duration of `MAX_OSTRACISM_DURATION` (default: 365 days, constitutional floor: 30 days). A single address may not be subject to more than one concurrent Ostracism order. Renewal requires a new proposal after expiry.

### Section 8.6 — Rehabilitation Path

An ostracized address may apply for early reinstatement after serving at least 50 % of the ostracism term, by submitting a reinstatement proposal. Reinstatement requires a STANDARD majority and ≥ 10 % quorum.

---

## Article IX: Emergency Powers

### Section 9.1 — EmergencyCouncil

The `EmergencyCouncil` is an M-of-N multi-signature body of trusted guardian addresses, separate from the Boule. Its composition is established at genesis and can only be changed by a CONSTITUTIONAL proposal.

- Default configuration: **5-of-9** (any 5 of 9 guardian signers must agree).
- Guardians are identified by on-chain addresses and are publicly disclosed.
- Guardians may not simultaneously serve on the Prytany to prevent concentration of emergency authority.

### Section 9.2 — Scope of Emergency Powers

The EmergencyCouncil may, without prior assembly vote:

1. **Pause** a specific contract or contract method for up to `EMERGENCY_PAUSE_TTL` (default: 48 hours).
2. **Upgrade** a contract implementation if a critical vulnerability has been disclosed and a patch is available, subject to a `EMERGENCY_UPGRADE_TTL` (default: 72 hours) timelock.
3. **Freeze** a specific address's governance participation pending an Apophasis review.

The EmergencyCouncil may **not** confiscate staked assets, modify the supply policy (Article XII), or override a completed assembly vote.

### Section 9.3 — The Investigative Board (*Apophasis*)

The Apophasis (ἀπόφασις) board is a five-member rotating body selected by VRF from non-Prytany Boule members at the start of each epoch. Its role is to:

1. Review all EmergencyCouncil actions within 7 days of execution.
2. Publish a public on-chain findings report.
3. Recommend ratification, revocation, or sanctions against EmergencyCouncil guardians if actions were found to be outside their mandate.

Apophasis recommendations are binding if adopted by a STANDARD assembly vote within 14 days of publication.

### Section 9.4 — TTL Expiry

All EmergencyCouncil actions include a `TTL` field. If the Ekklesia does not ratify the action within the TTL window, the action is automatically reverted (where technically feasible) by a governance keeper bot. The keeper is permissionless — any address may trigger the revert function once TTL has elapsed.

---

## Article X: Fee Distribution

Transaction fees collected across all three layers are routed according to the following table. Percentages are configurable by PARAMETER_CHANGE proposals within the bounds in Article V.

### Layer 1 (TALANTON) Fee Routing

| Destination | Default % | Description |
|-------------|-----------|-------------|
| L1 Block Producer | 60 % | Coinbase reward to mining node |
| L1 Treasury (OPERATIONS) | 20 % | Protocol infrastructure fund |
| L1 Burn | 20 % | Deflationary pressure per EIP-1559 analogue |

### Layer 2 (DRACHMA) Fee Routing

| Destination | Default % | Description |
|-------------|-----------|-------------|
| L2 Validator Pool | 50 % | Proportional to stake weight |
| L2 Treasury (CORE_DEVELOPMENT) | 20 % | Protocol development fund |
| L1 Anchor Subsidy | 20 % | Subsidizes L1 anchoring costs |
| L2 Burn | 10 % | Deflationary pressure |

### Layer 3 (OBOLOS) Fee Routing

| Destination | Default % | Description |
|-------------|-----------|-------------|
| L3 Validator Pool | 40 % | Proportional to stake weight |
| L3 Treasury (GRANTS) | 20 % | Ecosystem grants fund |
| L3 Treasury (CORE_DEVELOPMENT) | 15 % | Protocol development fund |
| L2 Anchor Subsidy | 15 % | Subsidizes L2 anchoring costs |
| L3 Burn | 10 % | Deflationary pressure |

Fee routing is implemented in `layer3-obolos/fees/FeeRouter.sol` and `layer2-drachma/fees/FeeRouter.sol`.

---

## Article XI: Supply Policy

### Section 11.1 — Maximum Supplies

| Token | Maximum Supply | Layer |
|-------|---------------|-------|
| TALANTON (TALN) | 21 000 000 | L1 |
| DRACHMA (DRM) | 41 000 000 | L2 |
| OBOLOS (OBL) | 61 000 000 | L3 |

These maximum supplies are hard-coded at the consensus layer and cannot be changed by any governance action. Changing them requires a hard fork with community consensus.

### Section 11.2 — Supply Governance Thresholds

Certain governance actions are unlocked or restricted based on the percentage of maximum supply that has been minted:

| Threshold | Unlocked Action |
|-----------|----------------|
| < 5 % minted | Standard issuance schedule; no governance restrictions |
| ≥ 5 % minted | PARAMETER_CHANGE required to adjust block reward beyond ±10 % |
| ≥ 10 % minted | CONSTITUTIONAL proposal required to adjust halving schedule |
| ≥ 50 % minted | Any increase in issuance rate requires CONSTITUTIONAL proposal with 72-hour community review |

### Section 11.3 — Burn Mechanics

Burned tokens are sent to the zero address (`0x000...000`) and are deducted from circulating supply. They do not reduce the maximum supply ceiling (which is a consensus-layer limit on new issuance, not a balance cap).

### Section 11.4 — Treasury Issuance

Treasury grants disbursed from the GRANTS or CORE_DEVELOPMENT tracks are paid from circulating treasury reserves, not from new issuance. New issuance is exclusively via block rewards according to the halving schedule.

---

## Article XII: Amendments

### Section 12.1 — Amendment Process

This Constitution may be amended by the following process:

1. A CONSTITUTIONAL proposal is submitted via `VotingSystem::submitProposal(CONSTITUTIONAL, ...)`.
2. The proposal text must include a diff of the specific sections being amended.
3. The standard CONSTITUTIONAL vote (14-day window, ≥ 66 % supermajority, ≥ 20 % quorum) must pass.
4. A mandatory 7-day execution delay follows passage before on-chain parameters are updated.
5. The updated `CONSTITUTION.md` document must be committed to the canonical repository within 48 hours of execution.

### Section 12.2 — Unamendable Provisions

The following provisions may **not** be changed by any governance vote. They require a hard fork:

- Maximum token supplies (Article XI, Section 11.1).
- The existence of the Boule, Ekklesia, and EmergencyCouncil institutions.
- The Isonomia absolute limits row in Article V (the outer bounds of the bounds table).
- The prohibition on confiscating staked assets.

### Section 12.3 — Canonical Reference

The canonical version of this Constitution is the version committed to the `main` branch of the PantheonChain repository at the time of each on-chain execution. In the event of discrepancy between this document and the deployed smart contract parameters, the **smart contract parameters prevail**.

### Section 12.4 — Effective Date

This Constitution takes effect at the block height recorded in `GovernanceConstants::CONSTITUTION_EFFECTIVE_BLOCK` on the OBOLOS chain.

---

## Appendix A: Key Contract Addresses (Mainnet — TBD)

> Contract addresses will be published here following mainnet deployment.

| Contract | Address |
|----------|---------|
| `Boule` | TBD |
| `VotingSystem` | TBD |
| `Treasury` | TBD |
| `Staking` | TBD |
| `Ostracism` | TBD |
| `EmergencyCouncil` | TBD |
| `FeeRouter (L3)` | TBD |
| `GovernanceConstants` | TBD |

## Appendix B: Glossary of Greek Terms

| Term | Transliteration | Meaning in Context |
|------|-----------------|-------------------|
| Ἀπόφασις | Apophasis | Investigative board that reviews emergency actions |
| Βουλή | Boule | The validator council selected by VRF sortition |
| Δοκιμασία | Dokimasia | Eligibility screening for council candidates |
| Ἐκκλησία | Ekklesia | The full staker assembly; sovereign governance body |
| Ἐπιστάτης | Epistates | Presiding officer of the Prytany, chosen daily |
| Εὐνομία | Eunomia | Good order; the governance pipeline structure |
| Ἰσηγορία | Isegoria | Equal right of proposal submission |
| Ἰσονομία | Isonomia | Constitutional parameter bounds enforceable by code |
| Κληρωτήρια | Kleroteria | The VRF-based sortition mechanism |
| Ὀστρακισμός | Ostrakismos | Community-voted temporary governance exclusion |
| Πρυτανεία | Prytany | The executive committee of 50 Boule members |
| Σωφροσύνη | Sophrosyne | Prudence; the veto and supermajority protections |

---

*This Constitution was ratified at genesis. All subsequent amendments are recorded on-chain and reflected in this document.*

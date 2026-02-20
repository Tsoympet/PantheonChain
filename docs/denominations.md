# Attic-style Display Denominations

PantheonChain supports an **Attic standard display set** for UI/UX formatting and helper parsing.

> These are display conventions only.
> They are **not** consensus changes, **not** historical guarantees across every city-state,
> **not** market pegs, and do **not** affect ledger state, gas accounting, fee rules, or tokenomics.

## Fixed protocol ratios

- `1 DRACHMA = 6 OBOLOS`
- `1 TALANTON = 6000 DRACHMA = 36000 OBOLOS`

All internal values remain integer base units.

## TALANTON display set

| Denomination | Symbol | Ratio to TAL | Input allowed |
|---|---|---:|---|
| talanton | TAL | 1 | yes |
| mina | MNA | 1/60 | no (display/reporting) |

## DRACHMA display set

| Denomination | Symbol | Ratio to DR | Input allowed |
|---|---|---:|---|
| drachma | DR | 1 | yes |
| obol (view alias) | OB | 1/6 | yes |
| tetradrachm | 4DR | 4 | yes |
| mina | MNA | 100 | yes |

## OBOLOS display set

| Denomination | Symbol | Ratio to OB | Input allowed |
|---|---|---:|---|
| obol | OB | 1 | yes |
| hemiobol | 1/2OB | 1/2 | no (display-only) |

`hemiobol` is display-only in this implementation. If a denomination cannot be represented exactly for parsing,
input is rejected with: `denomination not exactly representable with current decimals`.

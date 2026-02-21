# Enterprise Features

This directory contains enterprise-grade extensions for PantheonChain, providing permissioned
blockchain, consortium governance, compliance, audit, and SLA monitoring capabilities.

## Files

| File | Description |
|------|-------------|
| `permissioned.h` | Header for all enterprise classes |
| `permissioned.cpp` | Implementation of all enterprise classes |

## Classes

### PermissionedMode

Configures a private, access-controlled blockchain network.

- **Permission levels**: `ADMIN`, `VALIDATOR`, `PARTICIPANT`, `OBSERVER`
- Add/remove participants by address
- Enable or disable permissioned mode at runtime

### ConsortiumManager

Multi-organization governance for consortium chains.

- Register member organizations with voting weights
- Propose and vote on consortium decisions
- Simple majority approval logic

### ComplianceManager

KYC/AML compliance tooling.

- Register and query KYC records per address
- Screen transactions for AML alerts (large amounts, unverified parties)
- Report and retrieve suspicious-activity alerts

### AuditLogger

Comprehensive, tamper-evident audit trail.

- Log typed audit events (transactions, permission changes, security events, etc.)
- Query by event type and time range
- Export audit log to file

### SLAMonitor

Service Level Agreement tracking for node operators.

- Record block times, confirmation times, and transaction outcomes
- Configure SLA thresholds (uptime, block time, confirmation time, success rate)
- Report SLA violations

## Usage

Include the header and link the `permissioned.cpp` translation unit:

```cpp
#include "enterprise/permissioned.h"

parthenon::enterprise::PermissionedMode pm;
pm.SetEnabled(true);
pm.AddParticipant(address, PermissionedMode::PermissionLevel::VALIDATOR, "Org A");
```

Build with the rest of the project via CMake (add `enterprise/permissioned.cpp` to your target
sources).

## Status

Enterprise features are implemented and functional. Integration with the core node daemon
(`parthenond`) is planned for a future release.

#pragma once

#include <string>

namespace pantheon::common {

// Compute SHA-256d (double SHA-256) of the payload and return the result as a
// 64-character lowercase hex string.
// Note: despite the "Pseudo" prefix in the name (a historical artefact), this
// function uses the cryptographic SHA256d implementation.
std::string PseudoSha256d(const std::string& payload);

}  // namespace pantheon::common

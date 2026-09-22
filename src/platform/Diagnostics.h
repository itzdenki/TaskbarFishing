#pragma once

namespace diagnostics {
// The log is per launch, flushed after every line, and never contains save data.
void initialize(bool unattended) noexcept;
void reportError(const char* message) noexcept;
}

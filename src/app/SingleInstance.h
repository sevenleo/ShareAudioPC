#pragma once

#include "app/Result.h"

namespace shareaudio {

/**
 * Enforces that only a single instance of the application is running.
 * If a previous instance is detected, it terminates it before continuing.
 */
Result<void> enforce_single_instance();

} // namespace shareaudio

#pragma once

#include "app/Result.h"

#include <string>
#include <vector>

namespace shareaudio {

Result<std::vector<std::string>> list_local_ip_addresses();
bool is_loopback_address(const std::string& host);
Result<bool> is_local_address(const std::string& host);

} // namespace shareaudio

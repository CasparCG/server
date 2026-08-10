#pragma once

namespace caspar {

// Opt out of EcoQoS throttling, and of Windows 11 revoking a process's requested timer resolution
// once it is occluded or invisible. Returns false on Windows 10, where only the former applies.
bool disable_process_power_throttling();

} // namespace caspar

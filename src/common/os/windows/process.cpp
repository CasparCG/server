#include "process.h"

#include "windows.h"
#include <processthreadsapi.h>

// Added in the Windows 11 SDK (10.0.22000). Define it manually so we still build
// against older SDKs; the call below degrades gracefully at runtime.
#ifndef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
#define PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION 0x4
#endif

namespace caspar {

static bool set_power_throttling(ULONG control_mask)
{
    PROCESS_POWER_THROTTLING_STATE throttling = {};
    throttling.Version                        = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    throttling.ControlMask                    = control_mask;
    throttling.StateMask                      = 0; // 0 = opt out of the throttling named by ControlMask
    return SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &throttling, sizeof(throttling)) != FALSE;
}

bool disable_process_power_throttling()
{
    // IGNORE_TIMER_RESOLUTION is rejected with ERROR_INVALID_PARAMETER on Windows 10, and the
    // call is all-or-nothing, so fall back to the EXECUTION_SPEED opt-out alone there.
    if (set_power_throttling(PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
                             PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION)) {
        return true;
    }

    set_power_throttling(PROCESS_POWER_THROTTLING_EXECUTION_SPEED);
    return false;
}

} // namespace caspar

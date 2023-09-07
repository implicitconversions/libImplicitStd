#pragma once

// Expose the entire suite of icyappSettingsBase publicly.
// This header is kept under 'inc/opt/' so that linking libraries can opt into replacing it with their
// own header at the top-levle. Alternatively, the project can add 'inc/opt/' to include dirs and access
// this file directly.

#include "icyAppSettingsBase.h"


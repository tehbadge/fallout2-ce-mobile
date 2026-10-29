#ifndef FALLOUT_IOS_DISPLAY_METRICS_H
#define FALLOUT_IOS_DISPLAY_METRICS_H

#include "display_geometry.h"

namespace fallout {

// SDL's UIKit application runs the engine on the main thread. Other callers
// get unavailable metrics; never synchronously dispatch to the main thread.
IosDisplayGeometry iosDisplayMetricsGet();
void iosDisplayLogicalSafeInsets(const IosDisplayGeometry& geometry, int insets[4]);

} // namespace fallout
#endif

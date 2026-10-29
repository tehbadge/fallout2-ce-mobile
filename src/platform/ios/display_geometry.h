#ifndef FALLOUT_IOS_DISPLAY_GEOMETRY_H
#define FALLOUT_IOS_DISPLAY_GEOMETRY_H

#include <algorithm>
#include <cmath>

namespace fallout {

// All edges use left, top, right, bottom order. Kept UIKit/SDL independent
// so the coordinate conversion can be tested on the host.
struct IosDisplayGeometry {
    bool valid = false;
    float pointsWidth = 0;
    float pointsHeight = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    float pointsInsets[4] = {};
    float pixelsPerPointX = 0;
    float pixelsPerPointY = 0;
    float outputInsets[4] = {};
    int orientation = 0; // UIInterfaceOrientation, from the attached scene.
};

inline bool iosResolveDisplayGeometry(IosDisplayGeometry& geometry)
{
    geometry.valid = false;
    if (!std::isfinite(geometry.pointsWidth) || !std::isfinite(geometry.pointsHeight)
        || !(geometry.pointsWidth > 0) || !(geometry.pointsHeight > 0)
        || geometry.outputWidth <= 0 || geometry.outputHeight <= 0) {
        return false;
    }
    geometry.pixelsPerPointX = geometry.outputWidth / geometry.pointsWidth;
    geometry.pixelsPerPointY = geometry.outputHeight / geometry.pointsHeight;
    if (!std::isfinite(geometry.pixelsPerPointX) || !std::isfinite(geometry.pixelsPerPointY)) {
        return false;
    }
    for (int edge = 0; edge < 4; edge++) {
        if (!std::isfinite(geometry.pointsInsets[edge]) || geometry.pointsInsets[edge] < 0) {
            return false;
        }
        geometry.outputInsets[edge] = std::ceil(geometry.pointsInsets[edge]
            * (edge % 2 == 0 ? geometry.pixelsPerPointX : geometry.pixelsPerPointY));
    }
    geometry.valid = true;
    return true;
}

// origin and scale describe the actual SDL logical -> drawable transform.
inline void iosLogicalSafeInsets(const IosDisplayGeometry& geometry,
    int logicalWidth, int logicalHeight, float originX, float originY,
    float scaleX, float scaleY, int insets[4])
{
    std::fill(insets, insets + 4, 0);
    if (!geometry.valid || !(scaleX > 0) || !(scaleY > 0)) {
        return;
    }
    float bars[4] = { originX, originY,
        geometry.outputWidth - originX - logicalWidth * scaleX,
        geometry.outputHeight - originY - logicalHeight * scaleY };
    for (int edge = 0; edge < 4; edge++) {
        insets[edge] = static_cast<int>(std::ceil(std::max(geometry.outputInsets[edge] - bars[edge], 0.0f)
            / (edge % 2 == 0 ? scaleX : scaleY)));
    }
}

} // namespace fallout
#endif

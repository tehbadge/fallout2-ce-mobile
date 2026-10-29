#ifndef FALLOUT_IOS_CRT_SCALING_H
#define FALLOUT_IOS_CRT_SCALING_H

#include <algorithm>
#include <cmath>

namespace fallout {
struct IosCrtScale {
    float grillePitch;
    float scanPeriod;
    float beamOffset;
};

inline IosCrtScale iosCrtResolveScale(float logicalToOutputX, float logicalToOutputY,
    float pixelsPerPointX, float pixelsPerPointY)
{
    // Source rows alone can be sub-point detail in a compact view. Keep the
    // source raster when coarser; otherwise use a presentation raster with a
    // one-point bright band and one-point gap. Points are attached-view units,
    // not inferred PPI, millimeters or a guarantee of perceptual equivalence.
    // Three output pixels keep the fundamental beyond the Nyquist taper;
    // finer harmonics still use the shader's existing integration/suppression.
    auto rasterScale = [](float source, float pixelsPerPoint) {
        source = std::isfinite(source) && source >= 1.0f / 16384 && source <= 16384 ? source : 1;
        float presentation = std::isfinite(pixelsPerPoint) && pixelsPerPoint > 0
                && pixelsPerPoint <= 16384
            ? 2.0f * pixelsPerPoint : 0;
        // Missing view geometry uses only source/output information.
        return std::max(source, std::max(3.0f, presentation));
    };
    float sx = rasterScale(logicalToOutputX, pixelsPerPointX);
    float sy = rasterScale(logicalToOutputY, pixelsPerPointY);
    // One effective raster controls patterns and reconstruction together; no
    // amplitude compensation or device/preset-dependent adaptation factors.
    return { 2.0f * sx, sy, 0.36f * sx };
}
} // namespace fallout
#endif

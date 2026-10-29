#include "platform/ios/display_geometry.h"

#include <cassert>
#include <limits>

using namespace fallout;

int main()
{
    IosDisplayGeometry geometry;
    int insets[4] = { 1, 1, 1, 1 };
    assert(!iosResolveDisplayGeometry(geometry));
    iosLogicalSafeInsets(geometry, 1000, 480, 0, 0, 2, 2, insets);
    for (int value : insets) assert(value == 0);

    // Late attachment, symmetric landscape, drawable scale 3 on both axes.
    geometry.pointsWidth = 900;
    geometry.pointsHeight = 400;
    geometry.outputWidth = 2700;
    geometry.outputHeight = 1200;
    // Attached zero insets are legal, but a subsequent snapshot must replace
    // them when UIKit finishes laying out the safe area.
    assert(iosResolveDisplayGeometry(geometry));
    for (float value : geometry.outputInsets) assert(value == 0);
    geometry.pointsInsets[0] = geometry.pointsInsets[2] = 60;
    geometry.pointsInsets[3] = 20;
    assert(iosResolveDisplayGeometry(geometry));
    assert(geometry.pixelsPerPointX == 3 && geometry.pixelsPerPointY == 3);
    iosLogicalSafeInsets(geometry, 1000, 480, 100, 0, 2.5f, 2.5f, insets);
    assert(insets[0] == 32 && insets[2] == 32 && insets[1] == 0 && insets[3] == 24);

    // Rotation swaps asymmetric edges, including a nonzero vertical bar.
    geometry.orientation = 3;
    geometry.pointsInsets[2] = 0;
    assert(iosResolveDisplayGeometry(geometry));
    iosLogicalSafeInsets(geometry, 1000, 440, 100, 50, 2.5f, 2.5f, insets);
    assert(insets[0] == 32 && insets[2] == 0 && insets[3] == 4);
    geometry.orientation = 4;
    geometry.pointsInsets[0] = 0;
    geometry.pointsInsets[2] = 60;
    assert(iosResolveDisplayGeometry(geometry));
    iosLogicalSafeInsets(geometry, 1000, 440, 100, 50, 2.5f, 2.5f, insets);
    assert(insets[0] == 0 && insets[2] == 32 && insets[3] == 4);

    // Resume with a changed drawable: independent axes, fractional insets,
    // unequal logical scales and asymmetric viewport offsets.
    geometry.outputWidth = 1800;
    geometry.outputHeight = 1200;
    geometry.pointsInsets[0] = 10.1f;
    geometry.pointsInsets[1] = 7.1f;
    geometry.pointsInsets[2] = 30.1f;
    geometry.pointsInsets[3] = 20.1f;
    assert(iosResolveDisplayGeometry(geometry));
    assert(geometry.pixelsPerPointX == 2 && geometry.pixelsPerPointY == 3);
    iosLogicalSafeInsets(geometry, 880, 450, 10, 25, 2, 2.5f, insets);
    assert(insets[0] == 6 && insets[1] == 0 && insets[2] == 16 && insets[3] == 5);

    // A transient invalid snapshot cannot keep the prior valid insets alive.
    geometry.pointsHeight = 0;
    assert(!iosResolveDisplayGeometry(geometry));
    iosLogicalSafeInsets(geometry, 880, 450, 10, 25, 2, 2.5f, insets);
    for (int value : insets) assert(value == 0);
    geometry.pointsHeight = 400;
    assert(iosResolveDisplayGeometry(geometry));
    geometry.pointsWidth = std::numeric_limits<float>::infinity();
    assert(!iosResolveDisplayGeometry(geometry));
    return 0;
}

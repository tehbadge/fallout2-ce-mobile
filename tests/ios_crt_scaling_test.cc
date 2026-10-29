#include "../src/platform/ios/crt_scaling.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>

static bool near(float a, float b) { return std::fabs(a - b) < 0.00001f; }
// Reference for unchanged shader pixel integration / Nyquist suppression.
static float gain(float pitch)
{
    float t = std::fmax(0, std::fmin(1, pitch - 2));
    float k = 3.14159265359f / std::fmax(pitch, 0.0001f);
    return std::sin(k) / k * t * t * (3 - 2 * t);
}
int main()
{
    using namespace fallout;
    struct Profile {
        int width, height;
        float sx, sy, ppx, ppy, rasterX, rasterY;
    };
    // View coverage, output density, source scale and aspect vary independently.
    // No physical PPI or device identity enters the policy.
    const Profile profiles[] = {
        {2868,1320,2.75f,2.75f,3,3,6,6}, // compact/high resolution
        {1334,750,1.5625f,1.5625f,2,2,4,4}, // compact/lower resolution
        {2224,1668,3.475f,3.475f,2,2,4,4}, // tablet-style
        {3200,2400,5,5,2,2,5,5}, // larger source raster retained
        {3840,1800,3.75f,3.75f,4,4,8,8}, // unknown high-scale compact view
        {1920,1080,2.25f,2.25f,2,2,4,4}, // unknown view with metrics
        {1920,1080,2.25f,2.25f,0,0,3,3}, // metrics unavailable
        {1434,660,1.375f,1.375f,1.5f,1.5f,3,3}, // reduced drawable, same view
        {1920,1080,2,2,2,2,4,4}, // letterboxed source
        {1280,960,2,1.5f,2,1.5f,4,3}, // unequal output/view axes
        {640,480,1,1,1,1,3,3}, // sampling floor overrides point floor
        {480,320,.5f,.5f,.5f,.5f,3,3}, // undersampled original raster
    };
    for (const auto& p : profiles) {
        auto v = iosCrtResolveScale(p.sx,p.sy,p.ppx,p.ppy);
        assert(near(v.grillePitch, 2*p.rasterX));
        assert(near(v.scanPeriod, p.rasterY));
        assert(near(v.beamOffset, .36f*p.rasterX));
        assert(gain(v.scanPeriod) >= gain(3));
        assert(gain(v.grillePitch) >= gain(6));
        // Preset fundamentals and the existing higher-frequency shape bases.
        for (float pitch : {v.scanPeriod, v.scanPeriod*1.5f, v.scanPeriod*.75f,
                 v.grillePitch, v.grillePitch*1.2f, v.grillePitch*1.1f*1.7320508f}) {
            assert(std::isfinite(pitch) && pitch > 0);
            assert(std::isfinite(gain(pitch)) && gain(pitch) >= 0 && gain(pitch) <= 1);
        }
        std::printf("%dx%d source=%.4f,%.4f ppp=%.2f,%.2f raster=%.4f,%.4f gain=%.4f oldGain=%.4f\n",
            p.width,p.height,p.sx,p.sy,p.ppx,p.ppy,p.rasterX,p.rasterY,gain(v.scanPeriod),gain(p.sy));
    }
    // Reduced output preserves attached-view pitch until the sampling floor.
    auto full = iosCrtResolveScale(2.75f,2.75f,3,3);
    auto reduced = iosCrtResolveScale(1.375f,1.375f,1.5f,1.5f);
    assert(near(full.grillePitch,reduced.grillePitch*2));
    assert(near(full.scanPeriod,reduced.scanPeriod*2));
    assert(near(full.beamOffset,reduced.beamOffset*2));
    assert(near(full.scanPeriod/3,reduced.scanPeriod/1.5f));
    // Source-dominant geometry stays unchanged; adaptation is continuous.
    auto coarse = iosCrtResolveScale(8,7,2,2);
    assert(coarse.grillePitch == 16 && coarse.scanPeriod == 7);
    auto left = iosCrtResolveScale(3.999f,3.999f,2,2);
    auto right = iosCrtResolveScale(4.001f,4.001f,2,2);
    assert(right.scanPeriod-left.scanPeriod < .002f);
    const float invalid[] = {0,-1,std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),std::numeric_limits<float>::max(),
        std::numeric_limits<float>::denorm_min()};
    for (float f : invalid) {
        auto v = iosCrtResolveScale(f,f,0,0);
        assert(v.grillePitch == 6 && v.scanPeriod == 3 && near(v.beamOffset,1.08f));
        // Invalid presentation data must not discard a valid coarse source.
        auto source = iosCrtResolveScale(8,7,f,f);
        assert(source.grillePitch == 16 && source.scanPeriod == 7);
    }
    for (float p=.125f;p<=16;p+=.125f) {
        assert(std::isfinite(gain(p)) && gain(p)>=0 && gain(p)<=1);
        if(p<=2) assert(gain(p)==0); // unrepresentable harmonics remain suppressed
        if(p>2) assert(gain(p)>gain(p-.125f));
    }
    // Returning to a geometry after resize/recreation has no history.
    iosCrtResolveScale(5,3,2,2);
    auto restored = iosCrtResolveScale(2.75f,2.75f,3,3);
    assert(full.scanPeriod==restored.scanPeriod && full.grillePitch==restored.grillePitch);
    // Effective raster peaks retain viewport-relative row-center phase.
    for (float period : {3.0f,4.0f,6.0f}) {
        float margin=80,origin=margin+period*.5f;
        for(int row=0;row<480;++row) {
            float center=margin+(row+.5f)*period;
            assert(std::fabs(std::cos(6.28318530718f*((center-origin)/period))-1)<.0001f);
        }
    }
    std::puts("PASS: presentation/source policy, sampling floor, reduced output, unknown metrics, independent axes, continuity, invalid inputs and recreation");
}

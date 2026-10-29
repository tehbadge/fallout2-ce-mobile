#include "crt_renderer.h"
#include "crt_scaling.h"
#include "display_metrics.h"
#include "../../svga.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <string.h>

// Defined only in the isolated iOS SDL Metal renderer. SDL owns presentation.
extern "C" int F2CE_SDLMetalCopyWindowToTexture(SDL_Renderer*, void*);

namespace fallout {
namespace {
enum class Placement { PreUi, PostUi };
// Scalar-only layout shared with Metal; no SIMD alignment assumptions.
struct CrtParameters {
    uint32_t operation;
    float scanline;
    float grille;
    float blend;
    float period;
    float grillePitch;
    float beamOffset;
    float originY;
    uint32_t model;
    float chromaBlend;
    float chromaOffset;
};
static_assert(std::is_standard_layout<CrtParameters>::value && sizeof(CrtParameters) == 44);
static_assert(alignof(CrtParameters) == 4 && offsetof(CrtParameters, operation) == 0);
static_assert(offsetof(CrtParameters, scanline) == 4 && offsetof(CrtParameters, grille) == 8);
static_assert(offsetof(CrtParameters, blend) == 12 && offsetof(CrtParameters, period) == 16);
static_assert(offsetof(CrtParameters, grillePitch) == 20 && offsetof(CrtParameters, beamOffset) == 24);
static_assert(offsetof(CrtParameters, originY) == 28 && offsetof(CrtParameters, model) == 32);
static_assert(offsetof(CrtParameters, chromaBlend) == 36 && offsetof(CrtParameters, chromaOffset) == 40);

struct ModelDefinition {
    const char* name;
    CrtParameters parameters;
    float maskRatio;
    float rowRatio;
    float beamRatio;
};
// Ratios use the accepted effective raster. Tonytron leads with a fine vertical
// grille and clean beam; Arcade leads with horizontal raster and a secondary mask.
constexpr ModelDefinition models[] = {
    { "Off", {}, 0, 0, 0 },
    { "Tonytron", { 2, 0.08f, 0.22f, 0.08f, 3, 0, 0, 0, 0, 0, 0 }, 0.5f, 1.0f, 0.6f },
    { "Arcade", { 2, 0.42f, 0.04f, 0.28f, 3, 0, 0, 0, 1, 0, 0 }, 1.2f, 1.5f, 1.15f },
    { "Consumer TV", { 2, 0.18f, 0.09f, 0.42f, 3, 0, 0, 0, 2, 0.42f, 0 }, 1.1f, 1.0f, 1.25f },
};
static_assert(sizeof(models) / sizeof(models[0]) == static_cast<unsigned>(IosCrtModel::ConsumerTv) + 1);
IosCrtModel activeModel = IosCrtModel::Off;
Placement placement = Placement::PostUi;
bool rendererSupported = false;
#ifndef NDEBUG
bool diagnosticIdentity = false;
// Debugger-only, one-shot faults exercise fallback without GPU-invalid commands.
enum class DiagnosticFailure { None, Resources, Snapshot, Encoder };
DiagnosticFailure diagnosticFailure = DiagnosticFailure::None;
bool consumeDiagnosticFailure(DiagnosticFailure failure)
{
    if (diagnosticFailure != failure) return false;
    diagnosticFailure = DiagnosticFailure::None;
    return true;
}
#endif
CrtParameters parameters {};
id<MTLRenderPipelineState> pipeline = nil;
id<MTLTexture> snapshot = nil;
bool reportedDraw = false;
bool reportedFailure = false;

void releaseResources()
{
    [snapshot release];
    snapshot = nil;
    [pipeline release];
    pipeline = nil;
    reportedDraw = false;
    reportedFailure = false;
}

// This does not change SDL/UIView geometry or input coordinates.
void updateScaling(SDL_Renderer* renderer, int width, int height)
{
    int logicalWidth = 0, logicalHeight = 0;
    SDL_RenderGetLogicalSize(renderer, &logicalWidth, &logicalHeight);
    float sx = 1, sy = 1, lx = 0, ly = 0;
    SDL_RenderGetScale(renderer, &sx, &sy);
    SDL_RenderWindowToLogical(renderer, 0, 0, &lx, &ly);
    IosDisplayGeometry geometry = iosDisplayMetricsGet();
    IosCrtScale scale = iosCrtResolveScale(sx, sy,
        geometry.valid ? geometry.pixelsPerPointX : 0,
        geometry.valid ? geometry.pixelsPerPointY : 0);
    const ModelDefinition& definition = models[static_cast<unsigned>(activeModel)];
    parameters = definition.parameters;
    parameters.period = scale.scanPeriod * definition.rowRatio;
    parameters.grillePitch = scale.grillePitch * definition.maskRatio;
    parameters.beamOffset = scale.beamOffset * definition.beamRatio;
    parameters.chromaOffset = scale.beamOffset * (0.75f / 0.36f);
    // Put beam peaks at effective raster-row centers; retain the viewport phase when boxed.
    parameters.originY = -ly * sy + parameters.period * 0.5f;
#ifndef NDEBUG
    // Log once per geometry/model/placement change, not per frame or pan.
    static char last[768] = {};
    char message[768];
    SDL_snprintf(message, sizeof(message), "CRT scale: policy=presentation-raster output=%dx%d logical=%dx%d viewportOrigin=%.3f,%.3f logicalScale=%.5f,%.5f ppp=%.4f,%.4f grillePx=%.6f scanPx=%.6f beamPx=%.6f preset=%s strengths=%.3f,%.3f,%.3f placement=%s",
        width, height, logicalWidth, logicalHeight, -lx * sx, -ly * sy, sx, sy,
        geometry.valid ? geometry.pixelsPerPointX : 0, geometry.valid ? geometry.pixelsPerPointY : 0,
        parameters.grillePitch, parameters.period, parameters.beamOffset,
        definition.name, parameters.scanline, parameters.grille, parameters.blend,
        placement == Placement::PostUi ? "post" : "pre");
    if (strcmp(last, message) != 0) { SDL_Log("%s", message); SDL_strlcpy(last, message, sizeof(last)); }
#endif
}
bool makePipeline(CAMetalLayer* layer)
{
    // SDL's BGRA8Unorm drawable stores encoded display RGB, not linear light.
    // Decode only for CRT reconstruction; Debug identity preserves encoded RGB.
    NSString* source = @R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Params { uint operation; float scanline; float grille; float blend; float period; float grillePitch; float beamOffset; float originY; uint model; float chromaBlend; float chromaOffset; };
float3 decode(float3 c) {
    return select(c / 12.92, pow((c + 0.055) / 1.055, float3(2.4)), c > 0.04045);
}
float3 encode(float3 c) {
    return select(c * 12.92, 1.055 * pow(max(c, 0.0), float3(1.0/2.4)) - 0.055, c > 0.0031308);
}
vertex float4 crtVertex(uint id [[vertex_id]]) {
    float2 p = id == 0 ? float2(-1,-1) : (id == 1 ? float2(3,-1) : float2(-1,3));
    return float4(p,0,1);
}
// Fractional offsets interpolate decoded light, not encoded display RGB.
float3 sampleLight(texture2d<float, access::read> image, float x, uint y) {
    x = clamp(x, 0.0, float(image.get_width() - 1));
    uint a = uint(floor(x));
    uint b = min(a + 1, image.get_width() - 1);
    return mix(decode(image.read(uint2(a,y)).rgb), decode(image.read(uint2(b,y)).rgb), fract(x));
}
// Pixel-area integration plus a Nyquist taper. Sub-two-pixel repeats cannot
// be represented faithfully; suppress them rather than fold into false bands.
float patternGain(float pitch) {
    float k = 3.14159265359 / max(pitch, 0.0001);
    return sin(k) / k * smoothstep(2.0, 3.0, pitch);
}
fragment float4 crtFragment(float4 p [[position]],
    texture2d<float, access::read> image [[texture(0)]], constant Params& u [[buffer(0)]]) {
    uint2 xy = uint2(p.xy);
    float4 c = image.read(xy);
    if (u.operation != 2) return c;
    float3 center = decode(c.rgb);
    float3 sides = (sampleLight(image, float(xy.x) - u.beamOffset, xy.y)
        + sampleLight(image, float(xy.x) + u.beamOffset, xy.y)) * 0.5;
    float3 light = mix(center, sides, u.blend);
    const float3 luma = float3(0.2126, 0.7152, 0.0722);
    if (u.model == 2 && u.chromaBlend > 0) {
        // A modest bandwidth distinction, not a composite-video emulator.
        float3 wide = center * 0.5 + 0.25 *
            (sampleLight(image, float(xy.x) - u.chromaOffset, xy.y)
             + sampleLight(image, float(xy.x) + u.chromaOffset, xy.y));
        float y = dot(light, luma);
        float3 chroma = mix(light - y, wide - dot(wide, luma), u.chromaBlend);
        // Preserve luminance while compressing out-of-gamut chroma toward grey.
        float3 limits = select((1.0-y) / max(chroma, 0.00001),
            y / max(-chroma, 0.00001), chroma < 0.0);
        float k = min(1.0, min(limits.r, min(limits.g, limits.b)));
        light = y + chroma * k;
    }
    // Fixed synthetic screen raster: independent of camera/zoom/UI scale.
    const float tau = 6.28318530718;
    float phase = tau * (p.y - u.originY) / u.period;
    float scan = cos(phase) * patternGain(u.period);
    if (u.model == 1) {
        // A narrower rounded beam: separately integrate/taper each harmonic.
        scan += 0.22 * cos(2.0*phase) * patternGain(u.period * 0.5);
    }
    float3 xphase = tau * (p.x / u.grillePitch + float3(0, -1.0/3.0, 1.0/3.0));
    float3 mask = cos(xphase) * patternGain(u.grillePitch);
    if (u.model == 1) {
        // Slot-mask interruptions. Product spectra are filtered in both axes.
        float rowPitch = u.grillePitch * 1.5;
        float slot = cos(tau * p.y / rowPitch) * patternGain(rowPitch);
        mask *= 0.78 + 0.22 * slot;
    } else if (u.model == 2) {
        // Staggered shadow-mask lattice, as a bandlimited diagonal basis.
        float rowPitch = u.grillePitch * 0.8660254;
        float row = cos(tau * p.y / (rowPitch * 2.0));
        mask *= row * patternGain(rowPitch * 2.0);
    }
    float3 modulation = u.scanline * scan + u.grille * mask;
    // Bounded zero-mean modulation retains highlight headroom and black level.
    light += modulation * light * (1.0 - light);
    return float4(encode(light), c.a);
}
)METAL";
    NSError* error = nil;
    id<MTLLibrary> library = [layer.device newLibraryWithSource:source options:nil error:&error];
    if (!library) {
        SDL_Log("Metal CRT disabled: library: %s", error.localizedDescription.UTF8String);
        return false;
    }
    id<MTLFunction> vertex = [library newFunctionWithName:@"crtVertex"];
    id<MTLFunction> fragment = [library newFunctionWithName:@"crtFragment"];
    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.label = @"Fallout CRT reconstruction";
    descriptor.vertexFunction = vertex;
    descriptor.fragmentFunction = fragment;
    descriptor.colorAttachments[0].pixelFormat = layer.pixelFormat;
    descriptor.colorAttachments[0].blendingEnabled = NO;
    pipeline = [layer.device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    [descriptor release];
    [fragment release];
    [vertex release];
    [library release];
    if (!pipeline) {
        SDL_Log("Metal CRT disabled: pipeline: %s", error.localizedDescription.UTF8String);
        return false;
    }
    return true;
}

bool prepareResources(CAMetalLayer* layer, int width, int height)
{
#ifndef NDEBUG
    if (consumeDiagnosticFailure(DiagnosticFailure::Resources)) return false;
#endif
    if (snapshot && (snapshot.device != layer.device || snapshot.pixelFormat != layer.pixelFormat)) {
        [pipeline release];
        pipeline = nil;
        [snapshot release];
        snapshot = nil;
    }
    // Do not silently apply an sRGB transfer twice to an unfamiliar target.
    if (layer.pixelFormat != MTLPixelFormatBGRA8Unorm
        && layer.pixelFormat != MTLPixelFormatRGBA8Unorm) return false;
    if (!pipeline && !makePipeline(layer)) return false;
    if (!snapshot || snapshot.width != NSUInteger(width) || snapshot.height != NSUInteger(height)) {
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:layer.pixelFormat
            width:width height:height mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> replacement = [layer.device newTextureWithDescriptor:descriptor];
        if (!replacement) return false;
        replacement.label = @"Fallout reusable GPU window snapshot";
        // SDL's retained command buffers keep any old in-flight texture alive.
        [snapshot release];
        snapshot = replacement;
#ifndef NDEBUG
        SDL_Log("Metal CRT: allocated snapshot %dx%d format=%lu", width, height, (unsigned long)layer.pixelFormat);
#endif
    }
    return true;
}
} // namespace

// Keep the session API callable by Debug diagnostics before Settings uses it.
#ifndef NDEBUG
__attribute__((used))
#endif
IosCrtModel iosCrtGetModel()
{
    return activeModel;
}

#ifndef NDEBUG
__attribute__((used))
#endif
void iosCrtSetModel(IosCrtModel model)
{
    if (static_cast<unsigned>(model) >= sizeof(models) / sizeof(models[0]) || model == activeModel) return;
    if (model != IosCrtModel::Off && !rendererSupported) return;
    activeModel = model;
    reportedDraw = false;
#ifndef NDEBUG
    SDL_Log("Metal CRT: model=%s", models[static_cast<unsigned>(model)].name);
#endif
}

void iosCrtReset()
{
    releaseResources();
    rendererSupported = false;
}

void iosCrtInit(SDL_Renderer* renderer)
{
    iosCrtReset();
    SDL_RendererInfo info {};
    rendererSupported = SDL_GetRendererInfo(renderer, &info) == 0 && strcmp(info.name, "metal") == 0;
    if (!rendererSupported) {
        activeModel = IosCrtModel::Off;
        return;
    }
    placement = Placement::PostUi;
#ifndef NDEBUG
    const char* location = SDL_getenv("FALLOUT_CRT_PLACEMENT");
    if (location && strcmp(location, "pre") == 0) placement = Placement::PreUi;
    const char* diagnostic = SDL_getenv("FALLOUT_CRT_DIAGNOSTIC");
    diagnosticIdentity = diagnostic && strcmp(diagnostic, "identity") == 0;
#endif
}

void iosCrtDraw(SDL_Renderer* renderer, bool afterUi)
{
    if (activeModel == IosCrtModel::Off || !rendererSupported || afterUi != (placement == Placement::PostUi)) return;
    @autoreleasepool {
        CAMetalLayer* layer = (CAMetalLayer*)SDL_RenderGetMetalLayer(renderer);
        if (!layer || SDL_GetRenderTarget(renderer) != nullptr) return;
        int width = 0, height = 0;
        if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0 || width <= 0 || height <= 0) return;
        if (!prepareResources(layer, width, height)) {
            SDL_Log("Metal CRT disabled: resource initialization failed; keeping SDL rendering");
            activeModel = IosCrtModel::Off;
            releaseResources();
            return;
        }
        void* destination = (void*)snapshot;
#ifndef NDEBUG
        if (consumeDiagnosticFailure(DiagnosticFailure::Snapshot)) destination = nullptr;
#endif
        if (F2CE_SDLMetalCopyWindowToTexture(renderer, destination) != 0) {
            if (!reportedFailure) SDL_Log("Metal CRT: snapshot skipped: %s", SDL_GetError());
            reportedFailure = true;
            return; // Never sample stale content. SDL can render/present normally.
        }
        id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)SDL_RenderGetMetalCommandEncoder(renderer);
#ifndef NDEBUG
        if (consumeDiagnosticFailure(DiagnosticFailure::Encoder)) encoder = nil;
#endif
        if (!encoder) return;
        [encoder setRenderPipelineState:pipeline];
        [encoder setViewport:(MTLViewport){0, 0, double(width), double(height), 0, 1}];
        [encoder setScissorRect:(MTLScissorRect){0, 0, NSUInteger(width), NSUInteger(height)}];
        [encoder setCullMode:MTLCullModeNone];
        [encoder setFragmentTexture:snapshot atIndex:0];
        updateScaling(renderer, width, height);
#ifndef NDEBUG
        if (diagnosticIdentity) parameters.operation = 0;
#endif
        [encoder setFragmentBytes:&parameters length:sizeof(parameters) atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
#ifndef NDEBUG
        if (!reportedDraw) {
            SDL_Log("Metal CRT: drew %s-UI GPU snapshot at %dx%d framebufferOnly=%d", afterUi ? "post" : "pre",
                width, height, layer.framebufferOnly);
            reportedDraw = true;
        }
#endif
        // SDL restores its local state cache on its next batch and owns present.
    }
}
} // namespace fallout

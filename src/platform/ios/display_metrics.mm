#include "display_metrics.h"

#import <UIKit/UIKit.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include "../../svga.h"

// A transparent, noninteractive geometry observer. It follows the rendering
// view's bounds and safe-area changes without swizzling SDL or polling UIKit.
@interface FalloutDisplayObserver : UIView
@property(nonatomic) BOOL geometryDirty;
@end

@implementation FalloutDisplayObserver
- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.userInteractionEnabled = NO;
        self.opaque = NO;
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        self.geometryDirty = YES;
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        for (NSString* name in @[ UIApplicationDidBecomeActiveNotification,
                 UIApplicationWillEnterForegroundNotification,
                 UISceneDidActivateNotification, UISceneWillDeactivateNotification,
                 UIDeviceOrientationDidChangeNotification ]) {
            [center addObserver:self selector:@selector(invalidateGeometry:) name:name object:nil];
        }
    }
    return self;
}
- (void)invalidateGeometry:(NSNotification*)notification { self.geometryDirty = YES; }
- (void)layoutSubviews { [super layoutSubviews]; self.geometryDirty = YES; }
- (void)safeAreaInsetsDidChange { [super safeAreaInsetsDidChange]; self.geometryDirty = YES; }
- (void)didMoveToWindow { [super didMoveToWindow]; self.geometryDirty = YES; }
- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}
@end

namespace fallout {

IosDisplayGeometry iosDisplayMetricsGet()
{
    if (![NSThread isMainThread] || gSdlWindow == nullptr || gSdlRenderer == nullptr) {
        return {};
    }
    @autoreleasepool {
        static FalloutDisplayObserver* observer = nil;
        static IosDisplayGeometry cached;
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(gSdlWindow, &info) || info.subsystem != SDL_SYSWM_UIKIT) {
            return {};
        }
        UIWindow* window = info.info.uikit.window;
        UIView* view = window.rootViewController.view;
        if (!view || view.window != window || !window.windowScene) {
            cached = {};
            return cached; // Retry late attachment; no unavailable result is cached permanently.
        }
        if (observer.superview != view) {
            [observer removeFromSuperview];
            [observer release];
            observer = [[FalloutDisplayObserver alloc] initWithFrame:view.bounds];
            [view addSubview:observer];
        }
        int width = 0;
        int height = 0;
        if (SDL_GetRendererOutputSize(gSdlRenderer, &width, &height) != 0) {
            return {};
        }
        // A 180-degree landscape rotation can leave drawable dimensions and
        // symmetric insets unchanged. Checking this scene scalar also avoids
        // relying on device-orientation notification generation being enabled.
        UIInterfaceOrientation orientation = window.windowScene.interfaceOrientation;
        if (observer.geometryDirty || !cached.valid || cached.orientation != orientation
            || cached.outputWidth != width || cached.outputHeight != height) {
            IosDisplayGeometry next;
            next.pointsWidth = view.bounds.size.width;
            next.pointsHeight = view.bounds.size.height;
            next.outputWidth = width;
            next.outputHeight = height;
            UIEdgeInsets insets = view.safeAreaInsets;
            next.pointsInsets[0] = insets.left;
            next.pointsInsets[1] = insets.top;
            next.pointsInsets[2] = insets.right;
            next.pointsInsets[3] = insets.bottom;
            next.orientation = static_cast<int>(orientation);
            // Portrait/transitional geometry is not a valid landscape snapshot.
            if (UIInterfaceOrientationIsLandscape(orientation)) {
                iosResolveDisplayGeometry(next);
            }
            cached = next;
            observer.geometryDirty = !cached.valid;
            if (cached.valid) {
                SDL_Log("iOS display: output=%dx%d view=%.2fx%.2f pt density=%.4f,%.4f orientation=%d safePt=%.2f,%.2f,%.2f,%.2f",
                    width, height, next.pointsWidth, next.pointsHeight,
                    next.pixelsPerPointX, next.pixelsPerPointY, next.orientation,
                    insets.left, insets.top, insets.right, insets.bottom);
            }
        }
        return cached;
    }
}

void iosDisplayLogicalSafeInsets(const IosDisplayGeometry& geometry, int insets[4])
{
    if (!geometry.valid || !gSdlWindow || !gSdlRenderer) {
        std::fill(insets, insets + 4, 0);
        return;
    }
    // RenderGetViewport rounds its origin to logical integers. Inverting
    // WindowToLogical at window (0,0) retains SDL's exact viewport offset.
    float logicalX = 0;
    float logicalY = 0;
    float scaleX = 1;
    float scaleY = 1;
    SDL_RenderWindowToLogical(gSdlRenderer, 0, 0, &logicalX, &logicalY);
    SDL_RenderGetScale(gSdlRenderer, &scaleX, &scaleY);
    iosLogicalSafeInsets(geometry, screenGetWidth(), screenGetHeight(),
        -logicalX * scaleX, -logicalY * scaleY, scaleX, scaleY, insets);
}

} // namespace fallout

#ifndef FALLOUT_PLATFORM_IOS_CRT_RENDERER_H_
#define FALLOUT_PLATFORM_IOS_CRT_RENDERER_H_

struct SDL_Renderer;
namespace fallout {

enum class IosCrtModel : unsigned { Off, Tonytron, Arcade, ConsumerTv };

// Session-only selection: fresh processes start Off; GPU resets preserve it.
IosCrtModel iosCrtGetModel();
void iosCrtSetModel(IosCrtModel model);
void iosCrtInit(SDL_Renderer* renderer);
void iosCrtReset();
void iosCrtDraw(SDL_Renderer* renderer, bool afterUi);

// Debug only: FALLOUT_CRT_PLACEMENT=pre; FALLOUT_CRT_DIAGNOSTIC=identity.
// Geometry uses the SDL raster and attached-view presentation scale on every display.
} // namespace fallout
#endif

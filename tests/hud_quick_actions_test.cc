// Production layout and menu loop, with renderer/engine operations isolated.
#define SDL_MAIN_HANDLED
#include "../src/mui_options.cc"
#include "settings.h"
#include "hud_layout.h"
#include "game_commands.h"
#include "mui_icons.h"
#include "interface.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>

namespace fallout {
Settings settings;
GameQuitRequest _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
MessageList gMapMessageList;
MapElevation gElevation = ELEVATION_FIRST;
FpsLimiter sharedFpsLimiter;
FpsLimiter::FpsLimiter(unsigned int fps) : _fps(fps), _ticks(0) {}
void FpsLimiter::mark() {}
void FpsLimiter::throttle() const {}
int wmMapMaxCount() { return 1; }
static MuiTheme theme;
static std::map<std::string, MuiRect> rects;
static std::map<std::u32string, MuiRect> labelRects;
static std::map<MuiIcon, MuiRect> iconRects;
static MuiScreen* active;
static MuiContext context;
static float scrollOffset, px = -1, py = -1;
static MuiRect viewport;
static std::string tap;
static int step, saves, loads, confirms, posts;
static bool interfaceEnabled = true, confirm = false;
static GameCommandType posted;
static std::vector<std::string> script;
const MuiTheme& muiTheme() { return theme; }
const char* muiText(int, const char* fallback) { return fallback; }
std::u32string muiDecodeGameText(const char* text) { return std::u32string(text, text + strlen(text)); }
std::u32string muiDecodeUtf8(const char*) { return {}; }
void muiDrawTextAligned(const std::u32string& text, const MuiRect& rect, float, MuiColor, MuiAlign, MuiAlign)
{
    if (text == U"Quick Save" || text == U"Quick Load") labelRects[text] = rect;
}
float muiTextWidth(const std::u32string& text, float size) { return text.size() * size * 0.6f; }
float muiFitTextSize(const std::u32string& text, float width, float size, float minimum)
{
    return std::max(minimum, std::min(size, width / (text.size() * 0.6f)));
}
void muiDrawIcon(MuiIcon icon, float x, float y, float size, MuiColor)
{
    iconRects[icon] = { x - size / 2, y - size / 2, size, size };
}
void muiPushClip(const MuiRect&) {}
void muiPopClip() {}
void MuiContext::dim(bool) {}
void MuiContext::panel(const MuiRect&) {}
bool MuiContext::tappedOutside(const MuiRect&) { return false; }
float MuiContext::pointerX() const { return px; }
float MuiContext::pointerY() const { return py; }
float MuiContext::scroll(const std::string&, const MuiRect& rect, float height, bool)
{
    viewport = rect;
    return std::clamp(scrollOffset, 0.0f, std::max(0.0f, height - rect.h));
}
bool MuiContext::button(const std::string& id, const MuiRect& rect, const std::u32string&, MuiButtonStyle)
{
    rects[id] = rect;
    return interactive() && id == tap && rect.contains(px, py);
}
void muiPush(MuiScreen* screen) { assert(!active); active = screen; screen->finished = false; }
void muiRemove(MuiScreen* screen) { assert(active == screen); active = nullptr; }
bool interfaceBarEnabled() { return interfaceEnabled; }
bool touchHudConfirmQuickLoad() { assert(!active); confirms++; return interfaceEnabled && confirm; }
void gameCommandPost(GameCommandType type, int) { assert(!active); posts++; posted = type; }
int lsgSaveGame(int mode) { assert(mode == LOAD_SAVE_MODE_NORMAL); saves++; return 1; }
int lsgLoadGame(int mode) { assert(mode == LOAD_SAVE_MODE_NORMAL); loads++; return 1; }
int doPreferences(bool) { return 0; }
int showQuitConfirmationDialog() { return 0; }
int inputGetInput()
{
    assert(step < static_cast<int>(script.size()));
    tap = script[step++];
    return tap == "back" ? KEY_ESCAPE : -1;
}
void devAutotestTick() {}
void renderPresent()
{
    if (!active) return;
    std::string selected = tap;
    tap.clear(); active->build(context);
    tap = selected;
    auto it = rects.find(tap);
    if (it != rects.end()) { px = it->second.centerX(); py = it->second.centerY(); }
    active->build(context);
}
void gameTimeGetDate(int* month, int* day, int* year) { *month = *day = 1; *year = 2241; }
unsigned int gameTimeGetTime() { return 0; }
Map mapGetCurrentMap() { return static_cast<Map>(0); }
int wmMatchAreaContainingMapIdx(Map, City*) { return -1; }
bool messageListGetItem(MessageList*, MessageListItem*) { return false; }
}
using namespace fallout;
static bool same(HudRect a, HudRect b) { return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height; }
static std::map<HudElementId, HudRect> placed(const HudMetrics& metrics)
{
    std::map<HudElementId, HudRect> result;
    for (const auto& group : hudLayoutPlace(hudLayoutLandscape(), metrics)) {
        for (const auto& element : group.elements) {
            auto rect = element.rect;
            rect.x += group.rect.x; rect.y += group.rect.y;
            result[element.id] = rect;
        }
    }
    return result;
}
int main()
{
    theme.buttonTextSize = 16;
    for (float scale : {1.0f, 1.25f, 1.5f}) {
#ifdef __IPHONEOS__
        settings.touch.game_menu_quick_actions = false;
#endif
        HudMetrics metrics { 956, 440, scale, 62, 0, 62, 20 };
        auto baseline = placed(metrics);
        assert(baseline.count(HudElementId::QuickSave) && baseline.count(HudElementId::QuickLoad));
#ifdef __IPHONEOS__
        assert(!TouchSettings().game_menu_quick_actions);
        settings.touch.game_menu_quick_actions = true;
        auto compact = placed(metrics);
        assert(!compact.count(HudElementId::QuickSave) && !compact.count(HudElementId::QuickLoad));
        // The noninteractive indicator strip still avoids actual group bounds;
        // reclaiming system space can let it settle closer to its anchor.
        for (const auto& item : compact) {
            if (item.first != HudElementId::Indicators) assert(same(item.second, baseline.at(item.first)));
        }
        assert(hudLayoutPlace(hudLayoutLandscape(), metrics)[0].rect.width == hudDpToPixels(metrics, 44));
        settings.touch.game_menu_quick_actions = false;
        auto restored = placed(metrics);
        for (const auto& item : baseline) assert(same(item.second, restored.at(item.first)));
#endif
        for (float height : {440.0f, 260.0f}) {
            context.pixelsPerDp = scale;
            context.safe = {62, 0, 832, height - 20};
            context.isInteractive = true;
            for (bool on : {true, false}) {
#ifdef __IPHONEOS__
                settings.touch.game_menu_quick_actions = on;
                bool quick = on;
#else
                bool quick = false;
#endif
                for (bool saveLoad : {true, false}) {
                    GameMenuScreen menu(saveLoad, nullptr);
                    rects.clear(); labelRects.clear(); iconRects.clear(); scrollOffset = 0;
                    menu.build(context);
                    assert(rects.count("menu.save") == saveLoad && rects.count("menu.load") == saveLoad);
                    assert(rects.count("menu.quick_save") == (quick && saveLoad));
                    assert(rects.count("menu.quick_load") == (quick && saveLoad));
                    if (!(quick && saveLoad)) {
                        assert(labelRects.empty() && iconRects.empty());
                    }
                    if (quick && saveLoad) {
                        const auto& quickSave = rects.at("menu.quick_save");
                        const auto& quickLoad = rects.at("menu.quick_load");
                        const auto& cont = rects.at("menu.continue");
                        assert(quickSave.h == 44 * scale && quickLoad.h == quickSave.h);
                        assert(quickSave.y == cont.bottom() + 6 * scale);
                        assert(quickSave.y == quickLoad.y && quickSave.w == quickLoad.w);
                        assert(quickSave.x == cont.x && quickLoad.right() == cont.right());
                        assert(quickLoad.x == quickSave.right() + 6 * scale);
                        assert(rects.at("menu.save").y == quickSave.bottom() + 6 * scale);
                        for (int i = 0; i < 2; i++) {
                            const auto& frame = i == 0 ? quickSave : quickLoad;
                            const auto& label = labelRects.at(i == 0 ? U"Quick Save" : U"Quick Load");
                            const auto& icon = iconRects.at(i == 0 ? MuiIcon::QuickSave : MuiIcon::QuickLoad);
                            assert(icon.x >= frame.x && icon.right() < label.x);
                            assert(label.right() <= frame.right());
                            assert(label.y == frame.y && label.h == frame.h);
                            assert(icon.y >= frame.y && icon.bottom() <= frame.bottom());
                        }
                        tap = "menu.quick_save";
                        menu.pending = GameMenuScreen::Action::None;
                        menu.build(context); // Pointer outside viewport gates every child.
                        assert(menu.pending == GameMenuScreen::Action::None);
                        scrollOffset = 10000;
                        menu.build(context);
                        assert(rects.at("menu.exit").bottom() <= viewport.bottom() + 0.01f);
                    }
                    tap.clear();
                }
            }
        }
    }
#ifdef __IPHONEOS__
    settings.touch.game_menu_quick_actions = true;
    context.safe = {0, 0, 956, 440}; context.pixelsPerDp = 1;
    px = 450; py = 100; scrollOffset = 0;
    for (bool yes : {false, true}) {
        confirm = yes; step = confirms = posts = 0;
        script = {"menu.quick_load", "back"};
        muiGameMenuRun(true, nullptr);
        assert(confirms == 1 && posts == (yes ? 1 : 0));
        assert(step == (yes ? 1 : 2)); // No returns to menu after acceptance.
        if (yes) assert(posted == GameCommandType::QuickLoad);
        assert(!active && saves == 0 && loads == 0);
    }
    step = posts = 0; script = {"menu.quick_save"};
    muiGameMenuRun(true, nullptr);
    assert(posts == 1 && posted == GameCommandType::QuickSave && saves == 0);
    interfaceEnabled = false;
    step = posts = 0; script = {"menu.quick_save", "menu.quick_load", "back"};
    muiGameMenuRun(true, nullptr); assert(posts == 0);
    interfaceEnabled = true;
#endif
    for (const auto& action : {"menu.save", "menu.load"}) {
        step = 0; script = {action}; muiGameMenuRun(true, nullptr);
    }
    assert(saves == 1 && loads == 1);
}

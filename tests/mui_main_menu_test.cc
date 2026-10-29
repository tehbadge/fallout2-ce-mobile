// Exercise the production menu without game assets or a renderer. Scrolling
// supplies clamped offsets; actual gesture arbitration is covered by mui_slider_test.
#define SDL_MAIN_HANDLED
#include "../src/mui_main_menu.cc"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <unordered_map>

#include "input.h"
#include "kb.h"

namespace fallout {
Settings settings;
const char* gGameMsgPath = "";
MessageList gMiscMessageList;
static std::unordered_map<std::string, MuiRect> rects;
static MuiRect view, footer;
static float content, requestedOffset, px, py;
static int newestSlot = -1;
static std::unordered_map<std::string, bool> enabled;
static MuiTheme theme;
GameQuitRequest _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
unsigned char gPressedPhysicalKeys[SDL_NUM_SCANCODES] = {};
FpsLimiter sharedFpsLimiter;
FpsLimiter::FpsLimiter(unsigned int fps) : _fps(fps), _ticks(0) {}
void FpsLimiter::mark() {}
void FpsLimiter::throttle() const {}
FrmImage::FrmImage() = default;
FrmImage::~FrmImage() = default;
static std::vector<int> keys;
static size_t nextKey;
static bool requestBack;
int inputGetInput()
{
    assert(nextKey < keys.size()); // A suppressed exit must keep pumping input.
    if (requestBack) {
        requestBack = false;
        gMainMenuScreen->back();
    }
    return keys[nextKey++];
}
unsigned int getTicks() { return 0; }
unsigned int getTicksSince(unsigned int) { return 0; }
bool cursorIsHidden() { return false; }
void mouseShowCursor() {}
void mouseHideCursor() {}
int mouseGetEvent() { return 0; }
int soundPlayFile(const char*) { return 0; }
void brightnessIncrease() {}
void brightnessDecrease() {}
void devAutotestTick() {}
void renderPresent() {}
#if !FALLOUT_TOUCH_ONLY
bool muiIsEnabled() { return true; }
#endif
const MuiTheme& muiTheme() { return theme; }
bool muiReduceMotion() { return false; }
float MuiContext::appear() const { return 1; }
float muiLayerAlpha() { return 1; }
void muiSetLayerAlpha(float) {}
int lsgMobileNewestSlot() { return newestSlot; }
bool lsgMobileGetSlotInfo(int, MobileSaveSlotInfo*) { return false; }
bool messageListInit(MessageList*) { return false; }
bool messageListFree(MessageList*) { return true; }
bool messageListLoad(MessageList*, const char*) { return false; }
bool messageListGetItem(MessageList*, MessageListItem*) { return false; }
std::u32string muiPlaceText(int, int, MessageList*) { return {}; }
std::u32string muiDecodeGameText(const char*) { return U"text"; }
const char* muiText(int, const char* fallback) { return fallback; }
void muiPush(MuiScreen*) {}
void muiRemove(MuiScreen*) {}
void versionGetVersion(char* v, size_t) { v[0] = 0; }
void muiFillRect(const MuiRect&, MuiColor) {}
void muiFillRoundRect(const MuiRect&, float, MuiColor) {}
void muiFillHorizontalGradient(const MuiRect&, MuiColor, MuiColor) {}
void muiDrawTexturePart(SDL_Texture*, const SDL_Rect&, const MuiRect&, MuiColor) {}
SDL_Texture* muiPictureTexture(const FrmId&, int*, int*) { return nullptr; }
void muiDrawTextAligned(const std::u32string&, const MuiRect& rect, float, MuiColor, MuiAlign, MuiAlign vertical)
{
    if (vertical == MuiAlign::End) footer = rect;
}
float muiFitTextSize(const std::u32string&, float, float size, float) { return size; }
void muiPushClip(const MuiRect&) {}
void muiPopClip() {}
float MuiContext::pointerX() const { return px; }
float MuiContext::pointerY() const { return py; }
float MuiContext::scroll(const std::string& id, const MuiRect& rect, float height, bool)
{
    assert(id == "mainmenu");
    view = rect;
    content = height;
    return std::clamp(requestedOffset, 0.0f, std::max(height - rect.h, 0.0f));
}

bool MuiContext::touchable(const std::string& id, const MuiRect& rect, bool* pressed, bool*)
{
    rects[id] = rect;
    enabled[id] = isInteractive;
    if (pressed != nullptr) *pressed = false;
    return isInteractive && rect.contains(px, py);
}

bool MuiContext::button(const std::string& id, const MuiRect& rect, const std::u32string&, MuiButtonStyle)
{
    return touchable(id, rect, nullptr, nullptr);
}
} // namespace fallout

using namespace fallout;

static bool near(float a, float b)
{
    return std::fabs(a - b) < 0.01f;
}

static void checkGating(MainMenuScreen& menu, MuiContext& ui, float x, float y, bool interactive)
{
    px = x;
    py = y;
    ui.isInteractive = interactive;
    menu.choice = -1;
    menu.build(ui);
    bool expected = interactive && view.contains(x, y);
    for (const auto& button : enabled) assert(button.second == expected);
    assert(ui.isInteractive == interactive);
    if (!expected) assert(menu.choice == -1);
}

int main()
{
#ifdef __IPHONEOS__
    constexpr bool hasExit = false;
    constexpr const char* lastButton = "mainmenu.credits";
#else
    constexpr bool hasExit = true;
    constexpr const char* lastButton = "mainmenu.exit";
#endif
    // Exercise production dispatch, not just the screen's pending choice.
    muiMainMenuShow();
    for (int key : {KEY_ESCAPE, KEY_LOWERCASE_E, KEY_UPPERCASE_E}) {
        keys = {key, KEY_LOWERCASE_N};
        nextKey = 0;
        assert(mainMenuWindowHandleEvents() == (hasExit ? MAIN_MENU_EXIT : MAIN_MENU_NEW_GAME));
        assert(nextKey == (hasExit ? 1 : 2));
    }
    requestBack = true;
    keys = {-1, KEY_LOWERCASE_N};
    nextKey = 0;
    assert(mainMenuWindowHandleEvents() == (hasExit ? MAIN_MENU_EXIT : MAIN_MENU_NEW_GAME));

    // signal_close_game sets these two independent requests. On iOS only
    // its return-to-title signal survives; it is consumed without a stale exit.
    mainMenuRequestExit();
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_MAIN_MENU;
    keys = {-1, KEY_LOWERCASE_N};
    nextKey = 0;
    assert(mainMenuWindowHandleEvents() == (hasExit ? MAIN_MENU_EXIT : MAIN_MENU_NEW_GAME));
    assert(_game_user_wants_to_quit == (hasExit ? GAME_QUIT_REQUEST_EXIT : GAME_QUIT_REQUEST_NONE));
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
    keys = {KEY_LOWERCASE_N};
    nextKey = 0;
    assert(mainMenuWindowHandleEvents() == MAIN_MENU_NEW_GAME);
    // Explicit engine shutdown requests remain distinct from title shortcuts.
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
    keys = {-1};
    nextKey = 0;
    assert(mainMenuWindowHandleEvents() == MAIN_MENU_EXIT);
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
    muiMainMenuHide();

    for (float screenHeight : {440.0f, 402.0f, 280.0f, 480.0f}) {
        for (int scale : {100, 125, 150, 200}) {
            // Saves may exist with Continue disabled; Load must still appear.
            for (int saveState : {0, 1, 2}) {
                bool hasSaves = saveState != 0;
                bool hasContinue = saveState == 2;
                settings.enhancements.main_menu_continue = hasContinue;
                MuiContext ui;
                ui.pixelsPerDp = 3.0f * scale / 100.0f;
                ui.screen = {0, 0, 2868, screenHeight * 3};
                ui.safe = {186, 0, 2496, (screenHeight - 20) * 3};
                newestSlot = hasSaves ? 0 : -1;
                MainMenuScreen menu;
                float topRowY = 0;
                float scrollRange = 0;
                for (bool bottom : {false, true}) {
                    rects.clear();
                    enabled.clear();
                    requestedOffset = bottom ? 100000.0f : 0;
                    checkGating(menu, ui, ui.safe.x, ui.safe.y, true);
                    assert(rects.size() == 4 + (hasSaves ? 1 : 0) + (hasContinue ? 1 : 0) + (hasExit ? 1 : 0));
                    assert(rects.count("mainmenu.exit") == (hasExit ? 1 : 0));
                    assert(rects.count("mainmenu.load") == (hasSaves ? 1 : 0));
                    for (const char* id : {"mainmenu.new", "mainmenu.load", "mainmenu.settings"}) {
                        if (std::string(id) == "mainmenu.load" && !hasSaves) continue;
                        assert(near(rects.at(id).h, ui.dp(44)));
                        assert(near(rects.at(id).x, ui.safe.x + ui.dp(24)));
                        assert(near(rects.at(id).w, std::min(ui.dp(300), ui.safe.w * 0.42f)));
                    }
                    for (const char* id : {"mainmenu.intro", "mainmenu.credits", "mainmenu.exit"}) {
                        if (!hasExit && std::string(id) == "mainmenu.exit") continue;
                        assert(near(rects.at(id).h, ui.dp(38)));
                        assert(near(rects.at(id).y, rects.at(lastButton).y));
                        assert(near(rects.at(id).w, (view.w - ui.dp(8) * (hasExit ? 2 : 1)) / (hasExit ? 3 : 2)));
                    }
                    assert(near(rects.at("mainmenu.credits").x - rects.at("mainmenu.intro").right(), ui.dp(8)));
                    if (hasExit) assert(near(rects.at("mainmenu.exit").x - rects.at("mainmenu.credits").right(), ui.dp(8)));
                    assert(near(rects.at(lastButton).right(), view.right()));
                    if (hasContinue) assert(near(rects.at("mainmenu.continue").h, ui.dp(56)));
                    assert(near(view.bottom(), footer.y));
                    assert(near(footer.h, ui.dp(30)));
                    assert(footer.bottom() <= ui.safe.bottom() + 0.01f);
                    const MuiRect& first = rects.at(hasContinue ? "mainmenu.continue" : "mainmenu.new");
                    if (bottom) {
                        assert(rects.at(lastButton).bottom() <= view.bottom() + 0.01f);
                        assert(near(topRowY - rects.at(lastButton).y, scrollRange));
                    } else {
                        assert(first.y >= view.y - 0.01f);
                        topRowY = rects.at(lastButton).y;
                        scrollRange = std::max(content - view.h, 0.0f);
                        if (view.contains(first.centerX(), first.centerY())) {
                            checkGating(menu, ui, first.centerX(), first.centerY(), true);
                            assert(menu.choice == (hasContinue ? MAIN_MENU_CONTINUE : MAIN_MENU_NEW_GAME));
                        }
                    }
                    // A roomy layout preserves the original centered bounds.
                    if (screenHeight == 480 && scale == 100) {
                        assert(near(view.y, ui.safe.y + ui.dp(24)));
                        assert(near(footer.y, ui.safe.bottom() - ui.dp(54)));
                        float height = ui.dp((hasSaves ? 3 : 2) * 52 + 38 + (hasContinue ? 64 : 0));
                        assert(near(first.y, view.y + (view.h - height) / 2));
                        assert(scrollRange == 0);
                    }
                    // Footer, side margins and space above the viewport cannot
                    // activate even a scrolled button. Parent gating also survives.
                    checkGating(menu, ui, view.x + 1, footer.y + 1, true);
                    checkGating(menu, ui, view.x - 1, view.y + 1, true);
                    checkGating(menu, ui, view.right() + 1, view.y + 1, true);
                    checkGating(menu, ui, view.x + 1, view.y - 1, true);
                    checkGating(menu, ui, view.x + 1, view.y + 1, false);
                    checkGating(menu, ui, view.x + 1, view.y + 1, true);
                    if (bottom) {
                        const MuiRect settingsButton = rects.at("mainmenu.settings");
                        if (view.contains(settingsButton.centerX(), settingsButton.centerY())) {
                            checkGating(menu, ui, settingsButton.centerX(), settingsButton.centerY(), true);
                            assert(menu.choice == MAIN_MENU_OPTIONS);
                        }
                        MuiRect last = rects.at(lastButton);
                        checkGating(menu, ui, last.centerX(), last.centerY(), true);
                        assert(menu.choice == (hasExit ? MAIN_MENU_EXIT : MAIN_MENU_CREDITS));
                        // The former Exit area belongs to Credits on iOS,
                        // with no hidden Exit input left behind.
                        float oldWidth = (view.w - ui.dp(16)) / 3;
                        checkGating(menu, ui, view.right() - oldWidth / 2, last.centerY(), true);
                        assert(menu.choice == (hasExit ? MAIN_MENU_EXIT : MAIN_MENU_CREDITS));
                    } else if (rects.at(lastButton).centerY() >= view.bottom()) {
                        MuiRect last = rects.at(lastButton);
                        checkGating(menu, ui, last.centerX(), last.centerY(), true);
                        assert(menu.choice == -1);
                    }
                }
            }
        }
    }
    std::puts("PASS: title Back/key/script-close dispatch and 48 menu configurations, Exit visibility, layout, Settings, scrolling and input gating");
}

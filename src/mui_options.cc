#include "mui.h"
#include "mui_screens.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "dev_autotest.h"
#include "fps_limiter.h"
#include "game.h"
#include "input.h"
#ifdef __IPHONEOS__
#include "game_commands.h"
#include "interface.h"
#include "mui_icons.h"
#include "settings.h"
#include "touch_hud.h"
#endif
#include "kb.h"
#include "loadsave.h"
#include "map.h"
#include "message.h"
#include "preferences.h"
#include "scripts.h"
#include "svga.h"
#include "worldmap.h"

namespace fallout {

namespace {

    // Texts in `game\ce.msg`.
#ifdef __IPHONEOS__
    constexpr int kTextQuickSave = 497;
    constexpr int kTextQuickLoad = 498;
#endif
    constexpr int kTextContinue = 214;
    constexpr int kTextSaveGame = 215;
    constexpr int kTextLoadGame = 216;
    constexpr int kTextSettings = 217;
    constexpr int kTextExit = 218;

    // Game menu (the game's options window): where and when at the top,
    // continue, save, load, settings, exit (on the world map without save
    // and load, as the game has no options there). A tap outside or back
    // continues.
    class GameMenuScreen : public MuiScreen {
    public:
        enum class Action {
            None,
            Save,
            Load,
            Settings,
            Exit,
#ifdef __IPHONEOS__
            QuickSave,
            QuickLoad,
#endif
        };

        GameMenuScreen(bool saveLoad, const std::u32string* place)
            : saveLoad(saveLoad)
            , hasPlace(place != nullptr)
            , place(place != nullptr ? *place : std::u32string())
        {
            modal = true;
        }

        void build(MuiContext& ui) override;
        void back() override { finished = true; }
        void key(int keyCode) override;

        Action pending = Action::None;

    private:
        bool saveLoad;
        // The place in the header, the current map's without it.
        bool hasPlace;
        std::u32string place;
    };

    // The game's window keys.
    void GameMenuScreen::key(int keyCode)
    {
        switch (keyCode) {
        case KEY_RETURN:
        case KEY_LOWERCASE_O:
        case KEY_UPPERCASE_O:
        case KEY_LOWERCASE_D:
        case KEY_UPPERCASE_D:
            back();
            break;
        case KEY_LOWERCASE_S:
        case KEY_UPPERCASE_S:
            if (saveLoad) {
                pending = Action::Save;
            }
            break;
        case KEY_LOWERCASE_L:
        case KEY_UPPERCASE_L:
            if (saveLoad) {
                pending = Action::Load;
            }
            break;
        case KEY_LOWERCASE_P:
        case KEY_UPPERCASE_P:
            pending = Action::Settings;
            break;
        case KEY_LOWERCASE_E:
        case KEY_UPPERCASE_E:
        case KEY_F10:
            pending = Action::Exit;
            break;
        }
    }

    void GameMenuScreen::build(MuiContext& ui)
    {
        const MuiTheme& theme = muiTheme();
        ui.dim();

        MuiRect safe = ui.safeRect().inset(ui.dp(8.0f));
        float padding = ui.dp(12.0f);
        float gap = ui.dp(6.0f);
        float headerHeight = ui.dp(27.0f);

        struct Button {
            const char* id;
            int textId;
            const char* fallback;
            Action action;
        };
        const Button allButtons[] = {
            { "menu.continue", kTextContinue, "Continue", Action::None },
            { "menu.save", kTextSaveGame, "Save game", Action::Save },
            { "menu.load", kTextLoadGame, "Load game", Action::Load },
            { "menu.settings", kTextSettings, "Settings", Action::Settings },
            { "menu.exit", kTextExit, "Exit", Action::Exit },
        };
        const Button* buttons[5];
        int buttonCount = 0;
        for (const Button& button : allButtons) {
            if (saveLoad || (button.action != Action::Save && button.action != Action::Load)) {
                buttons[buttonCount++] = &button;
            }
        }

#ifdef __IPHONEOS__
        bool quickActions = saveLoad && settings.touch.game_menu_quick_actions;
#else
        constexpr bool quickActions = false;
#endif
        float buttonHeight = std::min(ui.dp(44.0f), (safe.h - padding * 2.0f - headerHeight - gap * (buttonCount - 1)) / buttonCount);
        // Only the optional row needs scrolling; retain the original fitting
        // menu exactly when it is absent. Never shrink the added touch targets.
        if (quickActions) buttonHeight = ui.dp(44.0f);
        float width = std::min(safe.w, ui.dp(312.0f));
        float height = padding * 2.0f + headerHeight + buttonHeight * buttonCount + gap * (buttonCount - 1);
        float contentHeight = height - padding * 2.0f - headerHeight;
        if (quickActions) {
            contentHeight += buttonHeight + gap;
            height = std::min(safe.h, height + buttonHeight + gap);
        }
        MuiRect card = { safe.centerX() - width / 2.0f, safe.centerY() - height / 2.0f, width, height };

        if (ui.tappedOutside(card)) {
            back();
        }

        ui.panel(card);

        // Where and the game's date and time.
        MuiRect header = { card.x + padding, card.y + padding, card.w - padding * 2.0f, headerHeight };
        int month;
        int day;
        int year;
        gameTimeGetDate(&month, &day, &year);
        unsigned int minutes = gameTimeGetTime() / 600;
        char date[48];
        snprintf(date, sizeof(date), "%02d.%02d.%04d  %02u:%02u", day, month, year, minutes / 60 % 24, minutes % 60);
        MuiRect placeRect = { header.x, header.y, header.w * 0.52f, header.h };
        MuiRect dateRect = { placeRect.right(), header.y, header.w - placeRect.w, header.h };
        muiDrawTextAligned(hasPlace ? place : muiPlaceText(mapGetCurrentMap(), gElevation), placeRect, ui.dp(12.0f), theme.textDim, MuiAlign::Start, MuiAlign::Center);
        muiDrawTextAligned(muiDecodeUtf8(date), dateRect, ui.dp(11.0f), theme.textDim, MuiAlign::End, MuiAlign::Center);

        MuiContext buttonsUi = ui;
        MuiRect viewport = { header.x, header.bottom(), header.w, card.bottom() - padding - header.bottom() };
        float y = header.bottom();
        if (quickActions) {
            y -= ui.scroll("menu", viewport, contentHeight);
            muiPushClip(viewport);
            buttonsUi.isInteractive = ui.interactive() && viewport.contains(ui.pointerX(), ui.pointerY());
        }
        for (int index = 0; index < buttonCount; index++) {
            const Button& button = *buttons[index];
            MuiRect rect = { header.x, y, header.w, buttonHeight };
            MuiButtonStyle style = button.action == Action::None ? MuiButtonStyle::Primary : MuiButtonStyle::Normal;
            if (buttonsUi.button(button.id, rect, muiDecodeGameText(muiText(button.textId, button.fallback)), style)) {
                if (button.action == Action::None) {
                    back();
                } else {
                    pending = button.action;
                }
            }
            y += buttonHeight + gap;
#ifdef __IPHONEOS__
            if (quickActions && button.action == Action::None) {
                MuiContext quickUi = buttonsUi;
                quickUi.isInteractive = buttonsUi.interactive() && interfaceBarEnabled();
                float halfWidth = (header.w - gap) / 2.0f;
                for (int quickIndex = 0; quickIndex < 2; quickIndex++) {
                    MuiRect quickRect = { header.x + quickIndex * (halfWidth + gap), y, halfWidth, buttonHeight };
                    if (quickUi.button(quickIndex == 0 ? "menu.quick_save" : "menu.quick_load", quickRect, U"")) {
                        pending = quickIndex == 0 ? Action::QuickSave : Action::QuickLoad;
                    }
                    // Reuse the normal button's frame and pressed feedback;
                    // center its existing icon and fitted label as one group.
                    std::u32string label = muiDecodeGameText(quickIndex == 0
                            ? muiText(kTextQuickSave, "Quick Save") : muiText(kTextQuickLoad, "Quick Load"));
                    float iconSize = ui.dp(20.0f);
                    float labelSize = muiFitTextSize(label, quickRect.w - ui.dp(16.0f) - iconSize - gap,
                        ui.dp(theme.buttonTextSize), ui.dp(10.0f));
                    float textWidth = muiTextWidth(label, labelSize);
                    float x = quickRect.centerX() - (iconSize + gap + textWidth) / 2.0f;
                    muiDrawIcon(quickIndex == 0 ? MuiIcon::QuickSave : MuiIcon::QuickLoad,
                        x + iconSize / 2.0f, quickRect.centerY(), iconSize, theme.buttonText);
                    MuiRect labelRect = { x + iconSize + gap, quickRect.y, textWidth, quickRect.h };
                    muiDrawTextAligned(label, labelRect, labelSize, theme.buttonText, MuiAlign::Start, MuiAlign::Center);
                }
                y += buttonHeight + gap;
            }
#endif
        }
        if (quickActions) muiPopClip();
    }

} // namespace

std::u32string muiPlaceText(int map, int elevation)
{
    return muiPlaceText(map, elevation, &gMapMessageList);
}

std::u32string muiPlaceText(int map, int elevation, MessageList* names)
{
    if (!mapIsValid(static_cast<Map>(map)) || !elevationIsValid(elevation)) {
        return std::u32string();
    }

    // As `mapGetCityName` and `mapGetName`, without their "Error" for a
    // missing name.
    std::u32string place;
    MessageListItem item;
    City city;
    if (wmMatchAreaContainingMapIdx(static_cast<Map>(map), &city) == 0) {
        item.num = 1500 + city;
        if (messageListGetItem(names, &item)) {
            place = muiDecodeGameText(item.text);
        }
    }

    item.num = map * 3 + elevation + 200;
    if (messageListGetItem(names, &item)) {
        if (!place.empty()) {
            place += U" · ";
        }
        place += muiDecodeGameText(item.text);
    }

    return place;
}

int muiOptionsScreenRun()
{
    return muiGameMenuRun(true, nullptr);
}

int muiGameMenuRun(bool saveLoad, const std::u32string* place)
{
    GameMenuScreen screen(saveLoad, place);
    muiPush(&screen);

    // As the game's window: 1 - a game was saved or loaded.
    int result = 0;
#ifdef __IPHONEOS__
    GameCommandType deferred = GameCommandType::None;
#endif
    while (!screen.finished) {
        sharedFpsLimiter.mark();

        int keyCode = inputGetInput();
        if (keyCode == KEY_ESCAPE) {
            screen.back();
        } else if (keyCode != -1) {
            screen.key(keyCode);
        }

        if (_game_user_wants_to_quit != GAME_QUIT_REQUEST_NONE) {
            screen.finished = true;
        }

        devAutotestTick();
        renderPresent();

        // What was tapped runs after the frame (its own screens).
        GameMenuScreen::Action action = screen.pending;
        screen.pending = GameMenuScreen::Action::None;
        switch (action) {
        case GameMenuScreen::Action::Save:
            if (lsgSaveGame(LOAD_SAVE_MODE_NORMAL) == 1) {
                result = 1;
                screen.finished = true;
            }
            break;
        case GameMenuScreen::Action::Load:
            if (lsgLoadGame(LOAD_SAVE_MODE_NORMAL) == 1) {
                result = 1;
                screen.finished = true;
            }
            break;
        case GameMenuScreen::Action::Settings:
            doPreferences(false);
            break;
        case GameMenuScreen::Action::Exit:
            showQuitConfirmationDialog();
            break;
#ifdef __IPHONEOS__
        case GameMenuScreen::Action::QuickSave:
            if (saveLoad && interfaceBarEnabled()) {
                deferred = GameCommandType::QuickSave;
                screen.finished = true;
            }
            break;
        case GameMenuScreen::Action::QuickLoad: {
            // Remove the menu before the modal question. Cancellation restores
            // the same screen (and its scroll); no load runs inside either UI.
            muiRemove(&screen);
            bool accepted = saveLoad && touchHudConfirmQuickLoad();
            muiPush(&screen);
            if (accepted) {
                deferred = GameCommandType::QuickLoad;
                screen.finished = true;
            }
            break;
        }
#endif
        case GameMenuScreen::Action::None:
            break;
        }

        sharedFpsLimiter.throttle();
    }

    muiRemove(&screen);
#ifdef __IPHONEOS__
    if (deferred != GameCommandType::None && _game_user_wants_to_quit == GAME_QUIT_REQUEST_NONE) {
        gameCommandPost(deferred);
    }
#endif
    return result;
}

} // namespace fallout

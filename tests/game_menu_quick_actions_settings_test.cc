// Production registry/read/write paths; isolate the engine's config storage.
#define SDL_MAIN_HANDLED
#include "../src/settings.cc"

#include <map>
#include <cstring>

namespace fallout {
Config gGameConfig;
static std::map<std::pair<std::string, std::string>, std::string> config, disk;
bool configGetString(Config*, const char* section, const char* key, char** value)
{
    auto it = config.find({section, key});
    if (it == config.end()) return false;
    *value = it->second.data();
    return true;
}
bool configGetInt(Config* c, const char* section, const char* key, int* value)
{
    char* text;
    if (!configGetString(c, section, key, &text)) return false;
    *value = std::stoi(text); return true;
}
bool configGetBool(Config* c, const char* section, const char* key, bool* value)
{
    int number;
    if (!configGetInt(c, section, key, &number)) return false;
    *value = number != 0; return true;
}
bool configGetDouble(Config* c, const char* section, const char* key, double* value)
{
    char* text;
    if (!configGetString(c, section, key, &text)) return false;
    *value = std::stod(text); return true;
}
bool configSetString(Config*, const char* section, const char* key, const char* value)
{
    config[{section, key}] = value; return true;
}
bool configSetInt(Config* c, const char* section, const char* key, int value)
{
    return configSetString(c, section, key, std::to_string(value).c_str());
}
bool configSetBool(Config* c, const char* section, const char* key, bool value)
{
    return configSetInt(c, section, key, value);
}
bool configSetDouble(Config* c, const char* section, const char* key, double value)
{
    return configSetString(c, section, key, std::to_string(value).c_str());
}
bool gameConfigInit(bool, int, char**) { config = disk; return true; }
bool gameConfigSave() { disk = config; return true; }
int compat_stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
void compat_windows_path_to_native(char*) {}
void compat_resolve_path(char*) {}
int debugPrint(const char*, ...) { return 0; }
}

using namespace fallout;
static void restart()
{
    settings = Settings();
    assert(settingsInit(false, 0, nullptr));
}
int main()
{
    restart();
    assert(!settings.touch.game_menu_quick_actions);
    const auto& descriptors = settingsGetDescriptors();
    auto it = std::find_if(descriptors.begin(), descriptors.end(), [](const SettingDescriptor& d) {
        return d.id == "touch.game_menu_quick_actions";
    });
    assert(it != descriptors.end());
    assert(!std::get<bool>(it->defaultValue));
    assert(std::none_of(descriptors.begin(), descriptors.end(), [](const SettingDescriptor& d) {
        return d.key == "hud_quick_actions"; // Negative fixture only: no legacy registration.
    }));
    for (bool old : {false, true}) {
        disk.clear();
        disk[{"touch", "hud_quick_actions"}] = old ? "1" : "0";
        restart();
        assert(!settings.touch.game_menu_quick_actions);
        for (bool enabled : {true, false}) {
            // Preferences Apply updates the runtime field, then saves it.
            settings.touch.game_menu_quick_actions = enabled;
            assert(std::get<bool>(settingsGetValue(*it)) == enabled);
            assert(settingsSave());
            assert(disk.at({"touch", "game_menu_quick_actions"}) == (enabled ? "1" : "0"));
            restart();
            assert(settings.touch.game_menu_quick_actions == enabled);
        }
    }
    disk.clear();
    restart();
    assert(settingsSave());
    assert(disk.count({"touch", "game_menu_quick_actions"}));
    assert(!disk.count({"touch", "hud_quick_actions"}));
}

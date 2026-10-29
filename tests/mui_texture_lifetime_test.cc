// Real cache and screen-stack functions; SDL texture storage is stubbed so
// upload counts, pixels, allocation failures and ownership are deterministic.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include "mui.h"

namespace {
struct Texture {
    int width, height;
    Uint32 format;
    std::vector<unsigned char> pixels;
};
std::map<SDL_Texture*, Texture*> textures;
int creates = 0, destroys = 0, uploads = 0;
bool failCreate = false, failUpload = false;
SDL_Texture* createTexture(SDL_Renderer*, Uint32 format, int, int width, int height)
{
    if (failCreate) return nullptr;
    auto* texture = new Texture { width, height, format, {} };
    auto* handle = reinterpret_cast<SDL_Texture*>(texture);
    textures[handle] = texture;
    creates++;
    return handle;
}
void destroyTexture(SDL_Texture* texture)
{
    assert(textures.count(texture) == 1);
    delete textures.at(texture);
    textures.erase(texture);
    destroys++;
}
int updateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch)
{
    // SDL's null rectangle updates the whole allocation, independently of
    // the cache's last successful upload dimensions. Preserve real row reads
    // here so ASan detects undersized source buffers after a failed resize.
    assert(rect == nullptr);
    uploads++;
    Texture& target = *textures.at(texture);
    if (failUpload) return -1;
    target.pixels.resize(target.width * target.height * 4);
    for (int row = 0; row < target.height; row++) {
        memcpy(target.pixels.data() + row * target.width * 4,
            static_cast<const unsigned char*>(pixels) + row * pitch, target.width * 4);
    }
    return 0;
}
int blend(SDL_Texture*, SDL_BlendMode) { return 0; }
int scale(SDL_Texture*, SDL_ScaleMode) { return 0; }
int drawBlend(SDL_Renderer*, SDL_BlendMode) { return 0; }
unsigned char palette[768] {};
}
#define SDL_CreateTexture createTexture
#define SDL_DestroyTexture destroyTexture
#define SDL_UpdateTexture updateTexture
#define SDL_SetTextureBlendMode blend
#define SDL_SetTextureScaleMode scale
#define SDL_SetRenderDrawBlendMode drawBlend
#include "../src/mui_draw.cc"
#undef SDL_CreateTexture
#undef SDL_DestroyTexture
#undef SDL_UpdateTexture
#undef SDL_SetTextureBlendMode
#undef SDL_SetTextureScaleMode
#undef SDL_SetRenderDrawBlendMode

namespace fallout {
unsigned char* directDrawGetPalette() { return palette; }
void muiIconsReset() {}
}
using namespace fallout;

namespace {
SDL_Renderer* renderer = reinterpret_cast<SDL_Renderer*>(1);
// Deliberately starts at the same version in every new object, like the
// production save/load, preferences, worldmap, scene and dialogue screens.
struct Screen : MuiScreen {
    unsigned int version = 1;
    explicit Screen() { muiPush(this); }
    ~Screen() override { muiRemove(this); }
    void build(MuiContext&) override {}
};
SDL_Texture* rgba(const char* key, unsigned char pixel, unsigned int version)
{
    unsigned char data[] = { pixel, 2, 3, 255 };
    return muiRgbaTexture(key, data, 1, 1, version);
}
SDL_Texture* indexed(const char* key, unsigned char pixel, unsigned int version)
{
    return muiIndexedTexture(key, &pixel, 1, 1, version);
}
unsigned char red(SDL_Texture* texture)
{
    Texture& stored = *textures.at(texture);
    if (stored.format == SDL_PIXELFORMAT_ABGR8888) return stored.pixels[0];
    Uint32 pixel;
    memcpy(&pixel, stored.pixels.data(), sizeof(pixel));
    return (pixel >> 16) & 255;
}
void begin() { muiDrawBegin(renderer); }

void uploadFailures(bool useIndexed)
{
    const char* key = useIndexed ? "failure.indexed" : "failure.rgba";
    const std::string cacheKey = useIndexed ? key : std::string("rgba.") + key;
    unsigned int version = 0;
    auto request = [&](int side) {
        // Keep the 1x1 RGBA input on the stack, as in the original overflow.
        // The indexed helper creates its own exactly sized RGBA heap buffer.
        unsigned char small[4] = { 1, 2, 3, 255 };
        unsigned char large[16] = { 1, 2, 3, 255, 1, 2, 3, 255,
            1, 2, 3, 255, 1, 2, 3, 255 };
        const unsigned char* pixels = side == 1 ? small : large;
        return useIndexed ? muiIndexedTexture(key, pixels, side, side, version)
                          : muiRgbaTexture(key, pixels, side, side, version);
    };
    auto valid = [&](int side) {
        auto& entry = gIndexedTextures.at(cacheKey);
        assert(entry.valid && entry.base.texture);
        assert(entry.base.width == side && entry.base.height == side);
        const Texture& allocation = *textures.at(entry.base.texture);
        assert(allocation.width == side && allocation.height == side);
        assert(allocation.pixels.size() == static_cast<size_t>(side * side * 4));
    };
    auto discarded = [&]() {
        auto& entry = gIndexedTextures.at(cacheKey);
        assert(!entry.valid && !entry.base.texture);
        assert(entry.base.width == 0 && entry.base.height == 0);
    };
    // Growing, shrinking and unchanged-size failed uploads. Each failure
    // must discard its allocation; retry the earlier size and earlier version.
    for (auto sizes : { std::pair<int, int> { 1, 2 }, { 2, 1 }, { 1, 1 }, { 2, 2 } }) {
        Screen screen;
        ++version;
        assert(request(sizes.first));
        valid(sizes.first);
        const int allocated = creates, freed = destroys, before = uploads;
        const size_t live = textures.size();
        ++version;
        failUpload = true;
        assert(!request(sizes.second));
        failUpload = false;
        discarded();
        const bool resized = sizes.first != sizes.second;
        assert(creates == allocated + (resized ? 1 : 0));
        assert(destroys == freed + (resized ? 2 : 1));
        assert(textures.size() == live - 1 && uploads == before + 1);
        --version;
        assert(request(sizes.first));
        valid(sizes.first);
        assert(textures.size() == live && uploads == before + 2);
        int reused = uploads, allocations = creates;
        assert(request(sizes.first));
        assert(uploads == reused && creates == allocations);
    }
    // Creation failure after discarding an existing differently sized
    // allocation must leave no texture eligible for an update or cache hit.
    for (int side : { 1, 2 }) {
        Screen screen;
        ++version;
        assert(request(side));
        const int allocated = creates, freed = destroys, before = uploads;
        failCreate = true;
        assert(!request(3 - side));
        failCreate = false;
        discarded();
        assert(creates == allocated && destroys == freed + 1 && uploads == before);
        assert(request(3 - side));
        valid(3 - side);
        assert(creates == allocated + 1 && uploads == before + 1);
    }
}
}

int main()
{
    palette[3] = 10;
    palette[6] = 20;
    begin();
    uploadFailures(false);
    uploadFailures(true);
    // Fixed production keys: reopening must upload even when version and
    // object address repeat. Existing allocations should still be reused.
    for (const char* key : { "loadsave.thumbnail.10", "loadsave.thumbnail.11",
             "loadsave.picture", "prefs.preview", "worldmap.fog", "selector.custom" }) {
        SDL_Texture* original;
        {
            Screen screen;
            original = rgba(key, 10, screen.version);
            assert(original && red(original) == 10);
            int before = uploads;
            assert(rgba(key, 10, screen.version) == original && uploads == before);
            assert(rgba(key, 20, ++screen.version) == original && uploads == before + 1);
            assert(red(original) == 20);
        }
        int allocated = creates, freed = destroys, before = uploads;
        {
            Screen screen;
            assert(rgba(key, 30, screen.version) == original && red(original) == 30);
            assert(uploads == before + 1 && creates == allocated && destroys == freed);
            before = uploads;
            assert(rgba(key, 30, screen.version) == original && uploads == before);
        }
        {
            Screen screen;
            int before = uploads;
            assert(rgba(key, 40, screen.version) == original && red(original) == 40);
            assert(uploads == before + 1);
        }
    }
    for (const char* key : { "scene.frame", "talk.head" }) {
        SDL_Texture* original;
        {
            Screen screen;
            original = indexed(key, 1, screen.version);
            assert(original && red(original) == 40);
            int before = uploads;
            assert(indexed(key, 1, screen.version) == original && uploads == before);
            assert(indexed(key, 2, ++screen.version) == original && red(original) == 80);
        }
        {
            Screen reopened;
            assert(indexed(key, 2, reopened.version) == original && red(original) == 80);
        }
        Screen reopened;
        assert(indexed(key, 1, reopened.version) == original && red(original) == 40);
        auto* preview = rgba("prefs.preview", 30, 1);
        int before = uploads;
        palette[3] = 11;
        begin();
        assert(indexed(key, 1, reopened.version) == original && red(original) == 44);
        assert(uploads == before + 1);
        assert(rgba("prefs.preview", 30, 1) == preview && uploads == before + 1);
        palette[3] = 10;
        begin();
    }
    // Pushing a child or re-pushing an existing screen invalidates pixels
    // without destroying textures that another drawing path may still use.
    {
        Screen parent;
        auto* texture = rgba("prefs.preview", 42, parent.version);
        int allocated = creates, freed = destroys;
        { Screen child; assert(textures.count(texture) == 1); }
        assert(creates == allocated && destroys == freed);
        assert(rgba("prefs.preview", 43, parent.version) == texture && red(texture) == 43);
        muiPush(&parent);
        assert(rgba("prefs.preview", 44, parent.version) == texture && red(texture) == 44);
    }
    // Failure cannot establish a valid cache hit: retry the same revision.
    {
        Screen screen;
        failCreate = true;
        assert(!rgba("new", 1, 1));
        assert(!indexed("new", 1, 1));
        failCreate = false;
        assert(rgba("new", 1, 1) && indexed("new", 1, 1));
        muiPush(&screen);
        failUpload = true;
        assert(!rgba("new", 2, 1) && !indexed("new", 2, 1));
        failUpload = false;
        int before = uploads;
        assert(red(rgba("new", 2, 1)) == 2 && red(indexed("new", 2, 1)) == 80);
        assert(uploads == before + 2);
    }
    const size_t cacheSize = gIndexedTextures.size();
    const int allocated = creates;
    for (int iteration = 0; iteration < 1000; iteration++) {
        Screen screen;
        assert(red(rgba("worldmap.fog", iteration % 256, screen.version)) == iteration % 256);
    }
    assert(gIndexedTextures.size() == cacheSize && creates == allocated);
    auto generation = muiDrawGeneration();
    muiDrawReset();
    assert(gIndexedTextures.empty() && textures.empty() && creates == destroys);
    assert(muiDrawGeneration() != generation);
    assert(!rgba("worldmap.fog", 7, 1)); // No renderer until the next frame.
    assert(!indexed("scene.frame", 1, 1));
    begin();
    {
        Screen screen;
        assert(red(rgba("worldmap.fog", 7, 1)) == 7);
        assert(red(indexed("scene.frame", 1, 1)) == 40);
    }
    muiDrawReset();
    assert(textures.empty() && creates == destroys);
    puts("Texture lifetime, cache reuse, palette, failures and reset checks passed.");
}

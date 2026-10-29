#include "death_subtitle.h"
#include "mui_font_data.h"
#include "save_preview_png.h"
#include "db.h"
#include <lodepng.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

// Observe decoder allocation entry without replacing decoding. IHDR inspection
// allocates nothing; reaching full decoding allocates the IDAT accumulation buffer.
static size_t pngAllocations = 0;
void* lodepng_malloc(size_t size) { ++pngAllocations; return std::malloc(size); }
void* lodepng_realloc(void* ptr, size_t size) { ++pngAllocations; return std::realloc(ptr, size); }
void lodepng_free(void* ptr) { std::free(ptr); }

// Stub only VFS I/O. All boundary decisions and PNG/font parsing are production code.
namespace fallout {
static std::vector<unsigned char> input;
static size_t position;
static int reportedSize;
static bool missing, readFailure, closeFailure;
static File stream;
File* fileOpen(const char*, const char*) { position = 0; return missing ? nullptr : &stream; }
int fileClose(File*) { return closeFailure ? -1 : 0; }
int fileGetSize(File*) { return reportedSize; }
int fileReadChar(File*) { return position == input.size() ? -1 : input[position++]; }
int fileEof(File*) { return !readFailure && position == input.size(); }
size_t fileRead(void* data, size_t size, size_t count, File*)
{
    size_t bytes = std::min(size * count, input.size());
    if (readFailure && bytes != 0) --bytes;
    std::memcpy(data, input.data(), bytes);
    return bytes / size;
}
}
using namespace fallout;
static void reset(const std::string& text = "")
{
    input.assign(text.begin(), text.end());
    reportedSize = static_cast<int>(input.size());
    missing = readFailure = closeFailure = false;
}
static void subtitles()
{
    struct Guarded { unsigned char before[16]; char text[512]; unsigned char after[16]; } data;
    for (size_t size : { 0, 20, 511, 512, 4096 }) {
        reset(std::string(size, 'x'));
        std::memset(&data, 0xA5, sizeof(data));
        assert(deathSubtitleRead("subtitle", data.text, sizeof(data.text)) == (size <= 511 ? 0 : -1));
        assert(std::strlen(data.text) == (size <= 511 ? size : 0));
        for (auto c : data.before) assert(c == 0xA5);
        for (auto c : data.after) assert(c == 0xA5);
    }
    reset("1:Hello\n2:World\n");
    assert(deathSubtitleRead("subtitle", data.text, sizeof(data.text)) == 0);
    assert(std::string(data.text) == "1:Hello 2:World ");
    // Timing removal remains at the existing callers, after a successful read.
    for (int failure = 0; failure < 3; ++failure) {
        reset("partially read");
        missing = failure == 0; readFailure = failure == 1; closeFailure = failure == 2;
        assert(deathSubtitleRead("subtitle", data.text, sizeof(data.text)) == -1);
        assert(data.text[0] == 0);
    }
    reset();
    assert(deathSubtitleRead("subtitle", nullptr, 512) == -1);
    data.text[0] = 'z';
    assert(deathSubtitleRead("subtitle", data.text, 0) == -1 && data.text[0] == 'z');
    assert(deathSubtitleRead("subtitle", data.text, 1) == 0 && data.text[0] == 0);
    reset("x");
    assert(deathSubtitleRead("subtitle", data.text, 1) == -1 && data.text[0] == 0);
}
static void fonts()
{
    std::vector<unsigned char> loaded;
    const char* names[] = { "FalloutClassicDialog.ttf", "PTM55FT.ttf" };
    for (int index = 0; index < 2; ++index) {
        reset();
        std::ifstream file(std::string(FONT_ASSET_DIR) + "/" + names[index], std::ios::binary);
        assert(file.good());
        const std::vector<unsigned char> original((std::istreambuf_iterator<char>(file)), {});
        input = original; reportedSize = static_cast<int>(input.size());
        assert(muiReadTrustedFont(index, loaded) && loaded == original);
        int offset = stbtt_GetFontOffsetForIndex(loaded.data(), 0);
        assert(offset >= 0 && static_cast<size_t>(offset) < loaded.size());
        stbtt_fontinfo info {};
        assert(stbtt_InitFont(&info, loaded.data(), offset));
        // Exercise real atlas parsing/rasterization for the production character ranges.
        std::vector<unsigned char> pixels(2048 * 2048);
        stbtt_pack_context context {};
        assert(stbtt_PackBegin(&context, pixels.data(), 2048, 2048, 0, 1, nullptr));
        stbtt_PackSetOversampling(&context, 2, 1);
        for (auto range : { std::pair<int,int>{0x20, 95}, {0xA0, 224}, {0x400, 96},
                {0x490, 2}, {0x2010, 24}, {0x2030, 11}, {0x20AC, 1}, {0x2116, 1}, {0x2122, 1}, {0x2212, 1} }) {
            std::vector<stbtt_packedchar> chars(range.second);
            assert(stbtt_PackFontRange(&context, loaded.data(), 0, 24, range.first, range.second, chars.data()));
        }
        stbtt_PackEnd(&context);
        // Exact identity rejects modifications to headers, directories, offsets and glyph data alike.
        for (size_t pos = 0; pos < original.size(); pos += 257) {
            input = original; input[pos] ^= 1;
            assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        }
        for (size_t pos = 0; pos < 64; ++pos) {
            input = original; input[pos] ^= 0xff;
            assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        }
        input = original; input.back() ^= 1;
        assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        input = original;
        for (int size : { INT_MIN, -1, 0, 1, 11, 12, static_cast<int>(original.size()) - 1,
                static_cast<int>(original.size()) + 1, INT_MAX }) {
            reportedSize = size;
            assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        }
        reportedSize = static_cast<int>(original.size());
        input.resize(12); // correct advertised size, truncated actual content
        assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        input = original;
        for (int failure = 0; failure < 3; ++failure) {
            missing = failure == 0; readFailure = failure == 1; closeFailure = failure == 2;
            assert(!muiReadTrustedFont(index, loaded) && loaded.empty());
        }
    }
    assert(!muiReadTrustedFont(-1, loaded));
    assert(!muiReadTrustedFont(2, loaded));
}
static std::vector<unsigned char> png(unsigned width, unsigned height, bool interlace = false, bool sixteenBit = false)
{
    lodepng::State state;
    state.encoder.auto_convert = 0;
    state.info_png.interlace_method = interlace;
    state.info_png.color.bitdepth = state.info_raw.bitdepth = sixteenBit ? 16 : 8;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * (sixteenBit ? 8 : 4), 77), encoded;
    assert(lodepng::encode(encoded, pixels, width, height, state) == 0);
    return encoded;
}
static void setDimensions(std::vector<unsigned char>& encoded, unsigned width, unsigned height)
{
    for (int i = 0; i < 4; ++i) {
        encoded[16 + i] = width >> (24 - 8*i);
        encoded[20 + i] = height >> (24 - 8*i);
    }
    lodepng_chunk_generate_crc(encoded.data() + 8);
}
static void previews()
{
    size_t bytes;
    assert(savePreviewPngDimensions(1600, 1600, &bytes) && bytes == 10240000);
    for (auto size : { std::pair<unsigned,unsigned>{0,1}, {1,0}, {1601,1}, {1,1601}, {UINT_MAX,UINT_MAX}, {0x80000000,2} })
        assert(!savePreviewPngDimensions(size.first, size.second, &bytes));
    std::vector<unsigned char> output = { 42 };
    unsigned w = 17, h = 19;
    auto reject = [&](const std::vector<unsigned char>& data) {
        auto before = output;
        assert(!savePreviewPngDecode(data, output, w, h));
        assert(output == before && w == 17 && h == 19);
    };
    for (auto size : { std::pair<unsigned,unsigned>{224,133}, {800,450}, {1599,1}, {1,1599}, {1600,1600} }) {
        auto encoded = png(size.first, size.second);
        assert(savePreviewPngDecode(encoded, output, w, h));
        assert(w == size.first && h == size.second && output.size() == static_cast<size_t>(w)*h*4);
        assert(std::all_of(output.begin(), output.end(), [](unsigned char c) { return c == 77; }));
    }
    for (bool sixteen : {false, true}) {
        auto encoded = png(71, 35, true, sixteen);
        assert(savePreviewPngDecode(encoded, output, w, h) && w == 71 && h == 35);
    }
    w = 17; h = 19;
    auto small = png(1,1);
    // IHDR alone: inside bounds proceeds to decoder (missing IDAT); outside
    // bounds must stop at preflight. A decoder spy is tested separately below.
    for (auto size : { std::pair<unsigned,unsigned>{1601,1}, {1,1601}, {UINT_MAX,UINT_MAX}, {0,1} }) {
        auto encoded = small; setDimensions(encoded, size.first, size.second);
        pngAllocations = 0;
        reject(encoded);
        assert(pngAllocations == 0); // no full decoder allocation before rejection
    }
    auto oversized = png(1601,1601);
    assert(oversized.size() < 100000);
    pngAllocations = 0; reject(oversized); assert(pngAllocations == 0);
    // A tiny accepted IHDR with huge decompressed IDAT exercises the fixed zlib bound.
    setDimensions(oversized, 1, 1);
    pngAllocations = 0; reject(oversized); assert(pngAllocations != 0);
    // Ancillary compressed metadata is independently bounded; text is unused.
    for (size_t size : {size_t(65536), size_t(65537)}) {
        std::vector<unsigned char> profile(size, 'x');
        uLongf compressedSize = compressBound(profile.size());
        std::vector<unsigned char> compressed(compressedSize);
        assert(compress2(compressed.data(), &compressedSize, profile.data(), profile.size(), 9) == Z_OK);
        compressed.resize(compressedSize);
        std::vector<unsigned char> chunkData = {'t', 0, 0};
        chunkData.insert(chunkData.end(), compressed.begin(), compressed.end());
        unsigned char* chunk = nullptr;
        size_t chunkSize = 0;
        assert(lodepng_chunk_create(&chunk, &chunkSize, chunkData.size(), "iCCP", chunkData.data()) == 0);
        auto encoded = small;
        encoded.insert(encoded.begin()+33, chunk, chunk+chunkSize);
        std::free(chunk);
        if (size > 65536) reject(encoded);
        else {
            assert(savePreviewPngDecode(encoded, output, w, h));
            w = 17; h = 19;
        }
    }
    // Exercise the encoder's ordinary auto-selected grayscale/palette color mode too.
    std::vector<unsigned char> automatic, solid(32*32*4, 255);
    assert(lodepng::encode(automatic, solid, 32, 32) == 0);
    assert(savePreviewPngDecode(automatic, output, w, h) && output == solid);
    w = 17; h = 19;
    reject({}); reject({1,2,3}); reject(std::vector<unsigned char>(small.begin(), small.begin()+32));
    auto bad = small; bad[29] ^= 1; reject(bad); // bad IHDR CRC
    bad = small; bad.resize(bad.size()-15); reject(bad);
    reject(std::vector<unsigned char>(8*1024*1024+1));
}
int main()
{
    subtitles(); fonts(); previews();
    std::puts("asset hardening: subtitles, trusted fonts and PNG boundaries passed");
}

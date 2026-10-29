#include "save_preview_png.h"
#include <cstdlib>
#include <limits>
#include <lodepng.h>
#include <zlib.h>
#include "loadsave.h"
namespace fallout {
namespace {
// LodePNG's built-in max_output_size is a soft limit. Use the existing zlib
// dependency with a fixed destination so malformed IDAT/ICC cannot grow it.
unsigned boundedZlib(unsigned char** out, size_t* outsize, const unsigned char* in,
    size_t insize, const LodePNGDecompressSettings* settings)
{
    const size_t limit = settings->max_output_size != 0
        ? settings->max_output_size
        : *static_cast<const size_t*>(settings->custom_context);
    *out = static_cast<unsigned char*>(std::malloc(limit));
    *outsize = 0;
    if (*out == nullptr) return 83;
    uLongf size = static_cast<uLongf>(limit);
    int error = uncompress(*out, &size, in, static_cast<uLong>(insize));
    if (error != Z_OK) return 110;
    *outsize = size;
    return 0;
}
}
bool savePreviewPngDimensions(unsigned width, unsigned height, size_t* rgbaBytes)
{
    constexpr unsigned maxDimension = kMobileWidePreviewMaxSize * 2;
    if (width == 0 || height == 0 || width > maxDimension || height > maxDimension
        || static_cast<size_t>(width) > std::numeric_limits<size_t>::max() / height / 4) return false;
    *rgbaBytes = static_cast<size_t>(width) * height * 4;
    return true;
}
bool savePreviewPngDecode(const std::vector<unsigned char>& encoded,
    std::vector<unsigned char>& rgba, unsigned& width, unsigned& height)
{
    if (encoded.empty() || encoded.size() > 8 * 1024 * 1024) return false;
    lodepng::State state;
    unsigned w = 0, h = 0;
    size_t rgbaBytes = 0;
    // inspect checks the signature, IHDR fields and CRC without decoding pixels.
    if (lodepng_inspect(&w, &h, &state, encoded.data(), encoded.size()) != 0
        || !savePreviewPngDimensions(w, h, &rgbaBytes)) return false;

    // Upper bound for filtered scanlines at PNG's maximum 64 bits/pixel,
    // including Adam7 rounding/filter bytes. Dimensions are already <= 1600.
    const size_t scanlineLimit = static_cast<size_t>(w) * h * 8 + (w + h) * 16 + 128;
    state.decoder.zlibsettings.custom_zlib = boundedZlib;
    state.decoder.zlibsettings.custom_context = &scanlineLimit;
    state.decoder.read_text_chunks = 0;
    state.decoder.remember_unknown_chunks = 0;
    state.decoder.max_icc_size = 64 * 1024;
    std::vector<unsigned char> decoded;
    if (lodepng::decode(decoded, w, h, state, encoded) != 0 || decoded.size() != rgbaBytes) return false;
    rgba.swap(decoded);
    width = w;
    height = h;
    return true;
}
}

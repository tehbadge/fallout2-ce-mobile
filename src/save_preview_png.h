#ifndef FALLOUT_SAVE_PREVIEW_PNG_H
#define FALLOUT_SAVE_PREVIEW_PNG_H
#include <cstddef>
#include <vector>
namespace fallout {
bool savePreviewPngDimensions(unsigned width, unsigned height, size_t* rgbaBytes);
bool savePreviewPngDecode(const std::vector<unsigned char>& encoded,
    std::vector<unsigned char>& rgba, unsigned& width, unsigned& height);
}
#endif

#include "mui_font_data.h"
#include <cstring>
#include "db.h"
#include "trusted_mui_fonts.h"
namespace fallout {
bool muiReadTrustedFont(int index, std::vector<unsigned char>& data)
{
    data.clear();
    const char* paths[] = { "fonts\\FalloutClassicDialog.ttf", "fonts\\PTM55FT.ttf" };
    const unsigned char* trusted[] = { kFalloutClassicDialog, kPTM55FT };
    const size_t sizes[] = { sizeof(kFalloutClassicDialog), sizeof(kPTM55FT) };
    if (index < 0 || index > 1) return false;
    File* stream = fileOpen(paths[index], "rb");
    if (stream == nullptr) return false;
    int size = fileGetSize(stream);
    // Check signed size before converting or allocating; exact sizes also cap allocation.
    bool ok = size > 0 && static_cast<size_t>(size) == sizes[index];
    if (ok) {
        data.resize(sizes[index]);
        ok = fileRead(data.data(), 1, data.size(), stream) == data.size()
            && std::memcmp(data.data(), trusted[index], data.size()) == 0;
    }
    if (fileClose(stream) != 0) ok = false;
    if (!ok) data.clear();
    return ok;
}
}

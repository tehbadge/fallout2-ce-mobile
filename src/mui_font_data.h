#ifndef FALLOUT_MUI_FONT_DATA_H
#define FALLOUT_MUI_FONT_DATA_H
#include <vector>
namespace fallout {
// Only the exact shipped Classic (0) and Modern (1) assets may reach stb.
bool muiReadTrustedFont(int index, std::vector<unsigned char>& data);
}
#endif

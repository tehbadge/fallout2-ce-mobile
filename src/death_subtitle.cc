#include "death_subtitle.h"
#include "db.h"
namespace fallout {
int deathSubtitleRead(const char* path, char* dest, size_t capacity)
{
    if (dest == nullptr || capacity == 0) return -1;
    dest[0] = '\0';
    File* stream = fileOpen(path, "rt");
    if (stream == nullptr) return -1;
    size_t length = 0;
    int result = 0;
    while (true) {
        int c = fileReadChar(stream);
        if (c == -1) {
            if (!fileEof(stream)) result = -1;
            break;
        }
        if (length == capacity - 1) {
            result = -1;
            break;
        }
        dest[length++] = c == '\n' ? ' ' : (c & 0xFF);
    }
    if (fileClose(stream) != 0) result = -1;
    dest[result == 0 ? length : 0] = '\0';
    return result;
}
}

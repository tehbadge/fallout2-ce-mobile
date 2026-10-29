#ifndef FALLOUT_DEATH_SUBTITLE_H
#define FALLOUT_DEATH_SUBTITLE_H
#include <cstddef>
namespace fallout {
// Reject oversized or failed reads, leaving an empty destination. Empty files succeed.
int deathSubtitleRead(const char* path, char* dest, size_t capacity);
}
#endif

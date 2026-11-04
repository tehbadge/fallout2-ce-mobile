"""Execute the production map-movement gate across engine/script wait states."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/touch_controls.cc').read_text()
start = source.index('static bool touchControlsViewMayMove()\n{')
end = source.index('\n}', start) + 2
fixture = r'''
#include <cassert>
static bool isoDisabled, scriptDisabled, scrolling;
static int cursor;
constexpr int MOUSE_CURSOR_WAIT_WATCH = 7;
bool isoIsDisabled() { return isoDisabled; }
bool gameUiIsDisabledByScript() { return scriptDisabled; }
bool gameMouseIsMapScrollingEnabled() { return scrolling; }
int gameMouseGetCursor() { return cursor; }
PRODUCTION
int main() {
    for (bool iso : {false, true}) for (bool script : {false, true})
    for (bool scroll : {false, true}) for (bool watch : {false, true}) {
        isoDisabled = iso; scriptDisabled = script; scrolling = scroll;
        cursor = watch ? MOUSE_CURSOR_WAIT_WATCH : 0;
        assert(touchControlsViewMayMove() == (!iso && !script && (scroll || watch)));
    }
}
'''.replace('PRODUCTION', source[start:end])
with tempfile.TemporaryDirectory() as temporary:
    src = Path(temporary) / 'policy.cc'
    src.write_text('#include <initializer_list>\n' + fixture)
    binary = Path(temporary) / 'policy'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-UNDEBUG', str(src), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: 16 engine-wait, script-lock, scrolling and disabled-map combinations')

// Exercise the production pointer queue, slider and scroll arbitration together.
// Drawing is stubbed; unused engine entry points are removed by the linker.
#define SDL_MAIN_HANDLED
#include "../src/mui.cc"
#include <cassert>
#include <cstdio>

namespace fallout {
float muiTextWidth(const std::u32string& text, float size) { return text.size() * size * .6f; }
void muiDrawTextAligned(const std::u32string&, const MuiRect&, float, MuiColor, MuiAlign, MuiAlign) {}
void muiFillRoundRect(const MuiRect&, float, MuiColor) {}
void muiStrokeRoundRect(const MuiRect&, float, float, MuiColor) {}
void muiFillCircle(float, float, float, MuiColor) {}
void muiStrokeCircle(float, float, float, float, MuiColor) {}
unsigned int touchLongPressMs() { return 500; }
void touchLogUiStaleDrop(int) {}
}
using namespace fallout;

struct Fixture {
    MuiContext ui;
    MuiRect area, track;
    float value = 25, offset = 0;
    bool dragging = false;
    bool scrollable = true;
    float maximum = 100;
    Fixture(float scale, bool right) {
        gPointer = {}; gEvents.clear(); gActiveId.clear(); gScrolls.clear();
        gSliderGesture = SliderGesture::Pending;
        ui.isInteractive = true; ui.pixelsPerDp = scale; gPixelsPerDp = scale;
        float x = right ? 40 : 20;
        area = { x * scale, 0, 400 * scale, 300 * scale };
        track = { (x + 100) * scale, 100 * scale, 200 * scale, 30 * scale };
    }
    void frame() {
        processEvents();
        ui.now += 16;
        offset = ui.scroll("prefs.rows", area, scrollable ? 1000 * ui.pixelsPerDp : area.h);
        value = ui.slider("prefs.rows.slider", track, value, 0, maximum, &dragging);
        if (gPointer.released) gActiveId.clear();
    }
    void event(PointerEventType type, float x, float y) {
        gEvents.push_back({type, x, y, 100});
        frame();
    }
    void down(float ratio = .75f) { event(PointerEventType::Down, track.x + track.w * ratio, track.centerY()); }
    void move(float dx, float dy) { event(PointerEventType::Move, gPointer.startX + ui.dp(dx), gPointer.startY + ui.dp(dy)); }
    void up() { event(PointerEventType::Up, gPointer.x, gPointer.y); }
};
int main() {
    for (float density : {1.f, 2.f, 3.f}) for (float size : {1.f, 1.25f, 1.5f, 2.f}) for (bool right : {false, true}) {
        float scale = density * size;
        {
            Fixture f(scale, right);
            f.down();
            assert(f.value == 25 && !f.dragging);
            f.up();
            assert(f.value == 75);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(4, 3);
            assert(f.value == 25);
            f.up();
            assert(f.value > 75);
        }
        {
            Fixture f(scale, right);
            f.down(.25);
            f.move(12, 2);
            assert(f.value > 25 && f.dragging);
            f.move(16, 60);
            assert(f.offset == 0 && f.dragging);
            f.up();
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(0, -12);
            assert(f.value == 25);
            f.move(2, -60);
            assert(f.offset > 0 && f.value == 25 && !f.dragging);
            f.up();
            assert(f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.event(PointerEventType::Down, f.area.x+2, f.track.centerY());
            f.move(0, -20);
            f.move(0, -70);
            f.up();
            assert(f.offset > 0 && f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(12, -18);
            f.move(50, -70);
            f.up();
            assert(f.value == 25 && f.offset > 0);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(18, -12);
            assert(f.value > 75);
            f.up();
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(15, 15);
            f.move(60, 20);
            f.up();
            assert(f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.down();
            gActiveId.clear();
            f.up();
            assert(f.value == 25);
            f.down();
            f.up();
            assert(f.value == 75);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(0, 8);
            f.move(0, -8);
            assert(f.value == 25);
            f.up();
            assert(f.value == 75);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(0, -20);
            f.move(0, 0);
            f.up();
            assert(f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.down();
            f.move(200, 0);
            assert(f.value == 100);
            f.move(-400, 0);
            assert(f.value == 0);
            f.up();
        }
        {
            Fixture f(scale, right);
            for (int i = 0; i < 20; i++) {
                f.down(.5);
                assert(!f.dragging);
                f.up();
                assert(f.value == 50);
            }
        }
        {
            Fixture f(scale, right);
            f.down();
            f.event(PointerEventType::Up, gPointer.x, f.track.y-100*scale);
            assert(f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.scrollable = false;
            f.down();
            f.move(0, -20);
            f.move(50, 0);
            f.up();
            assert(f.value == 25 && f.offset == 0);
        }
        {
            Fixture f(scale, right);
            f.scrollable = false;
            f.down(.25);
            f.move(30, 0);
            assert(std::fabs(f.value - 40) < .0001f);
            f.up();
        }
        {
            Fixture f(scale, right);
            f.down();
            f.ui.isInteractive = false;
            f.up();
            assert(f.value == 25);
        }
        {
            Fixture f(scale, right);
            f.maximum = 0;
            f.down();
            f.up();
            assert(f.value == 25 && !f.dragging);
        }
        {
            Fixture f(scale, right);
            gEvents.push_back({PointerEventType::Down, f.track.x + f.track.w * .75f, f.track.centerY(), 100});
            gEvents.push_back({PointerEventType::Up, f.track.x + f.track.w * .75f, f.track.centerY(), 100});
            f.frame();
            assert(f.value == 25);
            f.frame();
            assert(f.value == 75);
        }
    }
    // The runtime CRT segmented control is a child of the same scroll area.
    for (float scale : {1.f, 2.5f, 4.5f}) {
        Fixture f(scale, false);
        f.track.w = f.ui.dp(180);
        int selected = 0;
        std::vector<std::u32string> labels {U"Off", U"Tonytron", U"Arcade", U"Consumer TV"};
        auto frame = [&] {
            processEvents(); f.ui.now += 16;
            f.offset = f.ui.scroll("prefs.rows", f.area, f.ui.dp(1000));
            f.ui.segmented("prefs.rows.crt", f.track, labels, &selected);
            if (gPointer.released) gActiveId.clear();
        };
        auto event = [&](PointerEventType type, float x, float y) {
            gEvents.push_back({type,x,y,100}); frame();
        };
        frame();
        auto cell = gFrameWidgets.at("prefs.rows.crt.3");
#ifdef __IPHONEOS__
        for (int i=0;i<4;i++) {
            const auto& bounds = gFrameWidgets.at("prefs.rows.crt."+std::to_string(i));
            assert(bounds.w > 0 && bounds.x >= f.track.x && bounds.right() <= f.track.right()+.01f);
        }
#endif
        event(PointerEventType::Down,cell.centerX(),cell.centerY());
        assert(selected==0);
        event(PointerEventType::Move,cell.centerX(),cell.centerY()-f.ui.dp(40));
        event(PointerEventType::Move,cell.centerX(),cell.centerY()-f.ui.dp(70));
        event(PointerEventType::Up,cell.centerX(),cell.centerY()-f.ui.dp(70));
        assert(selected==0 && f.offset>0);
        event(PointerEventType::Down,cell.centerX(),cell.centerY());
        event(PointerEventType::Up,cell.centerX(),cell.centerY());
        assert(selected==3);
    }
    puts("PASS: 456 gesture cases across densities 1/2/3, scales 100/125/150/200%, mirrored landscape origins; CRT taps, scroll arbitration and iOS segmented bounds");
}

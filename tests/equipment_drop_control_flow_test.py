"""Exercise production equipment-drop control flow with mocked inventory actions.

The compiled harness embeds the current production drop, preview and consumption
functions, rectangle type and slot enums. It does not reimplement hit testing.
Simulator regressions separately cover real equipment rules and touch dispatch.
Run from the repository root: python3 tests/equipment_drop_control_flow_test.py
"""

from pathlib import Path
import subprocess
import argparse

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output-dir', type=Path, default=ROOT / 'out/equipment-test')
args = parser.parse_args()
OUTPUT = args.output_dir


def source_block(path, signature):
    """Read one balanced declaration/function from the production source."""
    source = (ROOT / path).read_text()
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    return source[start:end]


PREAMBLE = r"""
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
"""

MOCKS = r"""
struct DragState {
    bool active = false;
    int payload = 0;
    std::string source;
} gDrag;

struct PointerState {
    bool released = false;
    float x = 0;
    float y = 0;
} gPointer;

struct MuiContext {
    bool isInteractive = true;
    float scale = 1;
    float dp(float value) const { return value * scale; }
    float pointerX() const { return gPointer.x; }
    float pointerY() const { return gPointer.y; }
    bool dropped(const MuiRect&, int*);
};

struct Object {};
Object dude, armor, weapon, otherItem, container;
Object* gDude = &dude;

enum class InventoryActionType { Drop, UseOnSelf };
struct InventoryAction {
    InventoryActionType type;
    Object* item;
    InventorySlot slot;
};
struct InventoryView {
    Object* container = nullptr;
    Object* critter = gDude;
};

constexpr int kNoAction = -1;
constexpr int kGround = 100;
constexpr int kUse = 101;
std::vector<int> actions;
bool canDropOnto = false;
bool canUseOnSelf = false;

void inventoryQueueAction(const InventoryAction& action)
{
    actions.push_back(action.type == InventoryActionType::Drop ? kGround : kUse);
}
bool inventoryCanDropOnto(Object*, Object*) { return canDropOnto; }
bool inventoryCanUseOnSelf(Object*) { return canUseOnSelf; }

struct InventoryScreen {
    struct Cell {
        MuiRect rect;
        Object* item;
    };
    std::vector<Cell> cells;
    MuiRect infoRect, figureRect, crumbRect, listRect, slotRects[3];
    Object* dragging = &armor;
    InventorySlot origin = InventorySlot::None;

    Object* draggedItem(MuiContext&, const InventoryView&, InventorySlot* slot)
    {
        *slot = origin;
        return dragging;
    }
    void move(Object*, InventorySlot, InventoryDropTarget target, Object*)
    {
        actions.push_back(static_cast<int>(target));
    }
    void handleDrop(MuiContext&, const InventoryView&);
};
"""

CASES = r"""
const InventoryDropTarget targets[] = {
    InventoryDropTarget::LeftHand,
    InventoryDropTarget::RightHand,
    InventoryDropTarget::Armor,
};

struct Fixture {
    MuiContext ui;
    InventoryScreen screen;
    InventoryView view;
    int slotIndex;

    Fixture(float scale, int index) : slotIndex(index)
    {
        ui.scale = scale;
        const MuiRect nowhere { -10000, -10000, 1, 1 };
        screen.infoRect = screen.figureRect = screen.crumbRect = screen.listRect = nowhere;
        for (int i = 0; i < 3; ++i) {
            screen.slotRects[i] = { i * 200 * scale, 0, 100 * scale, 100 * scale };
        }
        screen.dragging = index == 2 ? &armor : &weapon;
        // Preview center is 81.6 dp into a 100 dp slot; finger is 20 dp below it.
        gPointer = { true, (index * 200 + 50) * scale, 120 * scale };
        gDrag = { true, 0, "source" };
        actions.clear();
        canDropOnto = false;
        canUseOnSelf = false;
    }

    MuiRect fingerArea() const
    {
        return { slotIndex * 200 * ui.scale, 110 * ui.scale,
            100 * ui.scale, 30 * ui.scale };
    }
};

int checks = 0;
void verify(Fixture& fixture, const char* name, int expected, bool consumed = true)
{
    fixture.screen.handleDrop(fixture.ui, fixture.view);
    const size_t count = expected == kNoAction ? 0 : 1;
    const bool correct = actions.size() == count
        && (count == 0 || actions.front() == expected)
        && gDrag.active == !consumed;
    if (!correct) {
        std::fprintf(stderr, "FAIL: %s, slot=%d, scale=%g, actions=%zu, active=%d\n",
            name, fixture.slotIndex, fixture.ui.scale, actions.size(), gDrag.active);
        std::abort();
    }
    ++checks;
}

void checkSlot(float scale, int index)
{
    const int destination = static_cast<int>(targets[index]);
    {
        Fixture f(scale, index);
        gPointer.y = 50 * scale;
        verify(f, "finger aligned", destination);
    }
    {
        Fixture f(scale, index);
        verify(f, "preview aligned, finger outside", destination);
    }
    {
        Fixture f(scale, index);
        gPointer.y = 200 * scale;
        verify(f, "both anchors outside", kNoAction, false);
    }
    {
        Fixture f(scale, index);
        f.screen.origin = static_cast<InventorySlot>(index + 1);
        verify(f, "preview on source slot cancels", kNoAction);
    }
    for (int fingerIndex = 0; fingerIndex < 3; ++fingerIndex) {
        if (fingerIndex == index) continue;
        {
            Fixture f(scale, index);
            f.screen.slotRects[fingerIndex] = f.fingerArea();
            verify(f, "finger equipment beats different preview equipment",
                static_cast<int>(targets[fingerIndex]));
        }
        {
            Fixture f(scale, index);
            f.screen.origin = static_cast<InventorySlot>(fingerIndex + 1);
            f.screen.slotRects[fingerIndex] = f.fingerArea();
            verify(f, "finger on source cancels different preview target", kNoAction);
        }
    }
    {
        Fixture f(scale, index);
        f.screen.cells.push_back({ f.fingerArea(), &otherItem });
        verify(f, "rejected finger item cell blocks preview", kNoAction);
    }
    {
        Fixture f(scale, index);
        f.screen.cells.push_back({ f.fingerArea(), &container });
        canDropOnto = true;
        verify(f, "accepted finger item cell beats preview",
            static_cast<int>(InventoryDropTarget::Backpack));
    }
    {
        Fixture f(scale, index);
        f.screen.origin = InventorySlot::LeftHand;
        f.screen.listRect = f.fingerArea();
        verify(f, "backpack removal beats preview",
            static_cast<int>(InventoryDropTarget::Backpack));
    }
    {
        Fixture f(scale, index);
        f.screen.infoRect = f.fingerArea();
        verify(f, "ground drop beats preview", kGround);
    }
    {
        Fixture f(scale, index);
        f.view.container = &container;
        f.screen.crumbRect = f.fingerArea();
        verify(f, "container exit beats preview",
            static_cast<int>(InventoryDropTarget::Body));
    }
    {
        Fixture f(scale, index);
        f.screen.figureRect = f.fingerArea();
        canUseOnSelf = true;
        verify(f, "use on self beats preview", kUse);
    }
    {
        Fixture f(scale, index);
        f.ui.isInteractive = false;
        verify(f, "inactive input cannot dispatch", kNoAction, false);
    }
    {
        Fixture f(scale, index);
        gPointer.released = false;
        verify(f, "held drag cannot dispatch", kNoAction, false);
    }
    {
        Fixture f(scale, index);
        f.screen.dragging = nullptr;
        verify(f, "missing dragged item", kNoAction, false);
    }
    // Type rejection belongs to the unchanged inventory engine, not the UI.
    if (index == 2) {
        Fixture f(scale, index);
        f.screen.dragging = &weapon;
        verify(f, "nonarmor preview dispatches through normal eligibility",
            static_cast<int>(InventoryDropTarget::Armor));
    }
}

int main()
{
    for (float scale : { 1.f, 2.f, 3.3f, 4.5f }) {
        for (int index = 0; index < 3; ++index) {
            checkSlot(scale, index);
        }
    }
    std::printf("PASS: %d production-function checks; inventory actions mocked\n", checks);
}
"""


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    declarations = [
        source_block("src/mui_draw.h", "struct MuiRect {") + ";",
        source_block("src/inventory.h", "enum class InventorySlot {") + ";",
        source_block("src/inventory.h", "enum class InventoryDropTarget {") + ";",
    ]
    functions = [
        source_block("src/mui.cc", "bool MuiContext::dropped("),
        source_block("src/mui_items.cc", "MuiRect muiDraggedItemRect("),
        source_block("src/mui_inventory.cc", "void InventoryScreen::handleDrop("),
    ]
    source = OUTPUT / "equipment-control-flow.cc"
    binary = OUTPUT / "equipment-control-flow"
    source.write_text("\n".join([PREAMBLE, *declarations, MOCKS, *functions, CASES]))
    subprocess.run([
        args.cxx, "-std=c++17", "-UNDEBUG", "-Wall", "-Wextra", "-Werror",
        str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()

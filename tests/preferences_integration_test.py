"""Exercise production Settings rows and CRT dispatch without a renderer.

Extract the row factory and relevant screen methods, as the equipment harness
extracts drop control flow. Only drawing, input widgets and runtime CRT storage
are mocked. Run through CTest or pass --sdl-include for each SDL header directory.
"""
from pathlib import Path
import re
import json
import subprocess
import argparse
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output-dir', type=Path, default=ROOT / 'out/preferences-test')
parser.add_argument('--sdl-include', action='append', required=True)
args = parser.parse_args()
OUTPUT = args.output_dir
SOURCE = (ROOT / 'src/mui_preferences.cc').read_text()


def function(signature, source=SOURCE):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


# Keep production constants, Values, Row, and makeRows verbatim. Other engine
# helpers in this prefix are discarded by the linker, never called by the test.
prefix = SOURCE[:SOURCE.index('    // Settings: sections at the left')]
fixture = r'''
class PreferencesScreen {
public:
    std::vector<Row> rows = makeRows();
    Values applied {}, edited {};
    bool tall = false, resetScroll = false, previewValid = false;
    Section section = Section::Display;
    std::vector<unsigned char> preview;
    int previewWidth = 0, previewHeight = 0;
    unsigned int previewVersion = 0;
    int heldSlider = -1;
    bool optionsLoaded = false;
    MessageList optionsMessages {};
    std::u32string optionLabel(const Row&, int);
    void previewChanges(const Values&) {}
    bool changed(const Row&) const;
    bool anyChanged() const;
    float rowHeight(MuiContext&, const Row&, float) const;
    float lineHeight(MuiContext&, const Row&) const;
    float noteSize(MuiContext&) const;
    void buildRow(MuiContext&, size_t, const MuiRect&);
    void buildRows(MuiContext&, const MuiRect&);
};
'''
methods = '\n'.join(function(signature) for signature in [
    'std::u32string PreferencesScreen::optionLabel(', 'bool PreferencesScreen::changed(', 'bool PreferencesScreen::anyChanged(',
    'float PreferencesScreen::rowHeight(', 'float PreferencesScreen::lineHeight(',
    'float PreferencesScreen::noteSize(', 'void PreferencesScreen::buildRow(',
    'void PreferencesScreen::buildRows(',
])
mocks = r'''
} // anonymous namespace
PRODUCTION_TABS
static std::vector<MuiRect> tabRects;
bool muiTabButton(MuiContext&, const std::string& id, const MuiRect& rect, MuiIcon, bool, bool) {
    tabRects.push_back(rect); return id == "test.map";
}
bool muiInventoryCost(std::u32string*, bool*) { return false; }
void muiDrawCostBadge(MuiContext&, const MuiRect&, const std::u32string&, bool) {}
void muiDrawUnavailableOverlay(MuiContext&, const MuiRect&, float) {}
PipboyUnavailable pipboyUnavailableReason() { return PipboyUnavailable::None; }
Settings settings;
static IosCrtModel model = IosCrtModel::Off;
static int choose = -1, switches = 0;
static std::vector<std::pair<MuiRect,bool>> controls;
static bool crtInteractive = false;
static float offset = 0;
IosCrtModel iosCrtGetModel() { return model; }
void iosCrtSetModel(IosCrtModel selected) { model = selected; switches++; }
bool hudIsTabletScreen(float scale) { return scale <= 1.25f; }
static MuiTheme theme;
const MuiTheme& muiTheme() { return theme; }
bool messageListGetItem(MessageList*, MessageListItem*) { return false; }
RESOURCE_LOOKUP
const char* muiText(int id, const char* fallback) {
    auto it = resourceText.find(id);
    return it == resourceText.end() ? fallback : it->second;
}
std::u32string muiDecodeGameText(const char* text) { return std::u32string(text, text + strlen(text)); }
float muiTextWidth(const std::u32string& text, float size) { return text.size() * size * .6f; }
float muiFitTextSize(const std::u32string&, float, float size, float) { return size; }
float muiLineHeight(float size) { return size; }
std::vector<std::u32string> muiWrapText(const std::u32string& text, float width, float size) {
    return std::vector<std::u32string>(std::max(1, int(std::ceil(muiTextWidth(text,size)/width))), U"note");
}
void muiFillRect(const MuiRect&, MuiColor) {}
void muiFillCircle(float, float, float, MuiColor) {}
void muiStrokeRoundRect(const MuiRect&, float, float, MuiColor) {}
void muiDrawTextAligned(const std::u32string&, const MuiRect&, float, MuiColor, MuiAlign, MuiAlign) {}
void muiPushClip(const MuiRect&) {}
void muiPopClip() {}
SDL_Texture* muiRgbaTexture(const std::string&, const unsigned char*, int, int, unsigned int) { return nullptr; }
MuiRect muiFitRect(const MuiRect& rect, float, float) { return rect; }
void muiDrawTexture(SDL_Texture*, const MuiRect&, MuiColor) {}
void _gsound_red_butt_press(int, int) {}
void preferencesPlaySoundEffectSample() {}
void preferencesPlaySpeechSample() {}
float MuiContext::pointerX() const { return 0; }
float MuiContext::pointerY() const { return 0; }
void MuiContext::setScroll(const std::string&, float value) { offset = value; }
float MuiContext::scroll(const std::string&, const MuiRect&, float, bool) { return offset; }
bool MuiContext::toggle(const std::string&, const MuiRect& rect, bool*) { controls.push_back({rect,interactive()}); return false; }
float MuiContext::slider(const std::string&, const MuiRect& rect, float value, float, float, bool* dragging) {
    controls.push_back({rect,interactive()}); *dragging = false; return value;
}
bool MuiContext::segmented(const std::string& id, const MuiRect& rect, const std::vector<std::u32string>& options, int* selected) {
    controls.push_back({rect,interactive()});
    if (id != "prefs.rows.crt") return false;
    assert((options == std::vector<std::u32string>{U"Off",U"Tonytron",U"Arcade",U"Consumer TV"}));
    crtInteractive = interactive();
    if (!interactive() || choose < 0) return false;
    *selected = choose; return true;
}
} // fallout
using namespace fallout;
int main() {
    assert(model == IosCrtModel::Off);
    PreferencesScreen screen;
    for (const auto& row : screen.rows) if (row.textId == kTextFont) {
        assert(screen.optionLabel(row,0)==U"Classic");
        assert(screen.optionLabel(row,1)==U"Modern");
    }
    size_t crt = 0, smoothing = 0, interface = 0, quick = 0;
    int crts = 0, filters = 0;
    for (size_t i=0; i<screen.rows.size(); i++) {
        const auto& row = screen.rows[i];
        if (row.kind == RowKind::Crt) { crt=i; crts++; }
        if (row.textId == kTextMapFilter) { smoothing=i; filters++; }
        if (row.textId == kTextGroupInterface) interface=i;
        if (row.textId == kTextGameMenuQuickActions) quick=i;
    }
    // Source-only heading IDs must still resolve to their intended fallback.
    for (const auto& row : screen.rows) {
        if (row.textId == kTextGroupText || row.textId == kTextGroupSaves || row.textId == kTextGroupSpeed)
            assert(strcmp(muiText(row.textId, row.fallback), row.fallback)==0);
    }
    assert(crts==1 && filters==1 && crt==smoothing+1 && interface==crt+1 && quick>interface);
    assert(!screen.rows[crt].get && !screen.rows[crt].set);
    assert(screen.rows[quick].kind==RowKind::Toggle);
    assert(screen.rows[quick].get(screen.edited)==0);
    screen.rows[quick].set(screen.edited,1);
    assert(screen.anyChanged());
    screen.edited=screen.applied;
    MuiContext ui; ui.isInteractive=true;
    for (bool tall : {false,true}) for (float scale : {1.f,1.25f,1.5f}) {
        ui.pixelsPerDp=scale; screen.tall=tall;
        for (int choice=0;choice<4;choice++) {
            choose=choice;
            screen.buildRow(ui,crt,{0,0,ui.dp(700),screen.rowHeight(ui,screen.rows[crt],ui.dp(700))});
            assert(int(model)==choice && !screen.anyChanged());
        }
        // Reset/reopening persisted values must leave runtime selection alone.
        screen.edited=screen.applied;
        PreferencesScreen reopened;
        assert(model==IosCrtModel::ConsumerTv && !reopened.anyChanged());
        float y=0;
        for(size_t i=0;i<crt;i++) if(screen.rows[i].section==Section::Display)
            y+=screen.rowHeight(ui,screen.rows[i],ui.dp(700));
        const float inset=ui.dp(tall?12:8);
        const float line=screen.lineHeight(ui,screen.rows[crt]);
        const float viewportHeight=ui.dp(90);
        for (int position : {0,1,2}) {
            // Fully visible; top clipped; bottom clipped.
            offset=position==0 ? y : position==1 ? y+inset+1 : y+line-inset-viewportHeight-1;
            int before=switches; choose=1; crtInteractive=false;
            screen.buildRows(ui,{0,0,ui.dp(700),viewportHeight});
            assert(crtInteractive==(position==0));
            assert(switches==before+(position==0));
        }
    }
    // Clipped choices, toggles and sliders must not steal footer/header taps.
    choose=-1;
    for (bool tall : {false,true}) for (float scale : {1.f,1.25f,1.5f}) {
        ui.pixelsPerDp=scale; screen.tall=tall;
        const MuiRect viewport {0,0,ui.dp(700),ui.dp(90)};
        for(float scroll=0;scroll<ui.dp(1800);scroll+=ui.dp(13)) {
            offset=scroll; controls.clear(); screen.buildRows(ui,viewport);
            for(const auto& control : controls) if(control.second) {
                assert(control.first.y>=viewport.y);
                assert(control.first.bottom()<=viewport.bottom());
            }
        }
    }
    for (float scale : {1.f,1.25f,1.5f}) {
        MuiContext tabsUi; tabsUi.pixelsPerDp=3*scale;
        MuiRect column {2500, tabsUi.dp(10), tabsUi.dp(52), 1260-tabsUi.dp(20)};
        tabRects.clear();
        assert(muiGameScreenTabs(tabsUi,column,"test",MuiGameScreenTab::Inventory,true)==MuiGameScreenTab::Map);
        assert(tabRects.size()==5);
        for (const auto& rect : tabRects) {
            assert(rect.x>=column.x && rect.right()<=column.right()+.01f);
            assert(rect.y>=column.y && rect.bottom()<=column.bottom()+.01f);
            assert(rect.w==rect.h && rect.w>=tabsUi.dp(44));
        }
        if (scale==1) {
            assert(tabRects[0].w==column.w);
            assert(tabRects[1].y==column.y+tabsUi.dp(60));
        }
        tabRects.clear();
        muiGameScreenTabs(tabsUi,column,"test",MuiGameScreenTab::None,false);
        assert(tabRects.size()==1 && tabRects[0].w==column.w);
    }
    puts("PASS: tab-column containment and routing; Settings order, unique controls, persistent quick actions, independent CRT selection and control clipping");
}
'''
# Resource presence is not the complete namespace: upstream reserves IDs in
# source even when translations are absent and muiText uses fallback labels.
english = (ROOT / 'files/ce.dat/text/english/game/ce.msg').read_bytes()
entries = re.findall(rb'^\{(\d+)\}\{[^}]*\}\{([^}]*)\}', english, re.M)
ids = [int(number) for number, _ in entries]
assert len(ids) == len(set(ids))
resource_text = {int(number): text.decode('latin1') for number, text in entries}
assert resource_text.get(375) == 'Game screens'
owners = [
    ('src/mui_preferences.cc', 'kTextGameMenuQuickActions', 'Quick Save/Load in Game Menu'),
    ('src/mui_options.cc', 'kTextQuickSave', 'Quick Save'),
    ('src/mui_options.cc', 'kTextQuickLoad', 'Quick Load'),
]
custom_ids = []
for owner, symbol, text in owners:
    definition = re.search(r'constexpr int ' + symbol + r' = (\d+);', (ROOT / owner).read_text())
    assert definition, symbol
    number = int(definition[1])
    assert resource_text.get(number) == text, (symbol, number)
    # Conservatively reject any other literal use in production source,
    # including source-only reservations, direct lookups, and enum values.
    for path in (ROOT / 'src').rglob('*'):
        if path.suffix not in ('.cc', '.h', '.mm'):
            continue
        source = path.read_bytes().decode('latin1')
        if path == ROOT / owner:
            source = source.replace(definition[0], '', 1)
        assert not re.search(r'\b' + str(number) + r'\b', source), (symbol, number, path)
    custom_ids.append(number)
assert len(set(custom_ids)) == len(custom_ids)
# Check duplicate entries in every bundled translation.
for path in (ROOT / 'files/ce.dat/text').rglob('ce.msg'):
    localized_ids = re.findall(rb'^\{(\d+)\}', path.read_bytes(), re.M)
    assert len(localized_ids) == len(set(localized_ids)), path
lookup_ids = set(custom_ids) | {395, 396, 397}
lookup = 'static const std::map<int, const char*> resourceText {' + ','.join(
    '{' + str(number) + ',' + json.dumps(text) + '}'
    for number, text in resource_text.items() if number in lookup_ids
) + '};'
mocks = mocks.replace('RESOURCE_LOOKUP', lookup)
mocks = mocks.replace('PRODUCTION_TABS', function('MuiGameScreenTab muiGameScreenTabs(', (ROOT / 'src/mui_game_dialog.cc').read_text()))
OUTPUT.mkdir(parents=True,exist_ok=True)
source=OUTPUT/'preferences_test.cc'
source.write_text('#define SDL_MAIN_HANDLED\n#include <cassert>\n#include <cstring>\n#include <map>\n#include "pipboy.h"\n'+prefix+fixture+methods+mocks)
binary=OUTPUT/'preferences_test'
subprocess.run([args.cxx,'-std=c++17','-UNDEBUG','-D__IPHONEOS__=1','-DFALLOUT_FORCE_TOUCH_ONLY=1',
    '-ffunction-sections','-fdata-sections',
    '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections',
    '-I'+str(ROOT/'src'), *['-I'+directory for directory in args.sdl_include],
    str(source),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True)

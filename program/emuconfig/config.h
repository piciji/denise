
#pragma once

struct Message;
struct InputMapping;
struct FileSetting;
struct FirmwareManager;

#include "../../guikit/api.h"
#include "../program.h"

namespace EmuConfigView {

struct TabWindow;
	
#include "../config/slider.h"
#include "layouts/system.h"
#include "layouts/swapper.h"
#include "layouts/states.h"
#include "layouts/border.h"
#include "layouts/video.h"
#include "layouts/input.h"
#include "layouts/firmware.h"
#include "layouts/palette.h"

struct TabWindow : public GUIKIT::Window {
    
    enum Layout : unsigned { System, Control, States, Presentation, Palette, Firmware, Border, Swapper };
    
    Emulator::Interface* emulator;
    
    Message* message;
    InputLayout* inputLayout = nullptr;
    SystemLayout* systemLayout = nullptr;
    FirmwareLayout* firmwareLayout = nullptr;
    BorderLayout* borderLayout = nullptr;
    VideoLayout* videoLayout = nullptr;
    PaletteLayout* paletteLayout = nullptr;
    StatesLayout* statesLayout = nullptr;
    SwapperLayout* swapperLayout = nullptr;    

    GUIKIT::TabFrameLayout tab;
    
    GUIKIT::Image joystickImage;
    GUIKIT::Image systemImage;
    GUIKIT::Image memoryImage;
    GUIKIT::Image cropImage;
    GUIKIT::Image displayImage;
    GUIKIT::Image scriptImage;    
    GUIKIT::Image swapperImage;
    GUIKIT::Image paletteImage;
	
	GUIKIT::Timer mtimer;

    auto build() -> void;	
    auto translate() -> void;
    auto show(Layout layout) -> void;
	auto showDelayed(Layout layout) -> void;
    auto update() -> void;
    auto ident( std::string name ) -> std::string;
	static auto getView( Emulator::Interface* emulator ) -> TabWindow*;

    TabWindow(Emulator::Interface* emulator);
};

}

extern std::vector<EmuConfigView::TabWindow*> emuConfigViews;

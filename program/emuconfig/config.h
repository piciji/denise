
#pragma once

struct Message;
struct InputMapping;
struct FileSetting;
struct FirmwareManager;

#include "../../guikit/api.h"
#include "../program.h"
#include "layouts/model.h"

namespace MediaView {
    struct MediaLayout;
}

namespace EmuConfigView {

struct TabWindow;
	
#include "../config/slider.h"
#include "layouts/system.h"
#include "layouts/configurations.h"
#include "layouts/border.h"
#include "layouts/video.h"
#include "layouts/input.h"
#include "layouts/firmware.h"
#include "layouts/palette.h"
#include "layouts/misc.h"
#include "layouts/audio.h"

struct TabWindow : public GUIKIT::Window {
    
    enum Layout : unsigned { System, Media, Configurations, Control, Presentation, Palette, Audio, Firmware, Border, Misc };
    
    Emulator::Interface* emulator;
    
    Message* message;    
    SystemLayout* systemLayout = nullptr;
    MediaView::MediaLayout* mediaLayout = nullptr;
    ConfigurationsLayout* configurationsLayout = nullptr;
    InputLayout* inputLayout = nullptr;
    AudioLayout* audioLayout = nullptr;
    FirmwareLayout* firmwareLayout = nullptr;
    BorderLayout* borderLayout = nullptr;
    VideoLayout* videoLayout = nullptr;
    PaletteLayout* paletteLayout = nullptr;
    MiscLayout* miscLayout = nullptr;
    GUIKIT::Settings* settings = nullptr;    

    GUIKIT::TabFrameLayout tab;
    
    GUIKIT::Image joystickImage;
    GUIKIT::Image systemImage;
    GUIKIT::Image driveImage;
    GUIKIT::Image scriptImage;
    GUIKIT::Image memoryImage;
    GUIKIT::Image cropImage;
    GUIKIT::Image displayImage;    
    GUIKIT::Image paletteImage;
    GUIKIT::Image volumeImage;
    GUIKIT::Image miscImage;
	
	GUIKIT::Timer mtimer;

    auto build() -> void;	
    auto translate() -> void;
    auto show(Layout layout) -> void;
	auto showDelayed(Layout layout) -> void;
    auto setLayout(Layout layout) -> void;
    auto prepareLayout(Layout layout) -> void;
	static auto getView( Emulator::Interface* emulator, bool createIfNotExist = false ) -> TabWindow*;

    TabWindow(Emulator::Interface* emulator);
};

}

extern std::vector<EmuConfigView::TabWindow*> emuConfigViews;

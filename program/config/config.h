
#pragma once

struct Message;
struct InputMapping;

#include "../../guikit/api.h"

namespace ConfigView {

#include "layouts/driver.h"
#include "layouts/audio.h"
#include "layouts/video.h"
#include "layouts/input.h"
#include "layouts/settings.h"

struct TabWindow : public GUIKIT::Window {
    
    enum Layout : unsigned { Audio, Video, Input, Settings };
    
    Message* message;
    AudioLayout* audioLayout = nullptr;
    VideoLayout* videoLayout = nullptr;
    InputLayout* inputLayout = nullptr;    
    SettingsLayout* settingsLayout = nullptr;    

    GUIKIT::TabFrameLayout tab;
    
    GUIKIT::Image volumeImage;
    GUIKIT::Image displayImage;
    GUIKIT::Image keyboardImage;
    GUIKIT::Image toolsImage;    
	
	GUIKIT::Timer mtimer;

    auto build() -> void;	
    auto translate() -> void;
    auto show(Layout layout) -> void;
	auto showDelayed(Layout layout) -> void;

    TabWindow();
};

}

extern ConfigView::TabWindow* configView;

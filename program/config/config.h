
#pragma once

struct Message;
struct InputMapping;

#include "../../guikit/api.h"

namespace ConfigView {

#include "slider.h"
#include "layouts/driver.h"
#include "layouts/audio.h"
#include "layouts/video.h"
#include "layouts/input.h"
#include "layouts/settings.h"

struct TabWindow : public GUIKIT::Window {
    
    enum Layout : unsigned { Video, Audio, Input, Settings };
    
    Message* message;
	VideoLayout* videoLayout = nullptr;
    AudioLayout* audioLayout = nullptr;    
    InputLayout* inputLayout = nullptr;    
    SettingsLayout* settingsLayout = nullptr;    

    GUIKIT::TabFrameLayout tab;
    
	GUIKIT::Image displayImage;
    GUIKIT::Image volumeImage;    
    GUIKIT::Image keyboardImage;
    GUIKIT::Image toolsImage;

    auto build() -> void;	
    auto translate() -> void;
    auto show(Layout layout) -> void;
    static auto open(Layout layout) -> void;

    TabWindow();
};

}

extern ConfigView::TabWindow* configView;

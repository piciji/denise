
#pragma once

struct Message;
struct InputMapping;

namespace Emulator {
    struct Interface;
}

#include "../../guikit/api.h"
#include "slider.h"

namespace ConfigView {

#include "layouts/drivers.h"
#include "layouts/settings.h"

struct TabWindow : public GUIKIT::Window {
    
    enum Layout : unsigned { Drivers, Settings };
    
    Message* message;
	DriversLayout* driversLayout = nullptr;
    SettingsLayout* settingsLayout = nullptr;    

    GUIKIT::TabFrameLayout tab;
    
	GUIKIT::Image gearsImage;
    GUIKIT::Image toolsImage;

    auto build() -> void;	
    auto translate() -> void;
    auto show(Layout layout) -> void;
    static auto open(Layout layout) -> void;

    TabWindow();
};

}

extern ConfigView::TabWindow* configView;

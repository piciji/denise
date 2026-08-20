
#include "config.h"
#include "archiveViewer.h"
#include "../view/view.h"

#include "../emuconfig/config.h"
#include "../program.h"
#include "../view/message.h"
#include "../input/manager.h"
#include "../audio/manager.h"
#include "../../data/icons.h"
#include "../thread/emuThread.h"
#include "../helper/miscHelper.h"

#include "layouts/drivers.h"
#include "layouts/settings.h"

ConfigView::TabWindow* configView = nullptr;

namespace ConfigView {

TabWindow::TabWindow() {
    message = new Message(this);
}

auto TabWindow::build() -> void {
    MiscHelper::applyGeometry(this, globalSettings, "screen_settings",
        {100, 100, 700u, 400u});

    gearsImage.loadPng((uint8_t*)Icons::gears, sizeof(Icons::gears));
    toolsImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));

    append(tab);
    
    settingsLayout = new SettingsLayout;
    driversLayout = new DriversLayout;

	tab.appendHeader("", gearsImage);
    tab.appendHeader("", toolsImage);

	tab.setLayout(Layout::Drivers, *driversLayout, {~0u, ~0u} );
    tab.setLayout(Layout::Settings, *settingsLayout, {~0u, ~0u} );

    tab.setMargin(10);
    tab.setSelection(0);        
    
    tab.onChange = [this]() {

    };

    onClose = [this]() {
        emuThread->lock();
        setVisible(false);
        emuThread->unlock();
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<int>("screen_settings_x", geometry.x);
        globalSettings->set<int>("screen_settings_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<unsigned>("screen_settings_width", geometry.width);
        globalSettings->set<unsigned>("screen_settings_height", geometry.height);
    };

    translate();
}

auto TabWindow::translate() -> void {
    setTitle( trans->get("app settings", {{"%app%", APP_NAME}} ) );

    settingsLayout->translate();
    driversLayout->translate();
    
    tab.setHeader(Layout::Drivers, trans->get("driver"));
	tab.setHeader(Layout::Settings, trans->get( "generic" ));
}

auto TabWindow::show(Layout layout) -> void {	
    tab.setSelection( (unsigned)layout );	
    setVisible();
	setFocused();
}

auto TabWindow::open(Layout layout) -> void {
    if (!configView) {
        configView = new ConfigView::TabWindow;
        configView->build();
    }
    configView->show( layout );
}

}

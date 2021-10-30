
#include "config.h"
#include "archiveViewer.h"
#include "../view/view.h"
#include "../view/status.h"
#include "../emuconfig/config.h"
#include "../program.h"
#include "../view/message.h"
#include "../input/manager.h"
#include "../audio/manager.h"
#include "../../data/icons.h"
#include "../../data/logos.h"

ConfigView::TabWindow* configView = nullptr;

namespace ConfigView {

#include "layouts/settings.cpp"
#include "layouts/input.cpp"
#include "layouts/audio.cpp"
#include "layouts/video.cpp"

TabWindow::TabWindow() {
    message = new Message(this);
}

auto TabWindow::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();

    GUIKIT::Geometry defaultGeometry = {100, 100, GUIKIT::Font::scale(700), GUIKIT::Font::scale(490)};
    
    GUIKIT::Geometry geometry = {globalSettings->get<int>("screen_settings_x", defaultGeometry.x)
        ,globalSettings->get<int>("screen_settings_y", defaultGeometry.y)
        ,globalSettings->get<unsigned>("screen_settings_width", defaultGeometry.width)
        ,globalSettings->get<unsigned>("screen_settings_height", defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry ); 
    
    volumeImage.loadPng((uint8_t*)Icons::volume, sizeof(Icons::volume));
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));    
    keyboardImage.loadPng((uint8_t*)Icons::keyboard, sizeof(Icons::keyboard));
    toolsImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));

    append(tab);
    
    settingsLayout = new SettingsLayout;
    audioLayout = new AudioLayout;
    videoLayout = new VideoLayout;
    inputLayout = new InputLayout;

	tab.appendHeader("", displayImage);
    tab.appendHeader("", volumeImage);    
    tab.appendHeader("", keyboardImage);
    tab.appendHeader("", toolsImage);                                

	tab.setLayout(Layout::Video, *videoLayout, {~0u, ~0u} );
    tab.setLayout(Layout::Audio, *audioLayout, {~0u, ~0u} );    
    tab.setLayout(Layout::Input, *inputLayout, {~0u, ~0u} );
    tab.setLayout(Layout::Settings, *settingsLayout, {~0u, ~0u} );                               

    tab.setMargin(10);
    tab.setSelection(0);        
    
    tab.onChange = [this]() {
        settingsLayout->removePreview();
    };

    onClose = [this]() {
        setVisible(false);
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<int>("screen_settings_x", geometry.x);
        globalSettings->set<int>("screen_settings_y", geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<unsigned>("screen_settings_width", geometry.width);
        globalSettings->set<unsigned>("screen_settings_height", geometry.height);
    };

    translate();
}

auto TabWindow::translate() -> void {
    setTitle( trans->get("app settings", {{"%app%", APP_NAME}} ) );

	inputLayout->translate();
    settingsLayout->translate();
    videoLayout->translate();
    audioLayout->translate();    
    
    tab.setHeader(Layout::Video, trans->get("video"));
    tab.setHeader(Layout::Audio, trans->get("audio"));
    tab.setHeader(Layout::Input, trans->get("input"));
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


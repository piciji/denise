
#include "config.h"
#include "archiveViewer.h"
#include "../view/view.h"
#include "../emuconfig/config.h"
#include "../program.h"
#include "../view/message.h"
#include "../input/manager.h"
#include "../audio/manager.h"

ConfigView::TabWindow* configView = nullptr;

namespace ConfigView {
	
namespace Icons {
    #include "../../data/img/icons.data"
}

#include "layouts/settings.cpp"
#include "layouts/input.cpp"
#include "layouts/audio.cpp"
#include "layouts/video.cpp"

TabWindow::TabWindow() {
    message = new Message(this);
}

auto TabWindow::build() -> void {
    winapi.disableBackgroundRedrawDuringResize();
    cocoa.keepMenuVisibilityOnDisplay();

    GUIKIT::Geometry defaultGeometry = {100, 100, 650, 420};
    
    GUIKIT::Geometry geometry = {settings->get<int>("screen_settings_x", defaultGeometry.x)
        ,settings->get<int>("screen_settings_y", defaultGeometry.y)
        ,settings->get<unsigned>("screen_settings_width", defaultGeometry.width)
        ,settings->get<unsigned>("screen_settings_height", defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry ); 
    
    volumeImage.loadPng((uint8_t*)Icons::volume, sizeof(Icons::volume));
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));    
    keyboardImage.loadPng((uint8_t*)Icons::keyboard, sizeof(Icons::keyboard));
    toolsImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));

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
        
    append(tab);
    
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
        settings->set<int>("screen_settings_x", geometry.x);
        settings->set<int>("screen_settings_y", geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>("screen_settings_width", geometry.width);
        settings->set<unsigned>("screen_settings_height", geometry.height);
    };

    translate();
}

auto TabWindow::translate() -> void {
    setTitle( APP_NAME " " + trans->get("settings") );

	inputLayout->translate();
    settingsLayout->translate();
    videoLayout->translate();
    audioLayout->translate();    
    
    tab.setHeader(Layout::Video, trans->get("video"));
    tab.setHeader(Layout::Audio, trans->get("audio"));
    tab.setHeader(Layout::Input, trans->get("input"));
	tab.setHeader(Layout::Settings, trans->get( "generic" ));
}

auto TabWindow::showDelayed(Layout layout) -> void {
	inputDriver->mUnacquire();
	mtimer.setInterval(100);
	
	mtimer.onFinished = [this, layout]() {
		mtimer.setEnabled(false);
		show(layout);
	};
	mtimer.setEnabled();
}

auto TabWindow::show(Layout layout) -> void {	
    tab.setSelection( (unsigned)layout );	
    setVisible();
	setFocused();
}

}

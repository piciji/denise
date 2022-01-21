
#include "config.h"
#include "../config/archiveViewer.h"
#include "../config/config.h"
#include "../view/view.h"
#include "../input/manager.h"
#include "../view/message.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../view/status.h"
#include "../firmware/manager.h"
#include "../video/palette.h"
#include "../cmd/cmd.h"
#include "../media/media.h"
#include "../audio/manager.h"
#include "../../data/icons.h"
#include "../thread/emuThread.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

std::vector<EmuConfigView::TabWindow*> emuConfigViews;

namespace EmuConfigView {

#define mes this->tabWindow->message
#define _settings this->tabWindow->settings
	
#include "layouts/input.cpp"
#include "layouts/system.cpp"
#include "layouts/configurations.cpp"
#include "layouts/video.cpp"
#include "layouts/audio.cpp"
#include "layouts/border.cpp"
#include "layouts/firmware.cpp"
#include "layouts/palette.cpp"
#include "layouts/misc.cpp"

TabWindow::TabWindow(Emulator::Interface* emulator) {
    this->emulator = emulator;
    this->settings = program->getSettings( emulator );
    message = new Message(this);
}

auto TabWindow::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();
    setDroppable();
	    
    GUIKIT::Geometry defaultGeometry = {100, 100, GUIKIT::Font::scale(900), GUIKIT::Font::scale(560)};
    
    GUIKIT::Geometry geometry = {settings->get<int>("screen_settings_x", defaultGeometry.x)
        ,settings->get<int>("screen_settings_y", defaultGeometry.y)
        ,settings->get<unsigned>("screen_settings_width", defaultGeometry.width)
        ,settings->get<unsigned>("screen_settings_height", defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry ); 

    joystickImage.loadPng((uint8_t*)Icons::joystick, sizeof(Icons::joystick));
    systemImage.loadPng((uint8_t*)Icons::system, sizeof(Icons::system));
    driveImage.loadPng((uint8_t*)Icons::drive, sizeof(Icons::drive));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    memoryImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    cropImage.loadPng((uint8_t*)Icons::crop, sizeof(Icons::crop));
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));    
    paletteImage.loadPng((uint8_t*)Icons::palette, sizeof(Icons::palette));
    volumeImage.loadPng((uint8_t*)Icons::volume, sizeof(Icons::volume));
    miscImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));
    
    tab.setMargin(10);
    append(tab);        
    
    tab.appendHeader("", systemImage);
    tab.appendHeader("", driveImage);
    tab.appendHeader("", scriptImage);
	tab.appendHeader("", joystickImage); 
	tab.appendHeader("", displayImage);
	if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        tab.appendHeader("", paletteImage);        
    }
    tab.appendHeader("", volumeImage);
    tab.appendHeader("", memoryImage);   
	tab.appendHeader("", cropImage);
    tab.appendHeader("", miscImage);
	
	if (tellMeShouldICreateTheUIRightAway()) {
		inputLayout = new InputLayout( this );
		systemLayout = new SystemLayout( this );
		mediaLayout = new MediaView::MediaLayout( this );
		configurationsLayout = new ConfigurationsLayout( this );
		firmwareLayout = new FirmwareLayout( this );
		videoLayout = new VideoLayout( this );
		if (dynamic_cast<LIBC64::Interface*>(emulator))
			paletteLayout = new PaletteLayout( this );

		audioLayout = new AudioLayout( this );
		borderLayout = new BorderLayout( this );
		miscLayout = new MiscLayout( this );
		mediaLayout->build();
		
		tab.setLayout(Layout::System, *systemLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Media, *mediaLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Configurations, *configurationsLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Control, *inputLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Presentation, *videoLayout, {~0u, ~0u}, false );
		if (dynamic_cast<LIBC64::Interface*>(emulator))
			tab.setLayout(Layout::Palette, *paletteLayout, {~0u, ~0u}, false );

		tab.setLayout(Layout::Audio, *audioLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Firmware, *firmwareLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Border, *borderLayout, {~0u, ~0u}, false );
		tab.setLayout(Layout::Misc, *miscLayout, {~0u, ~0u} );
	}
    
    onClose = [this]() {
        setVisible(false);
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>( "screen_settings_x", geometry.x);
        settings->set<int>( "screen_settings_y", geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>( "screen_settings_width", geometry.width);
        settings->set<unsigned>( "screen_settings_height", geometry.height);
    };
    
    onDrop = [&]( std::vector<std::string> files ) {
        if ( tab.selection() == Layout::Firmware ) {
            if (firmwareLayout)
                firmwareLayout->drop(files[0]);
        } else if ( tab.selection() == Layout::Media ) {
            if(mediaLayout)
                mediaLayout->drop(files[0]);
        }
    };

    tab.onChange = [&]() {
        prepareLayout( (TabWindow::Layout)tab.selection() );
    };

    translate();
}

auto TabWindow::translate() -> void {
    setTitle( trans->get("config") + " - " + emulator->ident );

    if(inputLayout) inputLayout->translate();
    if(systemLayout) systemLayout->translate();
    if(mediaLayout) mediaLayout->translate();
    if(configurationsLayout) configurationsLayout->translate();
    if(firmwareLayout) firmwareLayout->translate();
    if(borderLayout) borderLayout->translate();
    if(videoLayout) videoLayout->translate();
    if(paletteLayout) paletteLayout->translate();
    if(audioLayout) audioLayout->translate();
    if(miscLayout) miscLayout->translate();

    tab.setHeader(Layout::Control, trans->get("control"));
    tab.setHeader(Layout::System, trans->get("system"));
    tab.setHeader(Layout::Media, trans->get("software"));
    tab.setHeader(Layout::Configurations, trans->get("configurations"));
    tab.setHeader(Layout::Firmware, trans->get("firmware"));
    tab.setHeader(Layout::Border, trans->get("border"));
    tab.setHeader(Layout::Presentation, trans->get("presentation"));
    tab.setHeader(Layout::Misc, trans->get("miscellaneous"));

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        tab.setHeader(Layout::Palette, trans->get("palette"));        
    }
    
    tab.setHeader(Layout::Audio, trans->get("Audio"));
}

auto TabWindow::showDelayed(Layout layout) -> void {
	inputDriver->mUnacquire();
	mtimer.setInterval(30);
	
	mtimer.onFinished = [this, layout]() {
		mtimer.setEnabled(false);
		show(layout);
	};
	mtimer.setEnabled();
}

auto TabWindow::show(Layout layout) -> void {
    setLayout( layout );
    setVisible();
	setFocused();
}

auto TabWindow::prepareLayout(Layout layout) -> void {
    switch(layout) {
        case Layout::Control:
            if (!inputLayout) {
                inputLayout = new InputLayout( this );
                inputLayout->translate();
                tab.setLayout(Layout::Control, *inputLayout, {~0u, ~0u} );
            } break;
        case Layout::System:
            if (!systemLayout) {
                systemLayout = new SystemLayout(this);
                systemLayout->translate();
                tab.setLayout(Layout::System, *systemLayout, {~0u, ~0u} );
            } break;
        case Layout::Media:
            if (!mediaLayout) {
                mediaLayout = new MediaView::MediaLayout( this );
                mediaLayout->build();
                mediaLayout->translate();
                tab.setLayout(Layout::Media, *mediaLayout, {~0u, ~0u} );
            } break;
        case Layout::Configurations:
            if (!configurationsLayout) {
                configurationsLayout = new ConfigurationsLayout( this );
                configurationsLayout->translate();
                tab.setLayout(Layout::Configurations, *configurationsLayout, {~0u, ~0u} );
            } break;
        case Layout::Firmware:
            if (!firmwareLayout) {
                firmwareLayout = new FirmwareLayout( this );
                firmwareLayout->translate();
                tab.setLayout(Layout::Firmware, *firmwareLayout, {~0u, ~0u} );
            } break;
        case Layout::Presentation:
            if (!videoLayout) {
                videoLayout = new VideoLayout( this );
                videoLayout->translate();
                tab.setLayout(Layout::Presentation, *videoLayout, {~0u, ~0u} );
            } break;
        case Layout::Palette:
            if (!paletteLayout) {
                if (dynamic_cast<LIBC64::Interface*>(emulator)) {
                    paletteLayout = new PaletteLayout( this );
                    tab.setLayout(Layout::Palette, *paletteLayout, {~0u, ~0u} );
                    paletteLayout->translate();
                }
            } break;
        case Layout::Audio:
            if (!audioLayout) {
                audioLayout = new AudioLayout( this );
                audioLayout->translate();
                tab.setLayout(Layout::Audio, *audioLayout, {~0u, ~0u} );
            } break;
        case Layout::Border:
            if (!borderLayout) {
                borderLayout = new BorderLayout( this );
                borderLayout->translate();
                tab.setLayout(Layout::Border, *borderLayout, {~0u, ~0u} );
            } break;
        case Layout::Misc:
            if (!miscLayout) {
                miscLayout = new MiscLayout( this );
                miscLayout->translate();
                tab.setLayout(Layout::Misc, *miscLayout, {~0u, ~0u} );
            } break;
    }
}

auto TabWindow::setLayout(Layout layout) -> void {

    prepareLayout( layout );

    if ( (unsigned)layout != tab.selection() )
        tab.setSelection( (unsigned)layout );
}

auto TabWindow::getView( Emulator::Interface* emulator, bool createIfNotExist ) -> TabWindow* {

    if (!emulator)
        return nullptr;

	for (auto view : emuConfigViews) {
		if (view->emulator == emulator)
			return view;
	}

	if (!createIfNotExist)
	    return nullptr;

	auto emuView = new EmuConfigView::TabWindow( emulator );
    emuConfigViews.push_back( emuView );

    emuView->build();

	return emuView;
}

}



#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

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
#include "../media/fileloader.h"

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
#include "layouts/geometry.cpp"
#include "layouts/firmware.cpp"
#include "layouts/palette.cpp"
#include "layouts/misc.cpp"

TabWindow::TabWindow(Emulator::Interface* emulator) {
    this->emulator = emulator;
    this->settings = program->getSettings( emulator );
    message = new Message(this);
}

auto TabWindow::appendTab(Layout layout, GUIKIT::Image& image) -> void {
    tab.appendHeader("", image);
    tabIdents.push_back(layout);
}

auto TabWindow::getTabPos(Layout layout) -> int {
    return GUIKIT::Vector::findPos<Layout>(tabIdents, layout);
}

auto TabWindow::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();
    setDroppable();

    GUIKIT::Geometry defaultGeometry = {50, 50, GUIKIT::Font::scale(1000), GUIKIT::Font::scale(570)};

    if (GUIKIT::Application::isGtk()) {
        defaultGeometry.width = 1200;
        defaultGeometry.height = 700;
    }
    
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

    appendTab(Layout::System, systemImage);
    appendTab(Layout::Media, driveImage);
    appendTab(Layout::Configurations, scriptImage);
    appendTab(Layout::Control, joystickImage);
    appendTab(Layout::Presentation, displayImage);

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        appendTab(Layout::Palette, paletteImage);

    appendTab(Layout::Audio, volumeImage);
    appendTab(Layout::Firmware, memoryImage);
    appendTab(Layout::Geometry, cropImage);
    appendTab(Layout::Misc, miscImage);
	
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
        geometryLayout = new GeometryLayout( this );
		miscLayout = new MiscLayout( this );
		mediaLayout->build();
		
		tab.setLayout( getTabPos(Layout::System), *systemLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Media), *mediaLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Configurations), *configurationsLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Control), *inputLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Presentation), *videoLayout, {~0u, ~0u}, false );
		if (dynamic_cast<LIBC64::Interface*>(emulator))
			tab.setLayout( getTabPos(Layout::Palette), *paletteLayout, {~0u, ~0u}, false );

		tab.setLayout( getTabPos(Layout::Audio), *audioLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Firmware), *firmwareLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Geometry), *geometryLayout, {~0u, ~0u}, false );
		tab.setLayout( getTabPos(Layout::Misc), *miscLayout, {~0u, ~0u} );
	}
    
    onClose = [this]() {
        emuThread->lock();
        setVisible(false);
        emuThread->unlock();
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>( "screen_settings_x", geometry.x);
        settings->set<int>( "screen_settings_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>( "screen_settings_width", geometry.width);
        settings->set<unsigned>( "screen_settings_height", geometry.height);
    };
    
    onDrop = [&]( std::vector<std::string> files ) {
        if ( tab.selection() == getTabPos(Layout::Firmware) ) {
            if (firmwareLayout)
                firmwareLayout->drop(files[0]);
        } else if ( tab.selection() == getTabPos(Layout::Media) ) {
            if(mediaLayout)
                mediaLayout->drop(files[0]);
        }
    };

    tab.onChange = [&]() {
        unsigned tabPos = tab.selection();
        prepareLayout( tabPos );
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
    if(geometryLayout) geometryLayout->translate();
    if(videoLayout) videoLayout->translate();
    if(paletteLayout) paletteLayout->translate();
    if(audioLayout) audioLayout->translate();
    if(miscLayout) miscLayout->translate();

    tab.setHeader( getTabPos(Layout::Control), trans->get("control"));
    tab.setHeader( getTabPos(Layout::System), trans->get("system"));
    tab.setHeader( getTabPos(Layout::Media), trans->get("software"));
    tab.setHeader( getTabPos(Layout::Configurations), trans->get("configurations"));
    tab.setHeader( getTabPos(Layout::Firmware), trans->get("firmware"));
    tab.setHeader( getTabPos(Layout::Geometry), trans->get("geometry"));
    tab.setHeader( getTabPos(Layout::Presentation), trans->get("presentation"));
    tab.setHeader( getTabPos(Layout::Misc), trans->get("miscellaneous"));

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        tab.setHeader( getTabPos(Layout::Palette), trans->get("palette"));
    }
    
    tab.setHeader( getTabPos(Layout::Audio), trans->get("Audio"));
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
    int tabPos = getTabPos(layout);
    if (tabPos < 0)
        return;

    prepareLayout(layout, tabPos);
}

auto TabWindow::prepareLayout(unsigned tabPos) -> void {
    if (tabPos >= tabIdents.size())
        return;

    prepareLayout(tabIdents[tabPos], tabPos);
}

auto TabWindow::prepareLayout(Layout layout, unsigned tabPos) -> void {
    switch(layout) {
        case Layout::Control:
            if (!inputLayout) {
                inputLayout = new InputLayout( this );
                inputLayout->translate();
                tab.setLayout( tabPos, *inputLayout, {~0u, ~0u} );
            } break;
        case Layout::System:
            if (!systemLayout) {
                systemLayout = new SystemLayout(this);
                systemLayout->translate();
                tab.setLayout( tabPos, *systemLayout, {~0u, ~0u} );
            } break;
        case Layout::Media:
            if (!mediaLayout) {
                mediaLayout = new MediaView::MediaLayout( this );
                mediaLayout->build();
                mediaLayout->translate();
                tab.setLayout( tabPos, *mediaLayout, {~0u, ~0u} );
            }
            mediaLayout->setMediaView();
            break;
        case Layout::Configurations:
            if (!configurationsLayout) {
                configurationsLayout = new ConfigurationsLayout( this );
                configurationsLayout->translate();
                tab.setLayout( tabPos, *configurationsLayout, {~0u, ~0u} );
            } break;
        case Layout::Firmware:
            if (!firmwareLayout) {
                firmwareLayout = new FirmwareLayout( this );
                firmwareLayout->translate();
                tab.setLayout( tabPos, *firmwareLayout, {~0u, ~0u} );
            } break;
        case Layout::Presentation:
            if (!videoLayout) {
                videoLayout = new VideoLayout( this );
                videoLayout->translate();
                tab.setLayout( tabPos, *videoLayout, {~0u, ~0u} );
            } break;
        case Layout::Palette:
            if (!paletteLayout && dynamic_cast<LIBC64::Interface*>(emulator)) {
                if (dynamic_cast<LIBC64::Interface*>(emulator)) {
                    paletteLayout = new PaletteLayout( this );
                    tab.setLayout( tabPos, *paletteLayout, {~0u, ~0u} );
                    paletteLayout->translate();
                }
            } break;
        case Layout::Audio:
            if (!audioLayout) {
                audioLayout = new AudioLayout( this );
                audioLayout->translate();
                tab.setLayout( tabPos, *audioLayout, {~0u, ~0u} );
            } break;
        case Layout::Geometry:
            if (!geometryLayout) {
                geometryLayout = new GeometryLayout( this );
                geometryLayout->translate();
                tab.setLayout( tabPos, *geometryLayout, {~0u, ~0u} );
            } break;
        case Layout::Misc:
            if (!miscLayout) {
                miscLayout = new MiscLayout( this );
                miscLayout->translate();
                tab.setLayout( tabPos, *miscLayout, {~0u, ~0u} );
            } break;
    }
}

auto TabWindow::setLayout(Layout layout) -> void {
    unsigned tabPos = getTabPos( layout );
    prepareLayout( layout, tabPos );

    if ( tabPos != tab.selection() )
        tab.setSelection( tabPos );
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



#include "config.h"
#include "../config/archiveViewer.h"
#include "../config/config.h"
#include "../view/view.h"
#include "../input/manager.h"
#include "../view/message.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../tools/status.h"
#include "../firmware/manager.h"
#include "../video/palette.h"
#include "../cmd/cmd.h"
#include "../media/media.h"

#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

std::vector<EmuConfigView::TabWindow*> emuConfigViews;

namespace EmuConfigView {

namespace Icons {
    #include "../../data/img/icons.data"
}

namespace Fonts {
	#include "../../data/fonts/fonts.data"
}

#define mes this->tabWindow->message
	
#include "layouts/input.cpp"
#include "layouts/system.cpp"
#include "layouts/video.cpp"
#include "layouts/border.cpp"
#include "layouts/states.cpp"
#include "layouts/swapper.cpp"
#include "layouts/firmware.cpp"
#include "layouts/palette.cpp"

TabWindow::TabWindow(Emulator::Interface* emulator) {
    this->emulator = emulator;
    message = new Message(this);
}

auto TabWindow::build() -> void {
    winapi.disableBackgroundRedrawDuringResize();
    cocoa.keepMenuVisibilityOnDisplay();
    setDroppable();
	    
    GUIKIT::Geometry defaultGeometry = {100, 100, 850, 540};
    
    GUIKIT::Geometry geometry = {settings->get<int>(ident("screen_settings_x"), defaultGeometry.x)
        ,settings->get<int>(ident("screen_settings_y"), defaultGeometry.y)
        ,settings->get<unsigned>(ident("screen_settings_width"), defaultGeometry.width)
        ,settings->get<unsigned>(ident("screen_settings_height"), defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry ); 

    joystickImage.loadPng((uint8_t*)Icons::joystick, sizeof(Icons::joystick));
    systemImage.loadPng((uint8_t*)Icons::system, sizeof(Icons::system));
    memoryImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    cropImage.loadPng((uint8_t*)Icons::crop, sizeof(Icons::crop));
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    swapperImage.loadPng((uint8_t*)Icons::swapper, sizeof(Icons::swapper));
    paletteImage.loadPng((uint8_t*)Icons::palette, sizeof(Icons::palette));

    inputLayout = new InputLayout( this );
    systemLayout = new SystemLayout( this );
    firmwareLayout = new FirmwareLayout( this );
    videoLayout = new VideoLayout( this );
    if (emulator->ident == "C64")
        paletteLayout = new PaletteLayout( this );
    borderLayout = new BorderLayout( this );
    statesLayout = new StatesLayout( this );
    swapperLayout = new SwapperLayout( this );

    tab.appendHeader("", systemImage);
	tab.appendHeader("", joystickImage); 
	tab.appendHeader("", scriptImage); 
	tab.appendHeader("", displayImage);
	if (emulator->ident == "C64")
        tab.appendHeader("", paletteImage);

    tab.appendHeader("", memoryImage);   
	tab.appendHeader("", cropImage);
    tab.appendHeader("", swapperImage);
                                            

    tab.setLayout(Layout::System, *systemLayout, {~0u, ~0u} );
	tab.setLayout(Layout::Control, *inputLayout, {~0u, ~0u} );
	tab.setLayout(Layout::States, *statesLayout, {~0u, ~0u} );    
	tab.setLayout(Layout::Presentation, *videoLayout, {~0u, ~0u} );
	if (emulator->ident == "C64")
        tab.setLayout(Layout::Palette, *paletteLayout, {~0u, ~0u} );

    tab.setLayout(Layout::Firmware, *firmwareLayout, {~0u, ~0u} );
	tab.setLayout(Layout::Border, *borderLayout, {~0u, ~0u} );
    tab.setLayout(Layout::Swapper, *swapperLayout, {~0u, ~0u} );                        

    tab.setMargin(10);
    tab.setSelection(0);
    
    append(tab);

    onClose = [this]() {
        setVisible(false);
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>( ident("screen_settings_x"), geometry.x);
        settings->set<int>( ident("screen_settings_y"), geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>( ident("screen_settings_width"), geometry.width);
        settings->set<unsigned>( ident("screen_settings_height"), geometry.height);
    };
    
    onDrop = [&]( std::vector<std::string> files ) {
        if ( tab.selection() == Layout::Firmware )
            firmwareLayout->drop( files[0] );
    };

    translate();
}

auto TabWindow::translate() -> void {
    setTitle( trans->get("config") + " - " + emulator->ident );

    inputLayout->translate();
    systemLayout->translate();
    firmwareLayout->translate();
    borderLayout->translate();
    videoLayout->translate();
    statesLayout->translate();
    swapperLayout->translate();
    if (paletteLayout)
        paletteLayout->translate();

    tab.setHeader(Layout::Control, trans->get("control"));
    tab.setHeader(Layout::System, trans->get("system"));
    tab.setHeader(Layout::Firmware, trans->get("firmware"));
    tab.setHeader(Layout::Border, trans->get("border"));
    tab.setHeader(Layout::Presentation, trans->get("presentation"));
    tab.setHeader(Layout::States, trans->get("states"));
    tab.setHeader(Layout::Swapper, trans->get("disk_swapper"));
    if (emulator->ident == "C64")
        tab.setHeader(Layout::Palette, trans->get("palette"));
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

auto TabWindow::update() -> void {
	systemLayout->setEnabled( emulator != activeEmulator || !program->isRunning );
	statesLayout->directSave.save.setEnabled( emulator == activeEmulator && program->isRunning );	
}

auto TabWindow::ident( std::string name ) -> std::string {
	std::string _ident = emulator->ident;
    return GUIKIT::String::toLowerCase( _ident )+ "_" + GUIKIT::String::replace(name, " ", "_");
}

auto TabWindow::getView( Emulator::Interface* emulator ) -> TabWindow* {
	
	for (auto view : emuConfigViews) {
		if (view->emulator == emulator)
			return view;
	}
	return nullptr;
}

}

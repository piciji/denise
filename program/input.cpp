
#include "program.h"
#include "input/manager.h"
#include "view/view.h"

auto Program::initInput() -> void {   
	
	if (inputDriver) delete inputDriver;
    
    if (cmd->noDriver) {
        inputDriver = new DRIVER::Input;
        return;
    }
    
    inputDriver = DRIVER::Input::create( getInputDriver() );
	
    if ( !inputDriver->init( view->handle() ) ) {
        delete inputDriver;
        inputDriver = new DRIVER::Input;
    }
	InputManager::init();

    if (configView)
	    configView->inputLayout->loadInputList();

	for( auto emuView : emuConfigViews ) {
        if (emuView->inputLayout)
            emuView->inputLayout->loadDeviceList();
    }
}

auto Program::getInputDriver() -> std::string {
	auto curDriver = globalSettings->get<std::string>("input_driver", "");
	auto drivers = DRIVER::Input::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver)
            return driver;
	}
    
	return DRIVER::Input::preferred();
}

auto Program::jitPoll(int delay) -> bool {
    if (cmd->noGui)
        return false;

    return InputManager::jitPoll(delay);
}

auto Program::inputPoll( uint16_t deviceId, uint16_t inputId) -> int16_t {            
    auto guid = activeEmulator->devices[deviceId].inputs[inputId].guid;
    auto mapping = (InputMapping*)guid;
    if(mapping)
        return mapping->state;
    
    return 0;
}

auto Program::informCapsLock(bool state) -> void {

}

auto Program::getDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device* {
    unsigned defaultDevice = 0;

    for(auto& device : emulator->devices) {
        if (device.isJoypad() && connector->isPort2()) {
            defaultDevice = device.id;
            break;
        }
        if (device.isMouse() && dynamic_cast<LIBAMI::Interface*>( emulator ) && connector->isPort1()) {
            defaultDevice = device.id;
            break;
        }
    }

    unsigned deviceId = getSettings(emulator)->get<unsigned>( _underscore(connector->name), defaultDevice);
    
    return emulator->getDevice( deviceId );      
}

auto Program::isAnalogDeviceConnected( ) -> bool {
    
    if (!activeEmulator)
        return false;
    
    for(auto& connector : activeEmulator->connectors) {
        
        auto device = activeEmulator->getConnectedDevice( &connector );
        
        if ( device->isMouse() || device->isPaddles() || device->isLightDevice() )
            return true;        
    }
    
    return false;
}

auto Program::couldDeviceBlockSecondMouseButton( ) -> bool {
    
    if (!activeEmulator)
        return false;
    
    for(auto& connector : activeEmulator->connectors) {
        
        auto device = activeEmulator->getConnectedDevice( &connector );
        
        // light devices are usable even if mouse is not acquired.
        // some of these devices (light pens) needs two mouse buttons.
        // normally the second mouse button is reserved for displaying context menu.
        // in this case, we want to disable context menu.
        if ( device->isLightDevice() && device->inputs.size() > 3 )
            return true;                    
    }
    
    return false;
}

auto Program::absoluteMouseToEmu( Emulator::Interface* emulator ) -> GUIKIT::Position {    
    
    // absolute mouse position within viewport.
    GUIKIT::Position absPos = view->viewport.getMousePosition();

    // GUIKIT::Geometry geometry = view->viewport.geometry();
    DRIVER::Viewport& viewport = videoDriver->getViewport();

    if (!viewport.width || !viewport.height)
        return absPos;

    if (absPos.x > viewport.x)
        absPos.x -= viewport.x;
    else
        absPos.x = 0;

    if (absPos.y > viewport.y)
        absPos.y -= viewport.y;
    else
        absPos.y = 0;

    unsigned emuWidth = emulator->cropWidth();
    unsigned emuHeight = emulator->cropHeight();

    // scale host position to emu position    
    absPos.x = (absPos.x * emuWidth) / viewport.width;
    absPos.y = (absPos.y * emuHeight) / viewport.height;

    return absPos;  
}

auto Program::resetRunAhead() -> void {
    
    auto settings = getSettings( activeEmulator );
    
    if ( settings->get<bool>( "runahead_disable", true) ) {
        
        settings->set<unsigned>( "runahead", 0);
        
        activeEmulator->runAhead( 0 );

        auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );

        if (emuView && emuView->miscLayout)
            emuView->miscLayout->setRunAhead( 0, false );
    }   
}

auto Program::setJit(Emulator::Interface* emulator) -> void {

    auto settings = getSettings(emulator);

    emulator->setInputSampling( settings->get<unsigned>("input_sampling", 2, {0, 2}) );

    auto manager = InputManager::getManager(emulator);

    manager->jit.rescanDelay = settings->get<unsigned>("input_jit_delay", 5, {1, 10});
}

auto Program::setRunAhead(Emulator::Interface* emulator) -> void {
    
    auto settings = getSettings( emulator );
    
    emulator->runAhead( settings->get<unsigned>( "runahead", 0, {0u, 10u}) );
    
    emulator->runAheadPerformance( settings->get<bool>( "runahead_performance", dynamic_cast<LIBAMI::Interface*>(emulator)) );

    emulator->runAheadPreventJit( settings->get<bool>( "runahead_prevent_jit", true ) );
}

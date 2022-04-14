
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

auto Program::jitPoll() -> bool {
    
    return InputManager::jitPoll();
}

auto Program::inputPoll( uint16_t deviceId, uint16_t inputId) -> int16_t {            
    auto guid = activeEmulator->devices[deviceId].inputs[inputId].guid;
    auto mapping = (InputMapping*)guid;
    if(mapping)
        return mapping->state;
    
    return 0;
}

auto Program::getDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device* {
    
    unsigned deviceId = getSettings(emulator)->get<unsigned>( _underscore(connector->name), 0);
    
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

    GUIKIT::Geometry geometry = view->viewport.geometry();
    unsigned emuWidth = emulator->cropWidth();
    unsigned emuHeight = emulator->cropHeight();

    // scale host position to emu position    
    absPos.x = (absPos.x * emuWidth) / geometry.width;
    absPos.y = (absPos.y * emuHeight) / geometry.height;     

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

    emulator->enableJit( settings->get<bool>("input_jit", true) );

    auto manager = InputManager::getManager(emulator);

    manager->jit.rescanDelay = settings->get<unsigned>("input_jit_delay", 3, {1, 8});
}

auto Program::setRunAhead(Emulator::Interface* emulator) -> void {
    
    auto settings = getSettings( emulator );
    
    emulator->runAhead( settings->get<unsigned>( "runahead", 0, {0u, 10u}) );
    
    emulator->runAheadPerformance( settings->get<bool>( "runahead_performance", false) );

    emulator->runAheadPreventJit( settings->get<bool>( "runahead_prevent_jit", true ) );
}

auto Program::setAutofire(Emulator::Interface* emulator) -> void {

    auto settings = getSettings( emulator );

    auto manager = InputManager::getManager( emulator );

    manager->autoFireMode = settings->get<unsigned>( "autofire_mode", 0 );

    manager->autoFireFrequency = settings->get<unsigned>( "autofire_frequency", 1, {1, 200} );

    for (auto mapping : manager->autoFireMappings) {
        mapping->autoFire = settings->get<bool>( "autofire_" + mapping->setting->getIdent(), false );
    }

    manager->updateAutofireMappingsInUse();
}

#include "cmd.h"
#include "../view/view.h"
    
auto Cmd::set(int argc, char** argv) -> void {

    for (unsigned i = 0; i < argc; i++) {

        arguments.push_back( argv[i] );
        
        if ( (std::string)argv[i] == "-no-gui" )
            GUIKIT::Application::dummy = true;
        
        if ( (std::string)argv[i] == "-debugcart" )
            debug = true;            
    }  
    
}

auto Cmd::parse() -> void {
    
    std::vector<std::string> allowedSuffix = collectAllowedSuffix();
    std::vector<std::string> paths;
    bool limitCyclesNext = false;

    for( auto& arg : arguments ) {
        if (limitCyclesNext) {
            limitCyclesNext = 0;
                        
            if (GUIKIT::String::isNumber( arg )) {
                unsigned cycles = 0;    
                try {
                    cycles = std::stoi(arg);
                } catch(...) {}

                dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->activateDebugCart( cycles );
                
                continue;
            }            
        }          
        
        if (arg == "-vic-6569R3") { // pal 
            updateChipset(getEmulator("C64"), 0, 1);
        }
        else if (arg == "-vic-8565") { // pal 
            updateChipset(getEmulator("C64"), 1, 1);
        }
        else if (arg == "-vic-6567R8") { // ntsc 
            updateChipset(getEmulator("C64"), 0, 0);
        }
        else if (arg == "-vic-8562") { // ntsc 
            updateChipset(getEmulator("C64"), 1, 0);
        }
        else if (arg == "-sid-6581") {
            updateFeature( getEmulator("C64"), LIBC64::Interface::FeatureIdSid, 1 );
        }
        else if (arg == "-sid-8580") {
            updateFeature( getEmulator("C64"), LIBC64::Interface::FeatureIdSid, 0 );
        }
        else if (arg == "-cia-6526a") {
            updateFeature( getEmulator("C64"), LIBC64::Interface::FeatureIdCiaRev, 1 );
        }
        else if (arg == "-cia-6526") {
            updateFeature( getEmulator("C64"), LIBC64::Interface::FeatureIdCiaRev, 0 );
        }
        else if (arg == "-debugcart") {
            dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->activateDebugCart();            
            prepareDrives( getEmulator("C64") );
          //  disableCpuWaster( getEmulator("C64") );
			settings->set<bool>("audio_sync", false );
			settings->set<bool>("video_sync", false );
			settings->set<bool>("dynamic_rate_control", false );			
        }            
        else if (arg == "-limitcycles") {
            limitCyclesNext = 1;
        }
        else if (arg == "-no-driver") {
            dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->disableFilterCircuit();
            noDriver = 1;
        }
        else if (arg == "-no-gui") {
            noGui = 1;
            noDriver = 1;
            dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->disableFilterCircuit();
           // GUIKIT::Application::dummy = true;
			
        }
        else {
            std::string temp = arg;
            GUIKIT::String::toLowerCase( temp );
            
            for(auto& suffix : allowedSuffix) {
                
                if (GUIKIT::String::foundSubStr( temp, "." + suffix )) {
                    paths.push_back( arg );  
                    autoload = true;
                    break;
                }
            }                                  
        }
    }

    arguments = paths;
}

auto Cmd::autoloadImages() -> void {

    if (!autoload) {
        if (noGui)
            program->exit(1);
        
        return;
    }
    
    view->autoloadInit( arguments, 1 );
    
    view->autoloadFiles();
}

auto Cmd::getEmulator( std::string ident ) -> Emulator::Interface* {

    for( auto emulator : emulators ) {

        if (ident == "C64" && dynamic_cast<LIBC64::Interface*>(emulator) ) 
            return emulator;            
    }
    
    return nullptr;
}

auto Cmd::updateFeature( Emulator::Interface* emulator, unsigned ident, int value) -> void {

    for( auto& feature : emulator->features ) {

        if(feature.id == ident) {

            settings->set<int>( program->ident( emulator, feature.name ), value );

            return;
        }
    }
}

auto Cmd::updateChipset( Emulator::Interface* emulator, unsigned ident, bool pal) -> void {            

    settings->set( program->ident( emulator, "chipset" ), ident );

    settings->set<unsigned>( program->ident( emulator, "video_region"), pal ? 0 : 1);
}

auto Cmd::prepareDrives( Emulator::Interface* emulator ) -> void {
    
    for(auto& driveGroup : emulator->driveGroups) {
        
        if (driveGroup.isDiskDrive())
            settings->set<unsigned>( program->ident(emulator, driveGroup.name + "_count"), 1);
		
		else if (driveGroup.isTapeDrive())
            settings->set<unsigned>( program->ident(emulator, driveGroup.name + "_count"), 0);
        
//        for(auto& drive : driveGroup.drives) {
//            
//            settings->remove( program->ident( emulator, drive.name + "_path") );
//            settings->remove( program->ident( emulator, drive.name + "_file") );
//            settings->remove( program->ident( emulator, drive.name + "_id") );
//            settings->remove( program->ident( emulator, drive.name + "_wp") );
//            settings->remove( program->ident( emulator, drive.name + "_wp_enabled") );
//        }
    }
}

//auto Cmd::disableCpuWaster( Emulator::Interface* emulator ) -> void {
//    
//    for (auto& feature : emulator->features) {
//        if (feature.id == LIBC64::Interface::FeatureId::FeatureIdPowerThread)
//            settings->set<bool>( program->ident( emulator, feature.name ), false );
//        
//        if (feature.id == LIBC64::Interface::FeatureId::FeatureIdSidAccuracy)
//            settings->set<bool>( program->ident( emulator, feature.name ), false );
//    }           
//}

auto Cmd::collectAllowedSuffix() -> std::vector<std::string> {
    std::vector<std::string> allowedSuffix;
    
    for( auto emulator : emulators ) {
        for( auto& driveGroup : emulator->driveGroups ) {
            
            for (auto suffix : driveGroup.suffix) {
                
                GUIKIT::String::toLowerCase( suffix );
                
                allowedSuffix.push_back( suffix );
            }
        }
    }
        
    return allowedSuffix;
}
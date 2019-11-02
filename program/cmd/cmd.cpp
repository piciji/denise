
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
    bool reuSizeNext = false;
    bool aneMagicNext = false;

    for( auto& arg : arguments ) {
        if (limitCyclesNext) {
            limitCyclesNext = false;
            setCycles( arg );
            continue;          
        }          
        
        if (reuSizeNext) {
            reuSizeNext = false;                        
            setReuSize( arg );
            continue;
        }
        
        if (aneMagicNext) {
            aneMagicNext = false;
            setAneMagic( arg );
            continue;
        }
        
        if (arg == "-vic-6569R3") { // pal 
            updateChipset(getEmulator("C64"), 0);
            updateRegion(getEmulator("C64"), true);
            lockRegion = true;
        }
        else if (arg == "-vic-8565") { // pal 
            updateChipset(getEmulator("C64"), 1);
            updateRegion(getEmulator("C64"), true);
            lockRegion = true;
        }
        else if (arg == "-vic-6567R8") { // ntsc 
            updateChipset(getEmulator("C64"), 0);
            updateRegion(getEmulator("C64"), false);
            lockRegion = true;
        }
        else if (arg == "-vic-8562") { // ntsc 
            updateChipset(getEmulator("C64"), 1);
            updateRegion(getEmulator("C64"), false);
            lockRegion = true;
        }
        else if (arg == "-pal") { // pal 
            if (!lockRegion)
                updateRegion(getEmulator("C64"), true);
        }
        else if (arg == "-ntsc") { // pal 
            if (!lockRegion)
                updateRegion(getEmulator("C64"), false);
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
			dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->disableFilterCircuit();
            prepareDrives( getEmulator("C64") );
			settings->set<bool>("audio_sync", false );
			settings->set<bool>("video_sync", false );
			settings->set<bool>("dynamic_rate_control", false );			
            settings->set<bool>("fps", true );			
            settings->set("video_screen_text", 2);
        }            
        else if (arg == "-limitcycles") {
            limitCyclesNext = true;
        }
        else if (arg == "-reu") {
            reuSizeNext = true;
        }
        else if (arg == "-ane-magic") {
            aneMagicNext = true;
        }
        else if (arg == "-no-driver") {
            dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->disableFilterCircuit();
            noDriver = 1;
        }
        else if (arg == "-no-gui") {
            noGui = 1;
            noDriver = 1;
            dynamic_cast<LIBC64::Interface*>(getEmulator("C64"))->disableFilterCircuit();			
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

auto Cmd::updateChipset( Emulator::Interface* emulator, unsigned ident) -> void {            

    settings->set( program->ident( emulator, "chipset" ), ident );   
}

auto Cmd::updateRegion( Emulator::Interface* emulator, bool pal ) -> void {
    
    settings->set<unsigned>( program->ident( emulator, "video_region"), pal ? 0 : 1);
}

auto Cmd::prepareDrives( Emulator::Interface* emulator ) -> void {
    
    for(auto& mediaGroup : emulator->mediaGroups) {
        
        if (mediaGroup.isDisk())
            settings->set<unsigned>( program->ident(emulator, mediaGroup.name + "_count"), 1);
		
		else if (mediaGroup.isTape())
            settings->set<unsigned>( program->ident(emulator, mediaGroup.name + "_count"), 0);        
    }
}

auto Cmd::collectAllowedSuffix() -> std::vector<std::string> {
    std::vector<std::string> allowedSuffix;
    
    for( auto emulator : emulators ) {
        for( auto& mediaGroup : emulator->mediaGroups ) {
            
            for (auto suffix : mediaGroup.suffix) {
                
                GUIKIT::String::toLowerCase( suffix );
                
                allowedSuffix.push_back( suffix );
            }
        }
    }
        
    return allowedSuffix;
}

auto Cmd::setCycles(std::string arg) -> void {
    
    if (!GUIKIT::String::isNumber( arg ))
        return;
     
    unsigned cycles = 0;
    try {
        cycles = std::stoi(arg);
    } catch (...) {
        return;
    }

    dynamic_cast<LIBC64::Interface*> (getEmulator("C64"))->activateDebugCart(cycles);  
}

auto Cmd::setAneMagic(std::string arg) -> void {
    
    auto magic = GUIKIT::String::convertHexToInt( arg, 0xee ) & 0xff;
    
    updateFeature( getEmulator("C64"), LIBC64::Interface::FeatureIdCpuAneMagic, magic );
}

auto Cmd::setReuSize(std::string arg) -> void {
    
    if (!GUIKIT::String::isNumber( arg ))
        return;
        
    unsigned reuSize = 0;    
    try {
        reuSize = std::stoi(arg);
    } catch(...) {
        return;
    }

    auto emulator = getEmulator("C64");    
    auto& expansion = emulator->expansions[ LIBC64::Interface::ExpansionIdReu ];
    auto memoryType = expansion.memoryType;

    for(auto& memory : memoryType->memory) {
        if (memory.size == reuSize) {
            
            settings->set<unsigned>( program->ident(emulator, memoryType->name + "_mem"), memory.id);
            
            settings->set<unsigned>( program->ident(emulator, "expansion"), expansion.id);
        }
    }       
}
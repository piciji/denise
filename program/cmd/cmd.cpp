
#include "cmd.h"
#include "../view/view.h"    

auto Cmd::set(int argc, char** argv) -> void {
    
    options.push_back( {"-v, --version", "Output program version", ""} );    
    options.push_back( {"-h, --help", "Output this help screen", ""} );    
    options.push_back( {"-vic-6569R3", "Select VIC-II 6569R3 and PAL mode", ""} );
    options.push_back( {"-vic-8565", "Select VIC-II 8565 and PAL mode", ""} );
    options.push_back( {"-vic-6567R8", "Select VIC-II 6567R8 and NTSC mode", ""} );
    options.push_back( {"-vic-8562", "Select VIC-II 6562 and NTSC mode", ""} );
    options.push_back( {"-pal", "Select PAL Mode", ""} );
    options.push_back( {"-ntsc", "Select NTSC Mode", ""} );
    options.push_back( {"-sid-6581", "Select SID 6581", ""} );
    options.push_back( {"-sid-8580", "Select SID 8580", ""} );
    options.push_back( {"-cia-6526a", "Select CIA 6526a", ""} );
    options.push_back( {"-cia-6526", "Select CIA 6526", ""} );
    options.push_back( {"-reu", "Emulate REU Expansion", "<size in kb>"} );
    options.push_back( {"-debugcart", "Generate exit codes for VICE Testbench", ""} );    
    options.push_back( {"-limitcycles", "Specify number of cycles to run before quitting with an error (checks at complete frames)", "<cycles>"} );
    options.push_back( {"-exitscreenshot", "Save screen to PNG file, when exiting App", "<filePath>"} );    
    options.push_back( {"-ane-magic", "Force CPU to use this value for ANE and LAX opcode", "<value>"} );
    options.push_back( {"-no-driver", "Run without video, audio, input drivers", ""} );
    options.push_back( {"-no-gui", "Open without graphical user interface and force -no-driver", ""} );    
    
    for (unsigned i = 0; i < argc; i++) {

        arguments.push_back( argv[i] );
        
        if ( (std::string)argv[i] == "-no-gui" )
            GUIKIT::Application::dummy = true;
        
        else if ( (std::string)argv[i] == "-debugcart" )
            debug = true;            
        
        else if ( (std::string)argv[i] == "-h" )
            helpRequested = true;
        
        else if ( (std::string)argv[i] == "--help" )
            helpRequested = true;
        
        else if ( (std::string)argv[i] == "-v" )
            versionRequested = true;
        
        else if ( (std::string)argv[i] == "--version" )
            versionRequested = true;
    }  

}

auto Cmd::printHelp() -> void {
    
    GUIKIT::System::printToCmd( "\n" );
    
    if (versionRequested) {
        
        GUIKIT::System::printToCmd( "Version: " + (std::string)VERSION + "\n" );
        
        return;
    }    
    
    GUIKIT::System::printToCmd( "Usage: Denise [option]... [image path]... \n\n" );
    GUIKIT::System::printToCmd( "Available command-line options:\n" );
    
    for(auto& option : options) {                
        
        if (!option.param.empty())
            GUIKIT::System::printToCmd( option.ident + " " + option.param + "\n" );
        else
            GUIKIT::System::printToCmd( option.ident + "\n" );
        
        GUIKIT::System::printToCmd( "\t" + option.description + "\n" );
    }
}

auto Cmd::parse() -> void {
    
    std::vector<std::string> allowedSuffix = collectAllowedSuffix();
    std::vector<std::string> paths;
    bool limitCyclesNext = false;
    bool reuSizeNext = false;
    bool aneMagicNext = false;
    bool screenshotPathNext = false;
    typedef Emulator::Interface EmuInt;
	auto emuC64 = program->getEmulator("C64");

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
        
        if (screenshotPathNext) {
            screenshotPathNext = false;     	
            settings->set<unsigned>( program->ident( emuC64, "crop_type"), (unsigned)EmuInt::CropType::Monitor );            
            screenshotPath = arg; 
            continue;
        }
        
        if (arg == "-vic-6569R3") { // pal 
            updateChipset(emuC64, 0);
            updateRegion(emuC64, true);
            lockRegion = true;
        }
        else if (arg == "-vic-8565") { // pal 
            updateChipset(emuC64, 1);
            updateRegion(emuC64, true);
            lockRegion = true;
        }
        else if (arg == "-vic-6567R8") { // ntsc 
            updateChipset(emuC64, 0);
            updateRegion(emuC64, false);
            lockRegion = true;
        }
        else if (arg == "-vic-8562") { // ntsc 
            updateChipset(emuC64, 1);
            updateRegion(emuC64, false);
            lockRegion = true;
        }
        else if (arg == "-pal") { // pal 
            if (!lockRegion)
                updateRegion(emuC64, true);
        }
        else if (arg == "-ntsc") { // pal 
            if (!lockRegion)
                updateRegion(emuC64, false);
        }
        else if (arg == "-sid-6581") {
            updateFeature( emuC64, LIBC64::Interface::FeatureIdSid, 1 );
        }
        else if (arg == "-sid-8580") {
            updateFeature( emuC64, LIBC64::Interface::FeatureIdSid, 0 );
        }
        else if (arg == "-cia-6526a") {
            updateFeature( emuC64, LIBC64::Interface::FeatureIdCiaRev, 1 );
        }
        else if (arg == "-cia-6526") {
            updateFeature( emuC64, LIBC64::Interface::FeatureIdCiaRev, 0 );
        }
        else if (arg == "-debugcart") {
            dynamic_cast<LIBC64::Interface*>(emuC64)->activateDebugCart();   
			dynamic_cast<LIBC64::Interface*>(emuC64)->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut );
            prepareDrives( emuC64 );
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
            dynamic_cast<LIBC64::Interface*>(emuC64)->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::NoVideoOut );
            noDriver = 1;
        }
        else if (arg == "-no-gui") {
            noGui = 1;
            noDriver = 1;
            dynamic_cast<LIBC64::Interface*>(emuC64)->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::NoVideoOut );			
        }
        else if (arg == "-exitscreenshot") {
            screenshotPathNext = true;
            
        } else {
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
    
    if (!debug && !noDriver && !noGui && settings->get<bool>("open_fullscreen", false)) {
        view->setFullScreen(true);
    }
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

    dynamic_cast<LIBC64::Interface*>( program->getEmulator("C64") )->activateDebugCart(cycles);  
}

auto Cmd::setAneMagic(std::string arg) -> void {
    
    auto magic = GUIKIT::String::convertHexToInt( arg, 0xee ) & 0xff;
    
    updateFeature( program->getEmulator("C64"), LIBC64::Interface::FeatureIdCpuAneMagic, magic );
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

    auto emulator = program->getEmulator("C64");    
    auto& expansion = emulator->expansions[ LIBC64::Interface::ExpansionIdReu ];
    auto memoryType = expansion.memoryType;

    for(auto& memory : memoryType->memory) {
        if (memory.size == reuSize) {
            
            settings->set<unsigned>( program->ident(emulator, memoryType->name + "_mem"), memory.id);
            
            settings->set<unsigned>( program->ident(emulator, "expansion"), expansion.id);
        }
    }       
}


#include "cmd.h"
#include "../view/view.h"    

auto Cmd::set(int argc, char** argv) -> void {
    
    options.push_back( {"-v, --version", "Output program version", ""} );    
    options.push_back( {"-h, --help", "Output this help screen", ""} );  
	
    options.push_back( {"-vic-6569R3", "Select VIC-II 6569R3 and PAL mode", ""} );
    options.push_back( {"-vic-8565", "Select VIC-II 8565 and PAL mode", ""} );
    options.push_back( {"-vic-6567R8", "Select VIC-II 6567R8 and NTSC mode", ""} );
    options.push_back( {"-vic-8562", "Select VIC-II 6562 and NTSC mode", ""} );	
	options.push_back( {"-vic-6569R1", "Select VIC-II 6569R1 and PAL mode", ""} );
	options.push_back( {"-vic-6567R56A", "Select VIC-II 6567R56A and NTSC mode", ""} );
	options.push_back( {"-vic-6572", "Select VIC-II 6572 and PAL mode", ""} );
	options.push_back( {"-vic-6573", "Select VIC-II 6573 and NTSC mode with PAL Encoding", ""} );
		
    options.push_back( {"-sid-6581", "Select SID 6581", ""} );
    options.push_back( {"-sid-8580", "Select SID 8580", ""} );
	
    options.push_back( {"-cia-6526a", "Select CIA 6526a", ""} );
    options.push_back( {"-cia-6526", "Select CIA 6526", ""} );
	
    options.push_back( {"-reu", "Emulate REU Expansion", "<size in kb>"} );
    options.push_back( {"-debugcart", "Generate exit codes for VICE Testbench", ""} );    
    options.push_back( {"-limitcycles", "Specify number of cycles to run before quitting with an error (checks at complete frames)", "<cycles>"} );
    options.push_back( {"-exitscreenshot", "Save screen to PNG file, when exiting App", "<filePath>"} );    
    options.push_back( {"-ane-magic", "Force CPU to use this value for ANE opcode", "<value>"} );
	options.push_back( {"-lax-magic", "Force CPU to use this value for LAX opcode", "<value>"} );
    options.push_back( {"-no-driver", "Run without video, audio, input drivers", ""} );
    options.push_back( {"-no-gui", "Open without graphical user interface and force -no-driver", ""} );    
	options.push_back( {"-autostart-prg", "Set autostart mode for PRG files (1: Inject, 2: Disk image)", "<value>"} );
	options.push_back( {"-aggressive-fastforward", "aggressive Warp mode (emulates VIC sequencer every 15 frames only)", ""} );
	options.push_back( {"-fast-testbench", "analyze passed options and then decides on the use of aggressive fastforward and/or PRG memory injection", ""} );
    
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
	bool laxMagicNext = false;
    bool screenshotPathNext = false;
	bool autostartPrgNext = false;
	bool d64InUse = false;
	bool fastTestbench = false;
    typedef Emulator::Interface EmuInt;
	auto emuC64 = program->getEmulator("C64");
	auto diskGroup = emuC64->getDiskMediaGroup();
    GUIKIT::Settings* settingsC64 = program->getSettings( emuC64 );

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

		if (laxMagicNext) {
			laxMagicNext = false;
			setLaxMagic(arg);
			continue;
		}
        
        if (screenshotPathNext) {
            screenshotPathNext = false;     	
            settingsC64->set<unsigned>( "crop_type", (unsigned)EmuInt::CropType::Monitor );            
            screenshotPath = arg; 
            continue;
        }
		
		if(autostartPrgNext) {
			autostartPrgNext = false;			
			setAutoStartPrg( arg );
		}
        
        if (arg == "-vic-6569R3") { // pal 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 0 );
        }
        else if (arg == "-vic-8565") { // pal 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 1 );
        }
        else if (arg == "-vic-6567R8") { // ntsc 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 2 );
        }
        else if (arg == "-vic-8562") { // ntsc 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 3 );
        }				
		else if (arg == "-vic-6569R1") { // pal 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 4 );
        }
		else if (arg == "-vic-6567R56A") { // ntsc 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 5 );
        }
		else if (arg == "-vic-6572") { // pal
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 6 );
        }
		else if (arg == "-vic-6573") { // ntsc geometry, pal encoding 
			updateModel( emuC64, LIBC64::Interface::ModelIdVicIIModel, 7 );
        }
        else if (arg == "-sid-6581") {
            updateModel( emuC64, LIBC64::Interface::ModelIdSid, 1 );
        }
        else if (arg == "-sid-8580") {
            updateModel( emuC64, LIBC64::Interface::ModelIdSid, 0 );
        }
        else if (arg == "-cia-6526a") {
            updateModel( emuC64, LIBC64::Interface::ModelIdCiaRev, 1 );
        }
        else if (arg == "-cia-6526") {
            updateModel( emuC64, LIBC64::Interface::ModelIdCiaRev, 0 );
        }
		else if (arg == "-fast-testbench") {
            fastTestbench = true;
        }
        else if (arg == "-debugcart") {
            dynamic_cast<LIBC64::Interface*>(emuC64)->activateDebugCart();   
            prepareDrives( emuC64 );
			globalSettings->set<bool>("audio_sync", false );
			globalSettings->set<bool>("video_sync", false );
			globalSettings->set<bool>("dynamic_rate_control", false );			
            globalSettings->set<bool>("fps", true );			
            globalSettings->set("video_screen_text", 2);
            settingsC64->set<bool>( "video_cycle_accuracy", true );  
			updateModel( emuC64, LIBC64::Interface::ModelIdDisableGreyDotBug, 0 );
			
			if (!autostartPrgOverride)
				autostartPrg = 2;
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
		else if (arg == "-lax-magic") {
            laxMagicNext = true;
        }
        else if (arg == "-no-driver") {
            noDriver = 1;
        }
        else if (arg == "-no-gui") {
            noGui = 1;
            noDriver = 1;
        }
		else if (arg == "-aggressive-fastforward") {
			aggressiveFastforward = 1;
        }
        else if (arg == "-exitscreenshot") {
            screenshotPathNext = true;
		}
        else if (arg == "-autostart-prg") {
            autostartPrgNext = true;
                        
        } else {
            std::string temp = arg;
            GUIKIT::String::toLowerCase( temp );
            
            for(auto& suffix : allowedSuffix) {
                								
                if (GUIKIT::String::foundSubStr( temp, "." + suffix )) {
                    paths.push_back( arg );  
                    autoload = true;
					
					if (diskGroup) {
						auto _vec = diskGroup->suffix;
						if ( std::find(_vec.begin(), _vec.end(), suffix) != _vec.end() )
							d64InUse = true;
					}
					
                    break;
                }
            }                                  
        }
    }
	
	if (fastTestbench) {
		aggressiveFastforward = true;
		
		if (!screenshotPath.empty())
			aggressiveFastforward = false;
		else {
			for(auto path : paths) {		
				GUIKIT::String::toLowerCase( path );
				
				if (GUIKIT::String::foundSubStr( path, "vicii" )) {
					aggressiveFastforward = false;								
				}
				
				if (GUIKIT::String::foundSubStr( path, "fuxxor" )) {
					fastTestbench = false;
					aggressiveFastforward = false;
				}
				
				if (GUIKIT::String::foundSubStr( path, "ram0001" )) {
					aggressiveFastforward = false;
				}				
			}
		}
	}
	
	if (fastTestbench)
		autostartPrg = 1;	
	
	else if (!d64InUse && (autostartPrg == 2)) {
				
		if (diskGroup)
			diskGroup->suffix.push_back("prg");
	}
		
    arguments = paths;
}

auto Cmd::autoloadImages() -> void {

    if (!autoload) {
        if (noGui)
            program->exit(1);
        
        return;
    }
    
    view->autoloadInit( arguments, true, View::AutoLoad::AutoStart );
    
    view->autoloadFiles();
    
    if (!debug && !noDriver && !noGui && globalSettings->get<bool>("open_fullscreen", false)) {
        view->setFullScreen(true);
    }
	typedef Emulator::Interface EmuInt;
	
	if (activeEmulator) {
		if (aggressiveFastforward)
			activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::ReduceVideoOutput | (unsigned)EmuInt::FastForward::NoVideoSequencer );
		else if (noDriver || noGui)
			activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::NoVideoOut );	
		else if (debug)
			activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut );
	}	
}

auto Cmd::updateModel( Emulator::Interface* emulator, unsigned ident, int value) -> void {

    for( auto& model : emulator->models ) {

        if(model.id == ident) {

            auto settings = program->getSettings( emulator );
            
            settings->set<int>( _underscore( model.name ), value );

            return;
        }
    }
}

auto Cmd::prepareDrives( Emulator::Interface* emulator ) -> void {
    
    auto settings = program->getSettings( emulator );
    
    for(auto& mediaGroup : emulator->mediaGroups) {
        
        if (mediaGroup.isDisk())
            settings->set<unsigned>( _underscore( mediaGroup.name + "_count"), 1);
		
		else if (mediaGroup.isTape())
            settings->set<unsigned>( _underscore( mediaGroup.name + "_count"), 0);        
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
    
    updateModel( program->getEmulator("C64"), LIBC64::Interface::ModelIdCpuAneMagic, magic );
}

auto Cmd::setLaxMagic(std::string arg) -> void {
    
    auto magic = GUIKIT::String::convertHexToInt( arg, 0xee ) & 0xff;
    
    updateModel( program->getEmulator("C64"), LIBC64::Interface::ModelIdCpuLaxMagic, magic );
}

auto Cmd::setAutoStartPrg(std::string arg) -> void {    
	
	autostartPrg = 1; // inject 
	autostartPrgOverride = false;
	
    if (!GUIKIT::String::isNumber( arg ))
        return;
             
    try {
        autostartPrg = std::stoi(arg);	
		autostartPrgOverride = true;
    } catch(...) { }
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
    auto settings = program->getSettings( emulator );
    auto& expansion = emulator->expansions[ LIBC64::Interface::ExpansionIdReu ];
    auto memoryType = expansion.memoryType;

    for(auto& memory : memoryType->memory) {
        if (memory.size == reuSize) {
            
            settings->set<unsigned>( _underscore( memoryType->name ) + "_mem", memory.id);
            
            settings->set<unsigned>( "expansion", expansion.id);
        }
    }       
}


#include "cmd.h"
#include "../view/view.h"
#include "../media/autoloader.h"
#include "../thread/emuThread.h"

auto Cmd::set(int argc, char** argv) -> void {        	
	
	std::string arg;
	
    for (unsigned i = 0; i < argc; i++) {
        
		arg = (std::string)argv[i];
		
		if ( arg == "-no-driver" ) {
			noDriver = true;
			
        } else if ( arg == "-no-gui" ) {
			noGui = true;
			noDriver = true;
			
        } else if ( arg == "-debugcart" )
            debug = true;            
        
        else if ( arg == "-h" )
            helpRequested = true;
        
        else if ( arg == "--help" )
            helpRequested = true;
        
        else if ( arg == "-v" )
            versionRequested = true;
        
        else if ( arg == "--version" )
            versionRequested = true;	
		else
			arguments.push_back( arg );
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

	struct Options {
        std::string ident;
        std::string description;
        std::string param;
    };  
	
	std::vector<Options> options;
	options.push_back({"-v, --version", "Output program version", ""});
	options.push_back({"-h, --help", "Output this help screen", ""});

	options.push_back({"-vic-6569R3", "Select VIC-II 6569R3 and PAL mode", ""});
	options.push_back({"-vic-8565", "Select VIC-II 8565 and PAL mode", ""});
	options.push_back({"-vic-6567R8", "Select VIC-II 6567R8 and NTSC mode", ""});
	options.push_back({"-vic-8562", "Select VIC-II 6562 and NTSC mode", ""});
	options.push_back({"-vic-6569R1", "Select VIC-II 6569R1 and PAL mode", ""});
	options.push_back({"-vic-6567R56A", "Select VIC-II 6567R56A and NTSC mode", ""});
	options.push_back({"-vic-6572", "Select VIC-II 6572 and PAL mode", ""});
	options.push_back({"-vic-6573", "Select VIC-II 6573 and NTSC mode with PAL Encoding", ""});

	options.push_back({"-sid-6581", "Select SID 6581", ""});
	options.push_back({"-sid-8580", "Select SID 8580", ""});

	options.push_back({"-cia-6526a", "Select CIA 6526a", ""});
	options.push_back({"-cia-6526", "Select CIA 6526", ""});

	options.push_back({"-reu", "Emulate REU Expansion", "<size in kb>"});
	options.push_back({"-georam", "Emulate GeoRam Expansion", "<size in kb>"});
	options.push_back({"-debugcart", "Generate exit codes for VICE Testbench", ""});
	options.push_back({"-limitcycles", "Specify number of cycles to run before quitting with an error (checks at complete frames)", "<cycles>"});
	options.push_back({"-exitscreenshot", "Save screen to PNG file, when exiting App", "<filePath>"});
	options.push_back({"-ane-magic", "Force CPU to use this value for ANE opcode", "<value>"});
	options.push_back({"-lax-magic", "Force CPU to use this value for LAX opcode", "<value>"});
	options.push_back({"-no-driver", "Run without video, audio, input drivers", ""});
	options.push_back({"-no-gui", "Open without graphical user interface and force -no-driver", ""});
	options.push_back({"-autostart-prg", "Set autostart mode for PRG files (1: Inject, 2: Disk image)", "<value>"});
	options.push_back({"-aggressive-fastforward", "aggressive Warp mode (emulates VIC sequencer every 15 frames only)", ""});
	options.push_back({"-fast-testbench", "analyze passed options and then decides on the use of aggressive fastforward and/or PRG memory injection", ""});
	
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
	bool georamSizeNext = false;
    bool aneMagicNext = false;
	bool laxMagicNext = false;
    bool screenshotPathNext = false;
	bool autostartPrgNext = false;
	bool fastTestbench = false;
    typedef Emulator::Interface EmuInt;
	auto emuC64 = program->getEmulator("C64");
	auto diskGroup = emuC64->getDiskMediaGroup();
	unsigned cycles = 0;
    GUIKIT::Settings* settingsC64 = program->getSettings( emuC64 );
	bool hasFuxxorTest = false;
	bool hasViciiTest = false;
	bool hasRam0001Test = false;
	bool hasDefaultTest = false;
	bool emulateD64WithMoreAccuracy = false; // use G64 emulation

    for( auto& arg : arguments ) {
        if (limitCyclesNext) {
            limitCyclesNext = false;
            cycles = getCycles( arg );
            continue;          
        }          
        
        if (reuSizeNext) {
            reuSizeNext = false;                        
            setReuSize( arg );
            continue;
        }
		
		if (georamSizeNext) {
            georamSizeNext = false;                        
            setGeoRamSize( arg );
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
        else if (arg == "-limitcycles") {
            limitCyclesNext = true;
        }
        else if (arg == "-reu") {
            reuSizeNext = true;
        }
		else if (arg == "-georam") {
            georamSizeNext = true;
        }
        else if (arg == "-ane-magic") {
            aneMagicNext = true;
        }
		else if (arg == "-lax-magic") {
            laxMagicNext = true;
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

                if (diskListing.empty() && GUIKIT::String::foundSubStr( temp, "." + suffix + ":" )) {
                    std::replace( arg.begin(), arg.end(), '\\', '/');
                    // load specific listing from disk
                    auto splits = GUIKIT::String::split(arg, ':');

                    arg = "";
                    bool nextPart = false;
                    for(auto& split : splits) {
                        std::string temp = split;
                        GUIKIT::String::toLowerCase( temp );

                        if (!nextPart) {
                            if (!arg.empty())
                                arg += ":";
                            arg += split;
                        } else {
                            if (!diskListing.empty())
                                diskListing += ":";

                            diskListing += split;
                        }

                        if (GUIKIT::String::foundSubStr( temp, "." + suffix ))
                            nextPart = true;
                    }
                    paths.push_back( arg );
                    autoload = true;
                    break;

                } else if (GUIKIT::String::foundSubStr( temp, "." + suffix )) {
                    std::replace( arg.begin(), arg.end(), '\\', '/');
										
					if (!hasViciiTest && GUIKIT::String::foundSubStr( temp, "vicii" ))
						hasViciiTest = true;
					else if (!hasViciiTest && GUIKIT::String::foundSubStr( temp, "ef_test" )) // relies on VIC last bus value
						hasViciiTest = true;
					else if (!hasViciiTest && GUIKIT::String::foundSubStr( temp, "bonzai" ))
						hasViciiTest = true;
					else if (!hasFuxxorTest && GUIKIT::String::foundSubStr( temp, "fuxxor" ))
						hasFuxxorTest = true;
					else if (!hasRam0001Test && GUIKIT::String::foundSubStr( temp, "ram0001" ))
						hasRam0001Test = true;
                    else if (!emulateD64WithMoreAccuracy && GUIKIT::String::foundSubStr( temp, "rpm3." ))
                        emulateD64WithMoreAccuracy = true;
									
					// todo: dirty hack for Testbench to prevent injection of test.prg instead of loading "test",8,1
					else if (GUIKIT::String::foundSubStr( temp, "defaults" ) && GUIKIT::String::foundSubStr( temp, "test." )) {
						if (!hasDefaultTest)
							hasDefaultTest = true;
						else {							
							paths[0] = GUIKIT::String::replace( paths[0], ".prg", ".d64");
							continue;
						}						
					}
					
                    paths.push_back( arg );  
                    autoload = true;
                    break;
                }
            }                                  
        }
    }

	if (debug) {
		dynamic_cast<LIBC64::Interface*> (emuC64)->activateDebugCart( cycles );
		globalSettings->set<bool>("video_sync", false);
        globalSettings->set<bool>("threaded_renderer", false);
		globalSettings->set<bool>("fps_limit", false);
		globalSettings->set<bool>("fps", true);
		globalSettings->set("video_screen_text", 0);
		settingsC64->set<bool>("video_cycle_accuracy", true);
        settingsC64->set<unsigned>("Stepper_Seek_Time", 90);
        settingsC64->set<unsigned>("Disalign_Tracks", 1);
		
		updateModel(emuC64, LIBC64::Interface::ModelIdDisableGreyDotBug, 0);
	}	
	
	if (noGui)
		globalSettings->set<bool>("fps", false );
	
	if (fastTestbench) {
		aggressiveFastforward = true;
		
		if (!screenshotPath.empty())
			aggressiveFastforward = false;
		
		else {
			if (hasViciiTest)
				aggressiveFastforward = false;
			
			if (hasFuxxorTest) {
				fastTestbench = false;
				aggressiveFastforward = false;
			}
				
			if (hasRam0001Test)
				aggressiveFastforward = false;
			
			if (hasDefaultTest)
				fastTestbench = false;
		}
	}
	
	if (hasFuxxorTest)
		settingsC64->set<unsigned>("memory_value", 0);

    if(emulateD64WithMoreAccuracy)
        settingsC64->set<unsigned>("Emulate_D64_More_Accurate", 1);
	
	if (fastTestbench)
		autostartPrg = 1;

    else if (autostartPrg == 2) { // load prg as d64
        // allow it only, if there is a single PRG file loaded
		
		if ( diskGroup && (paths.size() == 1)) {
			auto _path = paths[0];
		
			GUIKIT::String::toLowerCase( _path );
			
			if (GUIKIT::String::foundSubStr( _path, ".prg" ))
				diskGroup->suffix.push_back("prg");
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

    emuThread->lock();
    if (debug) {
        autoloader->init( arguments, true, Autoloader::Mode::AutoStart, 1 );
    } else {
        autoloader->init( arguments, true, Autoloader::Mode::AutoStart, 0, diskListing );
    }

    autoloader->loadFiles();

    if (activeEmulator) {
        typedef Emulator::Interface EmuInt;

        if (aggressiveFastforward)
            activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::ReduceVideoOutput | (unsigned)EmuInt::FastForward::NoVideoSequencer );
        else if (noDriver || noGui)
            activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut | (unsigned)EmuInt::FastForward::NoVideoOut );
        else if (debug)
            activeEmulator->fastForward( (unsigned)EmuInt::FastForward::NoAudioOut );
    }

    emuThread->unlock();
    
    if (!debug && !noDriver && !noGui && globalSettings->get<bool>("open_fullscreen", false)) {
        view->fullscreenOnStartUp.setEnabled();
    }

    autoload = false;
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

auto Cmd::collectAllowedSuffix() -> std::vector<std::string> {
    std::vector<std::string> allowedSuffix;
    
    for( auto emulator : emulators ) {
        for( auto& mediaGroup : emulator->mediaGroups ) {		
            
            for (auto suffix : mediaGroup.suffix) {
                
                GUIKIT::String::toLowerCase( suffix );

                if (!GUIKIT::Vector::find(allowedSuffix, suffix))
                    allowedSuffix.push_back( suffix );
            }
        }
    }
    
    allowedSuffix.push_back( "sav" );
        
    return allowedSuffix;
}

auto Cmd::getCycles(std::string arg) -> unsigned {
    
    if (!GUIKIT::String::isNumber( arg ))
        return 0;
     
    try {
        return std::stoi(arg);
    } catch (...) {}

	return 0;
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
		
    if (!GUIKIT::String::isNumber( arg ))
        return;
             
    try {
        autostartPrg = std::stoi(arg);	
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

auto Cmd::setGeoRamSize(std::string arg) -> void {
    
    if (!GUIKIT::String::isNumber( arg ))
        return;
        
    unsigned geoRamSize = 0;    
    try {
        geoRamSize = std::stoi(arg);
    } catch(...) {
        return;
    }

    auto emulator = program->getEmulator("C64");  
    auto settings = program->getSettings( emulator );
    auto& expansion = emulator->expansions[ LIBC64::Interface::ExpansionIdGeoRam ];
    auto memoryType = expansion.memoryType;

    for(auto& memory : memoryType->memory) {
        if (memory.size == geoRamSize) {
            
            settings->set<unsigned>( _underscore( memoryType->name ) + "_mem", memory.id);
            
            settings->set<unsigned>( "expansion", expansion.id);
        }
    }       
}

auto Cmd::saveExitScreenshot() -> void {        
       
    if (!activeEmulator)
        return;
    
    auto pitch = activeEmulator->cropPitch();
    auto data = activeEmulator->cropData();
    auto width = activeEmulator->cropWidth();
    auto height = activeEmulator->cropHeight();    
    
    if (!data)
        return;
   
    std::string palIdent = "Pepto PAL";
    uint32_t* colorTable = nullptr;
    
    for(auto& palette : activeEmulator->palettes) {
        
        if (palette.name == palIdent) {
            colorTable = new uint32_t[ palette.paletteColors.size() ];
            unsigned i = 0;
            
            for(auto& col : palette.paletteColors)
                colorTable[i++] = col.rgb;
            
            break;
        }
    }
    
    if (!colorTable)
        return;
    
    uint8_t* screen = new uint8_t[width * height * 3];
    uint8_t* ptr = screen;
    uint32_t color;
    
	for(unsigned h = 0; h < height; h++) {
		for(unsigned w = 0; w < width; w++) {
            
            color = colorTable[*data++ & 0xf];
            
            *ptr++ = (color >> 16) & 0xff;
            *ptr++ = (color >> 8) & 0xff;
            *ptr++ = (color >> 0) & 0xff;
        }			

		data += pitch;		
	}
    
    unsigned pngSize = 0;
    GUIKIT::Image png;
    uint8_t* pngData = png.generatePng( screen, width, height, pngSize );
    
    GUIKIT::File file;
    file.setFile( screenshotPath );
    file.open(GUIKIT::File::Mode::Write);
    file.write( pngData, pngSize );
    
    delete[] screen;
    delete[] pngData;
    delete[] colorTable;
}
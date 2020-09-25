
#include "program.h"
#include "view/view.h"
#include "config/config.h"
#include "emuconfig/config.h"
#include "media/media.h"
#include "config/archiveViewer.h"
#include "input/manager.h"
#include "tools/filesetting.h"
#include "tools/filepool.h"
#include "tools/status.h"
#include "states/states.h"
#include "audio/manager.h"
#include "firmware/manager.h"
#include "video/palette.h"
#include "cmd/cmd.h"
#include <random>

Program* program = nullptr;
DRIVER::Input* inputDriver = new DRIVER::Input;
DRIVER::Audio* audioDriver = new DRIVER::Audio;
DRIVER::Video* videoDriver = new DRIVER::Video;
std::vector<Emulator::Interface*> emulators;
Emulator::Interface* activeEmulator = nullptr;
std::vector<GUIKIT::Settings*> settingsStorage;
GUIKIT::Settings* globalSettings = nullptr;
GUIKIT::Translation* trans = nullptr;
FilePool* filePool = nullptr;
Logger* logger = nullptr;
Cmd* cmd = nullptr;
Status* status = nullptr;
std::vector<FileSetting*> FileSetting::instances = {};
VideoManager* activeVideoManager = nullptr;

#include "files.cpp"
#include "video.cpp"
#include "audio.cpp"
#include "input.cpp"

int main(int argc, char** argv) {  
    cmd = new Cmd(argc, argv);
    if (cmd->helpRequested || cmd->versionRequested) {
        cmd->printHelp();
        return 0;
    }
    
    Program program;
    GUIKIT::Application::processEvents();
    GUIKIT::Application::run();
    return GUIKIT::Application::exitCode;
}

Program::Program() {	    
    program = this;    
    GUIKIT::Application::loop = [this]() { loop(); };    
    GUIKIT::Application::name = APP_NAME;
    globalSettings = new GUIKIT::Settings;
    settingsStorage.push_back( globalSettings );
    view = new View;    
    configView = new ConfigView::TabWindow;
    archiveViewer = new ArchiveViewer;    
    trans = new GUIKIT::Translation;
    logger = new Logger;
	filePool = new FilePool(10);
	status = new Status;
    audioManager = new AudioManager;    
    
    addEmulators();
    init();	  

    InputManager::build();
    view->build();
    configView->build();
	
	for( auto mediaView : mediaViews )
        mediaView->build();
    
    for( auto emuConfigView : emuConfigViews )
        emuConfigView->build();

    archiveViewer->build();
    view->show();    
    
	initInput();
	initAudio();
	initVideo();
    
    cmd->autoloadImages();		
}

auto Program::addEmulators() -> void {
    auto emulatorC64 = new LIBC64::Interface;
    emulatorC64->bind = this;
    emulators.push_back( emulatorC64 );

    auto emulatorAmi = new LIBAMI::Interface;
    emulatorAmi->bind = this;
    emulators.push_back( emulatorAmi );

    // this manager includes only hotkeys (when emulation is inactive)
	inputManagers.push_back( new InputManager( ) );
    
    for( auto emulator : emulators ) {
        auto settings = new GUIKIT::Settings;

        settings->setGuid(dynamic_cast<void*> (emulator));

        settingsStorage.push_back( settings );
        
        // inlcudes hotkeys + emulator keys
        inputManagers.push_back( new InputManager( emulator ) );
        
        emuConfigViews.push_back( new EmuConfigView::TabWindow( emulator ) );
		
		mediaViews.push_back( new MediaView::MediaWindow( emulator ) );
        
        states.push_back( new States( emulator ) );
        
        firmwareManagers.push_back( new FirmwareManager( emulator ) );    
        
        videoManagers.push_back( new VideoManager( emulator ) );
        
        if (dynamic_cast<LIBC64::Interface*>( emulator ))
            paletteManagers.push_back( new PaletteManager( emulator ) );
    }        
}

auto Program::init() -> void {
    
    if (!cmd->debug) {
        loadSettings();

        if (!loadTranslation(globalSettings->get<std::string>("translation", getSystemLangFile()))) {
            view->message->error("language plugin not found");
        }
    }
    
    cmd->parse();    
	
    status->init();
    
    for( auto emulator : emulators ) {
        
        for( auto& connector : emulator->connectors )            
            emulator->connect( &connector, getDevice( emulator, &connector ) );       

		for (auto& model : emulator->models)
			emulator->setModel( model.id, getSettings(emulator)->get<int>( _underscore(model.name), model.defaultValue, model.range) );				
        
		updateCrop( emulator );
        
        setPalette( emulator );
        
        setExpansionSelection( emulator );
        
		setDriveSpeedAndWobble( emulator );
		
        setAccuracy( emulator );
        
        setRunAhead( emulator );
    }   
    	
	logger->setSavePath( GUIKIT::System::getUserDataFolder(appFolder()) );
        
    isRunning = isPause = false;
}

auto Program::setDriveSpeedAndWobble(Emulator::Interface* emulator) -> void {
	
    auto settings = getSettings( emulator );
    
	for(auto& mediaGroup : emulator->mediaGroups) {
		if (mediaGroup.isDisk()) {
			double wobble = settings->get<double>( _underscore(mediaGroup.name) + "_wobble", 0.5,{0.0, 5.0});
			double speed = settings->get<double>(_underscore(mediaGroup.name) + "_speed", 300.0,{275.0, 325.0});
			emulator->setDriveSpeed(&mediaGroup, speed, wobble);

		} else if (mediaGroup.isTape()) {
			bool tapeWobble = settings->get<bool>(_underscore(mediaGroup.name) + "_wobble", false);
			emulator->setDriveSpeed(&mediaGroup, 0, tapeWobble);
		}
	}
}

auto Program::setAccuracy(Emulator::Interface* emulator) -> void {
    auto settings = getSettings( emulator );
    
    emulator->videoCycleAccuracy( settings->get<bool>( "video_cycle_accuracy", true) );
    emulator->videoScanlineThread( settings->get<bool>( "video_scanline_thread", false) );
    emulator->diskHighLoadThread( settings->get<bool>( "disk_highload_thread", false) );
    emulator->diskIdle( settings->get<bool>( "disk_idle", false) );
    emulator->audioRealtimeThread( settings->get<bool>( "audio_realtime_thread", false) );
}

auto Program::power( Emulator::Interface* emulator, bool regular ) -> void {                  
    
    bool emuSwap = activeEmulator != emulator;
    powerOff();			
    
    activeEmulator = emulator;
    auto settings = getSettings( emulator );
    activeVideoManager = VideoManager::getInstance( emulator );
	uint8_t* data;
	bool needTapeControl = false;   
    std::vector<std::string> brokenPaths;
    std::vector<std::string> missingFirmware;

    emulator->setExpansion( settings->get<unsigned>("expansion", 0) );
    
    // a loaded state before could change the values internally.
    for (auto& memoryType : emulator->memoryTypes) {
        unsigned memoryId = settings->get<unsigned>( _underscore(memoryType.name) + "_mem", memoryType.defaultMemoryId);
        emulator->setMemory(&memoryType, memoryId);
    }
        
    auto expansion = emulator->getExpansion();
    
    for(auto& mediaGroup : emulator->mediaGroups) {

        if (mediaGroup.isExpansion() && (&mediaGroup != expansion->mediaGroup))            
            // allow only expansion media groups for the currently used expansion
            continue;    
        
        auto selectedMedia = mediaGroup.selected;
        
        if(mediaGroup.isDrive()) {
            unsigned counter = settings->get( _underscore(mediaGroup.name) + "_count", mediaGroup.defaultUsage());        
            emulator->setDrivesConnected( &mediaGroup, counter );
            needTapeControl |= mediaGroup.isTape() && (counter > 0);
        }                
        
        for(auto& media : mediaGroup.media) {            
            
            if (selectedMedia && !media.memoryDump && (selectedMedia != &media) )
                // only one media element at a time can be used for this group
                continue;
            
            auto fSetting = FileSetting::getInstance( emulator, _underscore( media.name ) );

            media.guid = uintptr_t(nullptr);
            GUIKIT::File* file = filePool->get( fSetting->path );
            if(!file)
                continue;

            if (!program->loadImageDataWhenOk( file, fSetting->id, &mediaGroup, data )) {	                
                if ( regular && !GUIKIT::Vector::find( brokenPaths, fSetting->path ) )
                    brokenPaths.push_back( fSetting->path );
                
                continue;
            }            
            media.guid = uintptr_t(file);
            
            emulator->insertMedium(&media, data, file->archiveDataSize(fSetting->id));
            emulator->writeProtect(&media, (file->isArchived() || file->isReadOnly()) ? true : fSetting->writeProtect);
            MediaView::MediaWindow::getView( activeEmulator )->updateWriteProtection( &media, fSetting->writeProtect );
            filePool->assign( _ident(emulator, media.name + "store"), file);	           

            States::getInstance( activeEmulator )->updateImage( fSetting, &media );

            filePool->assign( _ident(emulator, media.name), file);
            
            if (mediaGroup.isExpansion()) {
                for(auto& jumper : mediaGroup.expansion->jumpers) {
                    bool state = settings->get<bool>( _underscore(media.name + "_jumper_" + jumper.name), false );
                    emulator->setExpansionJumper( &media, jumper.id, state );
                }                
            }   
            
            if (regular)
                updateSaveIdent( &media, fSetting->file );
        }
    }
    
    missingFirmware = FirmwareManager::getInstance( activeEmulator )->insert();
    GUIKIT::Vector::combine( brokenPaths, missingFirmware );
    
    showOpenError( brokenPaths );
    
    filePool->unloadOrphaned();
	
    audioManager->power();
    renderPlaceholder(true);
    
    if (emuSwap)
		setVideoFilter();	
	
    activeEmulator->power();
    resetRunAhead();
    isRunning = true;
	isPause = false;
	
	archiveViewer->setVisible(false);
	view->update();	
    view->setCursor( activeEmulator );
    view->updateFreeze( activeEmulator );

	if (needTapeControl)
		view->showTapeMenu( true );
    // a few emulation units generate random values
    // srand spreads a new seed for better randomness
    srand( time( NULL ) );
    
    globalSettings->set("last_used_emu", activeEmulator->ident);
    
    activeVideoManager->initFpsLimit();
}

auto Program::reset( Emulator::Interface* emulator ) -> void {
	if (!isRunning) {
		power(emulator);
		return;
	}
		
	emulator->reset();
}

auto Program::powerOff() -> void {    
    
    if ( activeEmulator ) {
        fastForward( false );
        activeEmulator->powerOff();
        
        for(auto& mediaGroup : activeEmulator->mediaGroups) {
            for(auto& media : mediaGroup.media) {
                
                if (media.guid) {
                    auto file = (GUIKIT::File*)media.guid;
                    // medium was written by emulation, lets update the listing
                    if (file->wasDataChanged() && filePool->has( _ident(activeEmulator, media.name + "store"), file))                        
                        MediaView::MediaWindow::getView( activeEmulator )->updateListing( &media );
                }                        
                
                filePool->assign( _ident(activeEmulator, media.name), nullptr);
                activeEmulator->ejectMedium( &media );
                States::getInstance( activeEmulator )->updateImage( nullptr, &media );
            }				
        }
		activeEmulator->unsetExpansion();
	}
	isRunning = false;
	
	view->showTapeMenu( false );    
	
	status->init(true);
    if (activeVideoManager)
        activeVideoManager->powerOff();
	videoDriver->clear();
	videoDriver->hintExclusiveFullscreen( false );
	audioDriver->clear();  
    audioManager->record.finish();
    activeEmulator = nullptr;
    activeVideoManager = nullptr;
    filePool->unloadOrphaned();
    view->updateFreeze(nullptr);
    updateSaveIdent( nullptr );
    InputManager::urgentUpdate = true;
    InputManager::resetJit();
}

auto Program::loop() -> void {
    if( willPoll() )
        InputManager::poll();
	
	if( willRun() )
        activeEmulator->run();            
	else {
        if (GUIKIT::Application::exitCode)
            return view->onClose();
        
		audioDriver->clear();
		GUIKIT::System::sleep( 20 );
		VideoManager::updateWhenNotRunning();
	}
    status->show();
}

auto Program::willPoll() -> bool {
    isFocused = view->focused();
    
    if( isFocused || configView->focused() )
        return true;
    
    for(auto mediaView : mediaViews)
        if (mediaView->focused())
            return true;
	
	for(auto emuConfigView : emuConfigViews)
        if (emuConfigView->focused())
            return true;
    
    return false;
}

auto Program::willRun() -> bool {
	static auto pauseFocusLoss = globalSettings->getOrInit("pause_focus_loss", false);
	
	if (!isRunning || isPause) return false;
	if (isFocused) return true;
	//no focus
	if (*pauseFocusLoss) return false;
	if (view->exclusiveFullscreen()) return false; //exclusive fullscreen can't run in background	
	
	return true;
}

auto Program::quit() -> void {
    powerOff();

    if (!cmd->debug && globalSettings->get<bool>("save_settings_on_exit", true) ) {
		saveSettings();
    }
    
	for(auto inputManager : inputManagers)
		delete inputManager;
    for(auto firmwareManager : firmwareManagers)
        delete firmwareManager;
    for( auto paletteManager : paletteManagers )
        delete paletteManager;
        
    delete inputDriver;
    delete audioDriver;
    delete videoDriver;
    delete trans;    
	delete logger;
	delete filePool;
    delete cmd;
    
    for(auto settings : settingsStorage)
        delete settings;
    
    // in case of exit request from emulation core
    status->update = false;
    GUIKIT::Application::loop = nullptr;
}

auto Program::loadTranslation(std::string file) -> bool {
        
    if (trans->read( translationFolder() + file )) return true;

    if (file != DEFAULT_TRANS_FILE) {
        if (trans->read( translationFolder() + DEFAULT_TRANS_FILE )) {
            globalSettings->set<std::string>("translation", DEFAULT_TRANS_FILE);
            return true;
        }
    }
    return false;
}

auto Program::getSystemLangFile() -> std::string {
    
    auto lang = GUIKIT::System::getOSLang();
    
    if (lang == GUIKIT::System::Language::DE)
        return "german.txt";
    
    if (lang == GUIKIT::System::Language::US)
        return "english.txt";
    
    if (lang == GUIKIT::System::Language::FR)
        return "french.txt";

    return DEFAULT_TRANS_FILE;
}

auto Program::translationFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + TRANSLATION_FOLDER;
}

auto Program::dataFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + DATA_FOLDER;
}

auto Program::fontFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + FONT_FOLDER;
}

auto Program::settingsFile( std::string ident ) -> std::string {

	return GUIKIT::System::getUserDataFolder(appFolder()) + ident + SETTINGS_FILE;
} 

auto Program::shaderFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + SHADER_FOLDER;
}

auto Program::imgFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + IMG_FOLDER;
}

auto Program::log(std::string data, bool newLine) -> void {
	logger->log(data, newLine);
}

auto Program::exit(int code) -> void {
    GUIKIT::Application::exitCode = code;

    if (!cmd->screenshotPath.empty())
        saveExitScreenshot();
    
    if (isRunning)
        view->onClose();
}

auto Program::updateDriveState(Emulator::Interface::Media* media, unsigned mode, unsigned track) -> void {
	status->updateDriveState(media, mode, track);
}

auto Program::appFolder() -> std::string {
	std::string _appFolder = APP_NAME;
	return GUIKIT::String::toLowerCase( _appFolder );
}

auto Program::questionToWrite(Emulator::Interface::Media* media) -> bool {
    
    return view->questionToWrite(media);
}

auto Program::getLastUsedEmu() -> Emulator::Interface* {
	
	auto ident = globalSettings->get<std::string>("last_used_emu", "");
	Emulator::Interface* defaultEmu = nullptr;

	for (auto emulator : emulators) {

		if (dynamic_cast<LIBC64::Interface*>(emulator))
            defaultEmu = emulator;
		
		if (emulator->ident == ident)
			return emulator;		
	}
	
	return defaultEmu;
}

auto Program::getEmulator( std::string ident ) -> Emulator::Interface* {

    for( auto emulator : emulators ) {

        if (emulator->ident == ident) 
            return emulator;            
    }
    
    return nullptr;
}

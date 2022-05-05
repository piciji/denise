
#include "program.h"
#include "view/view.h"
#include "config/config.h"
#include "emuconfig/config.h"
#include "config/archiveViewer.h"
#include "input/manager.h"
#include "tools/filesetting.h"
#include "tools/filepool.h"
#include "view/status.h"
#include "states/states.h"
#include "audio/manager.h"
#include "firmware/manager.h"
#include "video/palette.h"
#include "cmd/cmd.h"
#include "media/autoloader.h"
#include "media/fileloader.h"
#include "thread/emuThread.h"
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
std::vector<FileSetting*> FileSetting::instances = {};
VideoManager* activeVideoManager = nullptr;
bool Program::focused = false;

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
    GUIKIT::Application::name = APP_NAME;
    globalSettings = new GUIKIT::Settings;
	autoloader = new Autoloader;
	fileloader = new Fileloader;
    emuThread = new EmuThread;
    settingsStorage.push_back( globalSettings );
	if(!cmd->noGui) {
		view = new View;
		archiveViewer = new ArchiveViewer;    
		trans = new GUIKIT::Translation;		
		statusHandler = new StatusHandler;
		audioManager = new AudioManager;
	}    
    
	logger = new Logger;
	filePool = new FilePool(10);

    addEmulators();
    init();

	if(!cmd->noGui) {
		InputManager::build();
		view->build();		
		if (view->setVisible()) {			
			view->updateViewport();
			finishStartup();
		}
    } else {
        finishStartup();
    }
}

auto Program::finishStartup() -> void {
    initInput();
    initAudio();
    initVideo();

    cmd->autoloadImages();
	initUserInterface();
}

auto Program::initUserInterface() -> void {
    if (GUIKIT::Application::isQuit)
        return;

    bool threadedEmu = globalSettings->get<bool>("threaded_emu", false);

    if (cmd->noGui) {
		emuThread->enable( false );
        GUIKIT::Application::loop = [this]() { loopNoGui(); };
    } else if (!threadedEmu) {
		emuThread->enable( false );
        GUIKIT::Application::loop = [this]() { loop(); };
    } else {
        videoDriver->freeContext();
		GUIKIT::Application::loop = [this]() { loopUserInterface(); };
		emuThread->enable( true );
    }
}

auto Program::addEmulators() -> void {
    auto emulatorC64 = new LIBC64::Interface;
    emulatorC64->bind = this;
    emulators.push_back( emulatorC64 );

    auto emulatorAmi = new LIBAMI::Interface;
    emulatorAmi->bind = this;
    emulators.push_back( emulatorAmi );

    // this manager includes only hotkeys (when emulation is inactive)
	if (!cmd->noGui)
		inputManagers.push_back( new InputManager( ) );
    
    for( auto emulator : emulators ) {
        auto settings = new GUIKIT::Settings;

        settings->setGuid(dynamic_cast<void*> (emulator));

        settingsStorage.push_back( settings );
		
		if (cmd->noGui)
			continue;
        
        // inlcudes hotkeys + emulator keys
        inputManagers.push_back( new InputManager( emulator ) );

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
			if (view)
				view->message->error("language plugin not found");
        }		
    }
    
    cmd->parse();
    
    for( auto emulator : emulators )        
        initEmulator( emulator );
    	
	logger->setSavePath( GUIKIT::System::getUserDataFolder(appFolder()) );

	if (!cmd->debug)
        addCustomFont();
        
    isRunning = isPause = false;
}

auto Program::initEmulator( Emulator::Interface* emulator ) -> void {
    auto _settings = getSettings(emulator);

    setJit(emulator);

    for (auto& connector : emulator->connectors)
        emulator->connect(&connector, getDevice(emulator, &connector));
    
    for (auto& model : emulator->models)
        emulator->setModelValue( model.id, _settings->get<int>( _underscore(model.name), model.defaultValue, model.range) );
    
    updateCrop( emulator );

    setPalette( emulator );
    
    setExpansionSelection( emulator );

    setRunAhead( emulator );

    if (dynamic_cast<LIBC64::Interface*>( emulator ))
        setMemoryPattern( emulator );
}

auto Program::setMemoryPattern(Emulator::Interface* emulator) -> void {
    auto settings = getSettings( emulator );
    
    uint8_t value = settings->get<unsigned>("memory_value", 255);
    unsigned invertEvery = settings->get<unsigned>("memory_invert_every", 64);
    unsigned randomPatternLength = settings->get<unsigned>("memory_random_pattern", 1);
    unsigned repeatRandomPattern = settings->get<unsigned>("memory_random_repeat", 256);
    unsigned randomChance = settings->get<unsigned>("random_chance", 0);

    emulator->setMemoryInitParams( value, invertEvery, randomPatternLength, repeatRandomPattern, randomChance );
}

auto Program::power( Emulator::Interface* emulator, bool regular ) -> void {
    bool emuSwap = activeEmulator != emulator;
    powerOff();
    
    activeEmulator = emulator;
    auto settings = getSettings( emulator );
    activeVideoManager = VideoManager::getInstance( emulator );
    activeVideoManager->updateCrtThreads();
	uint8_t* data;
    std::vector<std::string> brokenPaths;

    emulator->setExpansion( settings->get<unsigned>("expansion", 0) );
    
    // a loaded state before could change the values internally.
    for (auto& memoryType : emulator->memoryTypes) {
        unsigned memoryId = settings->get<unsigned>( _underscore(memoryType.name) + "_mem", memoryType.defaultMemoryId);
        emulator->setMemory(&memoryType, memoryId);
    }
        
    auto expansion = emulator->getExpansion();
    
    for(auto& mediaGroup : emulator->mediaGroups) {

        if (mediaGroup.isExpansion() && (&mediaGroup != expansion->mediaGroup) && (&mediaGroup != expansion->mediaGroupExpanded) )
            // allow only expansion media groups for the currently used expansion
            continue;

        bool IPMode = mediaGroup.isExpansion() && mediaGroup.expansion->isRS232();

        auto selectedMedia = mediaGroup.selected;

        for(auto& media : mediaGroup.media) {            
            
            if (selectedMedia && !media.secondary && (selectedMedia != &media) )
                // only one media element at a time can be used for this group
                continue;
            
            auto fSetting = FileSetting::getInstance( emulator, _underscore( media.name ) );

            media.guid = (uintptr_t)(nullptr);

            if (!IPMode) {
                GUIKIT::File *file = filePool->get(fSetting->path);
                if (!file)
                    continue;

                if (!program->loadImageDataWhenOk(file, fSetting->id, &mediaGroup, data)) {
                    if (regular && !GUIKIT::Vector::find(brokenPaths, fSetting->path))
                        brokenPaths.push_back(fSetting->path);

                    continue;
                }
                media.guid = uintptr_t(file);

                emulator->insertMedium(&media, data, file->archiveDataSize(fSetting->id));
                emulator->writeProtect(&media, (file->isArchived() || file->isReadOnly()) ? true : fSetting->writeProtect);

                auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
                if (emuView && emuView->mediaLayout)
                    emuView->mediaLayout->updateWriteProtection( &media, fSetting->writeProtect );

                if (!mediaGroup.isProgram()) {
                    filePool->assign(_ident(emulator, media.name + "store"), file);
                    filePool->assign(_ident(emulator, media.name), file);
                }

            } else { // IP socket mode
                program->prepareSocket( &media, emulator, fSetting->path );
            }

            if (!cmd->noGui)
                States::getInstance( activeEmulator )->updateImage( fSetting, &media );

            if (mediaGroup.isExpansion()) {
                for(auto& jumper : mediaGroup.expansion->jumpers) {
                    bool state = settings->get<bool>( _underscore(media.name + "_jumper_" + jumper.name), jumper.defaultState );
                    emulator->setExpansionJumper( &media, jumper.id, state );
                }
            }
            
            if (regular && !IPMode)
                updateSaveIdent( &media, fSetting->file );
        }
    }
    
	if (!cmd->noGui) {
		FirmwareManager::getInstance( activeEmulator )->insert();

		showOpenError( brokenPaths );

		filePool->unloadOrphaned();

		audioManager->power();
		view->renderPlaceholder(true);

		if (emuSwap)
			setVideoFilter();

        //setVideoSynchronize();

		resetRunAhead();

		archiveViewer->setVisible(false);
		view->setCursor( activeEmulator );
		view->updateCartButtons( activeEmulator );

        view->showSpeedMenu();
		if (emulator->getModelValue( emulator->getModelIdOfEnabledDrives( emulator->getTapeMediaGroup() ) ) )
			view->showTapeMenu( true );
		// a few emulation units generate random values
		// srand spreads a new seed for better randomness
		srand( time( NULL ) );

		globalSettings->set("last_used_emu", activeEmulator->ident);

        statusHandler->resetFrameCounter();

        view->updateSpeedLabels();
	}
	
	activeEmulator->power();
	isRunning = true;
	isPause = false;

    hintExclusiveFullscreen( );
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
                    if (!cmd->noGui && file->wasDataChanged() && filePool->has( _ident(activeEmulator, media.name + "store"), file)) {
                        auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
                        if (emuView && emuView->mediaLayout)
                            emuView->mediaLayout->updateListing( &media );
                    }
                }                        
                
                filePool->assign( _ident(activeEmulator, media.name), nullptr);
                activeEmulator->ejectMedium( &media );
				
				if (!cmd->noGui)
					States::getInstance( activeEmulator )->updateImage( nullptr, &media );
            }				
        }
		activeEmulator->unsetExpansion();
        if (auto inputManager = InputManager::getManager(activeEmulator))
            inputManager->resetTouchlessAutofire();
	}
	isRunning = false;
    isPause = false;
	
	if (!cmd->noGui) {
        view->updatePauseCheck();
		view->showTapeMenu( false );
        view->showSpeedMenu( false );
        emuThread->clearEvents();
		statusHandler->clear();
		if (activeVideoManager)
			activeVideoManager->powerOff();
		videoDriver->clear();
        videoDriver->hintExclusiveFullscreen( false );
		audioDriver->clear();
		audioManager->powerOff();
		filePool->unloadOrphaned();
		view->updateCartButtons(nullptr);
		updateSaveIdent( nullptr );
		InputManager::urgentUpdate = true;
		InputManager::resetJit();
	}

    activeEmulator = nullptr;
    activeVideoManager = nullptr;
    warp.enableAutoWarp = false;
}

auto Program::loopNoGui() -> void {

	while (isRunning)
		activeEmulator->run();
}

auto Program::loop() -> void {
    if (!emuThread->enabled) {
        focused = view->focused();
    }

    InputManager::poll();

	if( willRun() ){
		unsigned frames = loopFrames;
		
		if (frames) {
			while(frames--) {
				if (activeEmulator)
					activeEmulator->run();
				else
					break;
			}
		} else
			activeEmulator->run();
	}
	else {
        if (GUIKIT::Application::exitCode)
            return view->onClose();
        
		audioDriver->clear();
		GUIKIT::System::sleep( 20 );
		VideoManager::updateWhenNotRunning();
	}

    if (statusHandler->hasUpdates())
        statusHandler->update();
}
// when emu thread is active
auto Program::loopUserInterface() -> void {
    GUIKIT::System::sleep( 1 );
    focused = view->focused();
    emuThread->handleStatusUpdate();
    emuThread->handleUIEvents();
}

auto Program::willRun() -> bool {
	static auto pauseFocusLoss = globalSettings->getOrInit("pause_focus_loss", false);
	
	if (!isRunning || isPause) return false;
	if (focused) return true;
	//no focus
	if (*pauseFocusLoss) return false;
	if (videoDriver && videoDriver->hasExclusiveFullscreen()) return false; //exclusive fullscreen can't run in background
	
	return true;
}

auto Program::hasFocus() -> bool {
    if( focused || (configView && configView->focused()) )
        return true;

    for(auto emuView : emuConfigViews)
        if (emuView->focused())
            return true;

    return false;
}

auto Program::quit() -> void {
    emuThread->lock();
    powerOff();
    if (statusHandler)
        statusHandler->clearUpdates();
    emuThread->unlock();
    delete emuThread;

    if (!cmd->debug) {
        if (globalSettings->get<bool>("save_settings_on_exit", true))
		    saveSettings( true );
        else
            forceSavingSomeGlobalSettings();
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
	if (trans)
		delete trans;    
	delete logger;
	delete filePool;
    delete cmd;
	delete autoloader;
    
    for(auto settings : settingsStorage)
        delete settings;

    // don't deinitialize "global settings", because of APP shutdown may trigger Window resizing which needs info from global settings
    
    // in case of exit request from emulation core
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

auto Program::settingsFileFromEmuFolder( std::string ident ) -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + "settings/" + ident + SETTINGS_FILE;
}

auto Program::shaderFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + SHADER_FOLDER;
}

auto Program::imgFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + IMG_FOLDER;
}

auto Program::soundFolder() -> std::string {
    return GUIKIT::System::getResourceFolder(appFolder()) + SOUND_FOLDER;
}

auto Program::log(std::string data, bool newLine) -> void {
	logger->log(data, newLine);
}

auto Program::exit(int code) -> void {
    GUIKIT::Application::exitCode = code;

    if (!cmd->screenshotPath.empty())
        cmd->saveExitScreenshot();
    
	if (cmd->noGui) {
		program->quit();
        GUIKIT::Application::quit();
			
	} else if (isRunning)
        view->onClose();
}

auto Program::updateDeviceState( Emulator::Interface::Media* media, bool write, unsigned position, bool LED, bool motorOff ) -> void {
	if (!media || cmd->noGui)
		return;

	statusHandler->updateDeviceState( media, write, position, LED, motorOff );
}

auto Program::appFolder() -> std::string {
	std::string _appFolder = APP_NAME;
	return GUIKIT::String::toLowerCase( _appFolder );
}

auto Program::questionToWrite(Emulator::Interface::Media* media) -> bool {
    
	if (!view)
		return false;
	
    return view->questionToWrite(media);
}

auto Program::autoStartFinish(bool soft) -> void {
    if (!activeEmulator || !warp.enableAutoWarp || !warp.active)
        return;

    if (soft && warp.motorControlled)
        return;

    videoDriver->freeContext();
    fastForward( false );
}

auto Program::informDriveLoading(bool state) -> void {

    if (!activeEmulator || !warp.enableAutoWarp || !warp.motorControlled)
        return;

    if (warp.active == state)
        return;

    videoDriver->freeContext();
    fastForward( state, warp.aggressive );
}

auto Program::initAutoWarp(Emulator::Interface::MediaGroup* mediaGroup) -> void {
    if (!activeEmulator)
        return;

    unsigned _autoWarp = program->getSettings( activeEmulator )->get<unsigned>("auto_warp", 0);

    warp.enableAutoWarp = _autoWarp != 0;

    if (warp.enableAutoWarp) {
        if (mediaGroup->isDisk())
            warp.motorControlled = !program->getSettings( activeEmulator )->get<bool>("auto_warp_disk_first_file", true);
        else
            warp.motorControlled = !program->getSettings( activeEmulator )->get<bool>("auto_warp_tape_first_file", false);

        fastForward(true, _autoWarp == 2);
    }
}

auto Program::jam( Emulator::Interface::Media* media ) -> void {

    if (cmd->noGui)
        return;

    std::string out = "CPU Jam";
    if (media)
        out = "CPU " + media->name + " Jam";

    statusHandler->setMessage( out, 10, true );
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

auto Program::setThreadPriority( Emulator::Interface::ThreadPriority priority, float minProcessingTimeInMilliSeconds, float maxProcessingTimeInMilliSeconds) -> bool {

    bool result = GUIKIT::ThreadPriority::setPriority( (GUIKIT::ThreadPriority::Mode)priority, minProcessingTimeInMilliSeconds, maxProcessingTimeInMilliSeconds );

    if (!result) {
        if (priority == Emulator::Interface::ThreadPriority::Realtime)
            result = GUIKIT::ThreadPriority::setPriority(GUIKIT::ThreadPriority::Mode::High);
    }

    return result;
}

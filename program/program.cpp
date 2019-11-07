
#include "program.h"
#include "view/view.h"
#include "config/config.h"
#include "emuconfig/config.h"
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
GUIKIT::Settings* settings = nullptr;
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
    Program program(argc, argv);
    GUIKIT::Application::processEvents();
    GUIKIT::Application::run();
    return GUIKIT::Application::exitCode;
}

Program::Program(int& argc, char** argv) {	  	
    program = this;    
    GUIKIT::Application::loop = [this]() { loop(); };    
    GUIKIT::Application::name = APP_NAME;        
    view = new View;    
    configView = new ConfigView::TabWindow;
    archiveViewer = new ArchiveViewer;
    settings = new GUIKIT::Settings;
    trans = new GUIKIT::Translation;
    logger = new Logger;
	filePool = new FilePool(10);
	status = new Status;
    audioManager = new AudioManager;
    cmd = new Cmd(argc, argv);
    
    addEmulators();
    init();	  

    InputManager::build();
    view->build();
    configView->build();
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
        // inlcudes hotkeys + emulator keys
        inputManagers.push_back( new InputManager( emulator ) );
        
        emuConfigViews.push_back( new EmuConfigView::TabWindow( emulator ) );
        
        states.push_back( new States( emulator ) );
        
        firmwareManagers.push_back( new FirmwareManager( emulator ) );    
        
        videoManagers.push_back( new VideoManager( emulator ) );
        
        if (dynamic_cast<LIBC64::Interface*>( emulator ))
            paletteManagers.push_back( new PaletteManager( emulator ) );
    }    
}

auto Program::init() -> void {
    
    if (!cmd->debug) {
        settings->load(settingsFile());

        if (!loadTranslation(settings->get<std::string>("translation", getSystemLangFile()))) {
            view->message->error("language plugin not found");
        }
    }
    
    cmd->parse();    
	
    status->init();
    
    for( auto emulator : emulators ) {
        
        for( auto& connector : emulator->connectors )            
            emulator->connect( &connector, getDevice( emulator, &connector ) );       

		for (auto& feature : emulator->features)
			emulator->setFeature( feature.id, settings->get<int>( ident(emulator, feature.name), feature.defaultValue, feature.range) );				
		updateCrop( emulator );
        
        setPalette( emulator );
        
        setExpansionSelection( emulator );
    }   
    	
	logger->setSavePath( GUIKIT::System::getUserDataFolder(appFolder()) );
        
    isRunning = isPause = false;
}

auto Program::power( Emulator::Interface* emulator, bool showImageError ) -> void {    
    bool emuSwap = activeEmulator != emulator;
    powerOff();			
    
    activeEmulator = emulator;
    activeVideoManager = VideoManager::getInstance( emulator );
	uint8_t* data;
	bool needTapeControl = false;   
    std::vector<std::string> brokenPaths;
    std::vector<std::string> missingFirmware;

    emulator->setExpansion( settings->get<unsigned>(ident(emulator, "expansion"), 0) );
    
    // we need to update memory, cpu, chipset every power cycle.
    // a loaded state before could change the values internally.
    for (auto& memoryType : emulator->memoryTypes) {
        unsigned memoryId = settings->get<unsigned>(ident(emulator, memoryType.name + "_mem"), memoryType.defaultMemoryId);
        emulator->setMemory(&memoryType, memoryId);
    }

    emulator->setChipset(settings->get<unsigned>(ident(emulator, "chipset"), 0));
    emulator->setCpu(settings->get(ident(emulator, "cpu"), 0));
        
    auto expansion = emulator->getExpansion();
    
    for(auto& mediaGroup : emulator->mediaGroups) {

        if (mediaGroup.isExpansion() && (&mediaGroup != expansion->mediaGroup))            
            // allow only expansion media groups for the currently used expansion
            continue;    
        
        auto selectedMedia = mediaGroup.selected;
        
        if(mediaGroup.isDrive()) {
            unsigned counter = settings->get( ident(emulator, mediaGroup.name + "_count"), mediaGroup.defaultUsage());        
            emulator->setDrivesConnected( &mediaGroup, counter );
            needTapeControl |= mediaGroup.isTape() && (counter > 0);
        }                
        
        for(auto& media : mediaGroup.media) {            

            if (mediaGroup.isMemory() && media.expansion && (media.expansion != expansion))
                // this memory dump belongs to an expansion, which is not in use this time
                continue; 
            
            if (selectedMedia && (selectedMedia != &media) )
                // only one media element at a time can be used for this group
                continue;
            
            auto setting = FileSetting::getInstance( ident(emulator, media.name) );

            media.guid = uintptr_t(nullptr);
            GUIKIT::File* file = filePool->get( setting->path );
            if(!file)
                continue;

            if (!program->loadImageDataWhenOk( file, setting->id, &mediaGroup, data )) {	                
                if ( showImageError && !GUIKIT::Vector::find( brokenPaths, setting->path ) )
                    brokenPaths.push_back( setting->path );
                
                continue;
            }            
            media.guid = uintptr_t(file);
            
            emulator->insertMedium(&media, data, file->archiveDataSize(setting->id));
            emulator->writeProtect(&media, file->isArchived() ? true : setting->writeProtect);
            filePool->assign(ident(emulator, media.name + "store"), file);	           

            States::getInstance( activeEmulator )->updateImage( setting, &media );

            filePool->assign(ident(emulator, media.name), file);
        }
    }
    
    missingFirmware = FirmwareManager::getInstance( activeEmulator )->insert();
    GUIKIT::Vector::combine( brokenPaths, missingFirmware );
    
    showOpenError( brokenPaths );
    
    filePool->unloadOrphaned();
    
    emulator->setRegion( (Emulator::Interface::Region) settings->get<unsigned>( ident(emulator, "video_region"), 0u, {0u, 1u}) );
    audioManager->power();
    
    if (emuSwap)
        activeVideoManager->shader.recreate = true;
    activeEmulator->power();
    isRunning = true;
	isPause = false;
	
	archiveViewer->setVisible(false);
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->update();	
	view->update();	
    view->setCursor( activeEmulator );
    view->updateFreeze( activeEmulator );

	if (needTapeControl)
		view->showTapeMenu( true );
    // a few emulation units generate random values
    // srand spreads a new seed for better randomness
    srand( time( NULL ) );
    
    settings->set("last_used_emu", activeEmulator->ident);
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
        activeEmulator->powerOff();
        
        for(auto& mediaGroup : activeEmulator->mediaGroups) {
            for(auto& media : mediaGroup.media) {
                
                if (media.guid) {
                    auto file = (GUIKIT::File*)media.guid;
                    // medium was written by emulation, lets update the listing
                    if (file->wasDataChanged() && filePool->has( ident(activeEmulator, media.name + "store"), file))                        
                        EmuConfigView::TabWindow::getView( activeEmulator )->mediaLayout->updateListing( &media );
                }                        
                
                filePool->assign( ident(activeEmulator, media.name), nullptr);
                activeEmulator->ejectMedium( &media );
                States::getInstance( activeEmulator )->updateImage( nullptr, &media );
            }				
        }
		activeEmulator->unsetExpansion();
	}
	isRunning = false;
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->update();
	
	view->showTapeMenu( false );
    view->setDefaultCursor();
	
	status->init(true);
    if (activeVideoManager)
        activeVideoManager->powerOff();
	videoDriver->clear();
	videoDriver->hintExclusiveFullscreen( false );
	audioDriver->clear();    
    activeEmulator = nullptr;
    activeVideoManager = nullptr;
    filePool->unloadOrphaned();
    view->updateFreeze(nullptr);
}

auto Program::loop() -> void {
    if( willPoll() ) InputManager::poll();
	
	if( willRun() ) activeEmulator->run();
	else {
        if (GUIKIT::Application::exitCode)
            return view->onClose();
        
		audioDriver->clear();
		GUIKIT::System::sleep( 20 );
	}
    status->show();
}

auto Program::willPoll() -> bool {
    isFocused = view->focused();
    
    if( isFocused || configView->focused() )
        return true;
    
    for(auto emuConfigView : emuConfigViews)
        if (emuConfigView->focused())
            return true;
    
    return false;
}

auto Program::willRun() -> bool {
	static auto pauseFocusLoss = settings->getOrInit("pause_focus_loss", false);
	
	if (!isRunning || isPause) return false;
	if (isFocused) return true;
	//no focus
	if (*pauseFocusLoss) return false;
	if (view->exclusiveFullscreen()) return false; //exclusive fullscreen can't run in background	
	
	return true;
}

auto Program::quit() -> void {
    powerOff();

    if (!cmd->debug && settings->get<bool>("save_settings_on_exit", true) ) {
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
    delete settings;
	delete logger;
	delete filePool;
    delete cmd;
    // in case of exit request from emulation core
    status->update = false;
    GUIKIT::Application::loop = nullptr;
}

auto Program::loadTranslation(std::string file) -> bool {
        
    if (trans->read( translationFolder() + file )) return true;

    if (file != DEFAULT_TRANS_FILE) {
        if (trans->read( translationFolder() + DEFAULT_TRANS_FILE )) {
            settings->set<std::string>("translation", DEFAULT_TRANS_FILE);
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

auto Program::settingsFile() -> std::string {
	return GUIKIT::System::getUserDataFolder(appFolder()) + SETTINGS_FILE;
} 

auto Program::log(std::string data, bool newLine) -> void {
	logger->log(data, newLine);
}

auto Program::exit(int code) -> void {
    GUIKIT::Application::exitCode = code;
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

auto Program::ident( Emulator::Interface* emulator, std::string name ) -> std::string {
	std::string _ident = emulator->ident;
    return GUIKIT::String::toLowerCase(_ident) + "_" + GUIKIT::String::replace(name, " ", "_");
}

auto Program::saveSettings() -> void {
	
	bool success = settings->save(settingsFile());
	
	if (!success)
		view->message->warning(trans->get("cfg_not_save",{{"%path%", settingsFile()}}));

}

auto Program::rememberNotToSaveSettings() -> void {
	GUIKIT::Settings tempSettings;
	
	if (!tempSettings.load( settingsFile() ))
		return;
	
	tempSettings.set<bool>("save_settings_on_exit", false);
	
	tempSettings.save( settingsFile() );
}

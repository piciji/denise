
#include "states.h"
#include "../firmware/manager.h"
#include "../view/status.h"
#include "../audio/manager.h"

std::vector<States*> states;

States::States(Emulator::Interface* emulator) {
    saveSettings = new GUIKIT::Settings;
    this->emulator = emulator;
    settings = program->getSettings(emulator);
}

auto States::load( std::string path, bool prependFolder ) -> void {

    if (path == "")
        path = generateAutoPath();
    else if (prependFolder)
        path = statesFolder() + path;
        
    GUIKIT::File file( path );

    if ( !file.open( GUIKIT::File::Mode::Read ) )
        return statusMessage( "state_error_load", file.getFileName() );

    auto data = file.read();
    if (data == nullptr || file.getSize() == 0)
        return statusMessage( "state_error_load", file.getFileName() );

    if (!emulator->checkstate(data, file.getSize())) 
        return statusMessage( "state_incompatible", file.getFileName() );
    
    GUIKIT::Settings loadSettings;    
    bool imageFileLoaded = loadSettings.load( path + ".images" );
            
    if (forcePowerNextLoad || !activeEmulator || (activeEmulator != emulator) )
        program->power( emulator, !imageFileLoaded );      
    
    errorPaths.clear();
    
    std::vector<Emulator::Interface::Media*> loadedMedia;
    if (imageFileLoaded) {
        loadedMedia = loadImagePaths( &loadSettings );        
        loadFirmwarePaths( &loadSettings );
    }
        
    program->showOpenError( errorPaths, true );
	
    emulator->loadstate( data, file.getSize() );

    updateWriteProtection( loadedMedia );
	
    updateModels();

    updateConnectedDevices();
    
    updateTapeMenu();    
    
    updateExpansionJumper();

    view->updateCartButtons( emulator );
    
    InputManager::resetJit();
    
    forcePowerNextLoad = false;

    program->warp.enableAutoWarp = false;
}

auto States::save( std::string path, bool prependFolder ) -> void {

    if (path == "")
        path = generateAutoPath();
    else if (prependFolder)
        path = statesFolder() + path;
    
    if (!activeEmulator || (activeEmulator != emulator))
        return;

    unsigned size = 0;
    uint8_t* data = nullptr;
    GUIKIT::File file( path );
    std::string langKey = "state_saved";

    data = emulator->savestate( size );

    if (!data)
        langKey = "state_error_save";
    else {            
        if (!file.open(GUIKIT::File::Mode::Write, true) || !file.write(data, size))
            langKey = "state_error_save";
        else {
            if ( !saveImagePaths( path + ".images" ) ) {
                // don't inform the user. it could confuse him.
                // it's unlikely the state file was saved but the path file didn't.
            }
            // remember emu which generates latest savestate.
            // while emulation is off and a state will be loaded from hotkeys
            // the emu which generated the last state will be used.
            globalSettings->set("fast_save_emu", emulator->ident);
        }                            
    }            

    statusMessage( langKey, file.getFileName() );    
}    

auto States::loadFirmwarePaths( GUIKIT::Settings* loadSettings ) -> void {

    auto firmwareManager = FirmwareManager::getInstance( emulator );
    auto setting = new FileSetting( loadSettings );

    firmwareManager->missingFirmware.clear();
    
    for( auto& firmware : emulator->firmwares ) {
        
        setting->ident = firmware.name;
        setting->update();

        InsertFirmware* inserted = findFirmware( &firmware );

        if (inserted) {
            if ((inserted->setting->path == setting->path)
                && (inserted->setting->id == setting->id)) {
                // ok, already inserted
                continue;
            }
        }

        // savestate was generated with different firmware
        // use a store level not used for preconfigured firmware sets
        unsigned storeLevel = firmwareManager->maxSets + 10;
        
        FileSetting* storeSetting = firmwareManager->getSetting( &firmware, storeLevel );
        storeSetting->id = setting->id;
        storeSetting->path = setting->path;
        storeSetting->setSaveable( false );

        if (firmwareManager->loadImage( &firmware, storeLevel ))
            firmwareManager->useImage( &firmware, storeLevel );
        else {
            // load default firmware if
            // 1. no custom firmware was submitted in savestate
            // 2. path of custom firmware is not working anymore
            firmwareManager->insertFirmware(&firmware, 0);
        }
    }

    if (!firmwareManager->missingFirmware.empty())
        GUIKIT::Vector::combine( errorPaths, firmwareManager->missingFirmware );
}

auto States::oneMediumOnly(Emulator::Interface::MediaGroup* group, Emulator::Interface::Media* mediaInUse) -> void {
    
    for( auto& media : group->media ) {
        
        if ((&media == mediaInUse) || media.secondary)
            continue;

        media.guid = uintptr_t(nullptr);
        filePool->assign( _ident(emulator, media.name), nullptr);
        updateImage(nullptr, &media);
    }
    
    if (!mediaInUse)
        emulator->ejectMedium( group->selected ); 
}

auto States::loadImagePaths( GUIKIT::Settings* loadSettings ) -> std::vector<Emulator::Interface::Media*> {
    std::vector<Emulator::Interface::Media*> loadedMedia;
    
    auto setting = new FileSetting( loadSettings );

    for( auto& mediaGroup : emulator->mediaGroups ) {
        
        if (mediaGroup.isProgram())
            continue;

        bool IPMode = mediaGroup.isExpansion() && mediaGroup.expansion->isRS232();

        auto mediaSelected = mediaGroup.selected;
        Emulator::Interface::Media* mediaInUse = nullptr;
        
        for( auto& media : mediaGroup.media ) {

            setting->ident = media.name;
            setting->update();

            if (setting->path.empty()) {
                if (!mediaSelected || media.secondary) {
                    emulator->ejectMedium( &media );
                    media.guid = uintptr_t(nullptr);
                    filePool->assign( _ident(emulator, media.name), nullptr);  
                    updateImage( nullptr, &media );
                }
                continue;
            }
            
            if (mediaSelected && !media.secondary)
                mediaInUse = mediaSelected;
            else
                mediaInUse = &media;
            
            InsertImage* inserted = findImage( mediaInUse );

            if (inserted) {
                if ((inserted->setting->path == setting->path)
                    && (inserted->setting->id == setting->id)) {
					
                    if (!GUIKIT::Vector::find( loadedMedia, mediaInUse ))
                        loadedMedia.push_back( mediaInUse );
                    continue;
                }
            }

            if (IPMode) {
                program->prepareSocket( &media, emulator, setting->path );
                updateImage( setting, mediaInUse );
                continue;
            }

            GUIKIT::File* file = filePool->get( setting->path );

            if (!file)
                continue;                           

            uint8_t* data = nullptr;

            if (!program->loadImageDataWhenOk( file, setting->id, &mediaGroup, data )) {
                if ( !GUIKIT::Vector::find( errorPaths, setting->path ) )
                    errorPaths.push_back(setting->path);
                continue;
            }                                    

            emulator->ejectMedium( mediaInUse );
			
			mediaInUse->guid = uintptr_t(file);
            emulator->insertMedium( mediaInUse, data, file->archiveDataSize( setting->id ));
                       
            if (!GUIKIT::Vector::find( loadedMedia, mediaInUse ))
                loadedMedia.push_back( mediaInUse );
                       
            filePool->assign( _ident(emulator, mediaInUse->name), file);
            updateImage( setting, mediaInUse );
        }
                        
        if (mediaSelected)
            oneMediumOnly( &mediaGroup, mediaInUse ? mediaSelected : nullptr );            
    }

    filePool->unloadOrphaned();

    delete setting;
    
    return loadedMedia;
}

auto States::saveImagePaths( std::string path ) -> bool {

    updateSaveable();
    
    return saveSettings->save( path );       
}

auto States::findImage( Emulator::Interface::Media* media ) -> InsertImage* {

    for (auto& insert : inserted) {

        if (insert.media == media)            
            return &insert;        
    }
    
    return nullptr;
}

auto States::updateImage( FileSetting* setting, Emulator::Interface::Media* media ) -> void {
    
    InsertImage* insert = findImage( media );
    
    if ( insert ) {
        copySetting( insert->setting, setting );
        return;
    }    

    auto fileSetting = new FileSetting( saveSettings );
    
    fileSetting->ident = media->name;
    copySetting( fileSetting, setting );    
    
    inserted.push_back( {fileSetting, media} );
}

auto States::findFirmware( Emulator::Interface::Firmware* firmware ) -> InsertFirmware* {

    for (auto& insert : insertedFirmware) {

        if (insert.firmware == firmware)            
            return &insert;        
    }
    
    return nullptr;
}

auto States::updateFirmware( FileSetting* setting, Emulator::Interface::Firmware* firmware ) -> void {
    
    InsertFirmware* insert = findFirmware( firmware );
    
    if ( insert ) {
        copySetting( insert->setting, setting );
        insert->setting->setSaveable( setting != nullptr );
        return;
    }    

    auto fileSetting = new FileSetting( saveSettings );
    
    fileSetting->ident = firmware->name;
    copySetting( fileSetting, setting );
    fileSetting->setSaveable( setting != nullptr );
    
    insertedFirmware.push_back( {fileSetting, firmware} );
}

auto States::copySetting( FileSetting* target, FileSetting* src ) -> void {

    target->setPath( src ? src->path : "" );

    target->setId( src ? src->id : 0 );
}

auto States::getInstance( Emulator::Interface* emulator ) -> States* {
	
	for (auto state : states) {
		if (state->emulator == emulator)
			return state;
	}
    
	return nullptr;
}

auto States::getInstanceAuto() -> States* {
    
    if (activeEmulator)
        return getInstance( activeEmulator );
    
    // while loading by hotkeys emulation could be powered off.
    std::string ident = globalSettings->get<std::string>("fast_save_emu", "");
    States* defaultState = nullptr;
    
    for (auto state : states) {
        
        if (dynamic_cast<LIBC64::Interface*>(state->emulator))
            defaultState = state;
        
        if (ident == state->emulator->ident)
            return state;        
    }
    
    return defaultState;
}

auto States::changeSlot( bool down ) -> void {
    
    unsigned pos = settings->get<unsigned>("save_slot", 0);
    if (down && pos == 0)
        return;
    
    pos += down ? -1 : 1;
    settings->set<unsigned>("save_slot", pos);
    
    statusMessage( "slot_changed", std::to_string(pos) );
}

auto States::statusMessage( std::string langKey, std::string replacer ) -> void {
    statusHandler->setMessage(trans->get(langKey,{
        {"%ident%", replacer}
    }), 4, GUIKIT::String::foundSubStr(langKey, "error") || GUIKIT::String::foundSubStr(langKey, "incompatible") );
}

auto States::statesFolder() -> std::string {
    
    auto path = settings->get<std::string>( "states_folder", "");

    if (path.empty()) {
        std::string _emuIdent = emulator->ident;
        path = program->appFolder() + "/states/" + GUIKIT::String::toLowerCase(_emuIdent);
        std::string basePath = GUIKIT::System::getUserDataFolder( );
        
        GUIKIT::File::createDir( path, basePath );
        
        path = basePath + path;
    }
    
    return GUIKIT::File::beautifyPath(path);
}

auto States::generateAutoPath() -> std::string {
    
    auto ident = settings->get<std::string>( "save_ident", "savestate");
    if (ident == "")
        ident = "savestate";
    auto pos = settings->get<unsigned>( "save_slot", 0);

    return statesFolder() + ident + "_" + std::to_string( pos ) + ".sav";
}

auto States::updateTapeMenu() -> void {
    
    auto media = activeEmulator->getTape(0);
    if (!media)
        return;

    unsigned count = emulator->getModelValue( emulator->getModelIdOfEnabledDrives(media->group) );
    
    Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed;
    if (count)
        mode = emulator->getTapeControl( media );
    
    view->showTapeMenu( count ? true : false, mode );
}

auto States::updateSaveable() -> void {
    
	auto emuExpansion = emulator->getExpansion();
		
    auto expansionMediaGroup = emuExpansion->mediaGroup;
	auto expansionMediaGroupExpanded = emuExpansion->mediaGroupExpanded;
    
    for( auto& mediaGroup : emulator->mediaGroups ) {

        unsigned maxCount = 1;
        if (mediaGroup.isDrive())
            maxCount = emulator->getModelValue( emulator->getModelIdOfEnabledDrives( &mediaGroup  ) );

        for( auto& media : mediaGroup.media ) {
            
            auto insert = findImage( &media );
            
            if (!insert)
                continue;
            
            if (mediaGroup.isExpansion()) {
				if ( ((expansionMediaGroup == &mediaGroup) || (expansionMediaGroupExpanded == &mediaGroup)) && (!media.secondary || emulator->hasExpansionSecondaryRom()) )
					insert->setting->setSaveable( !insert->setting->path.empty() );
				else
					insert->setting->setSaveable( false );
                
            } else if (mediaGroup.isProgram()) {
                insert->setting->setSaveable( false );
                
            } else                
                insert->setting->setSaveable( maxCount > 0 );
            
            if (maxCount)
                maxCount--;
        }
    }
}

auto States::updateConnectedDevices() -> void {
    
    std::vector<unsigned> deviceIds;
    
    for( auto& connector : emulator->connectors ) {
        auto deviceId = settings->get<unsigned>( _underscore(connector.name), 0);
        deviceIds.push_back( deviceId );
    }
    
    for( auto& connector : emulator->connectors ) {
                
        auto device = emulator->getConnectedDevice( &connector );
        
        GUIKIT::Vector::eraseVectorElement( deviceIds, device->id );        
        
        settings->set<unsigned>( _underscore(connector.name), device->id);

        view->checkInputDevice( emulator, &connector, device );
    }
    
    if (deviceIds.size() > 0)
        InputManager::getManager( emulator )->updateMappingsInUse();
    EmuConfigView::TabWindow::getView(emulator)->inputLayout->updateConnectorButtons();
    view->setCursor( emulator );
}

auto States::updateModels() -> void {

    bool regionChange = false;
    bool resamplerChange = false;
    
    auto cfgView = EmuConfigView::TabWindow::getView( emulator );

    for(auto& model : emulator->models) {

        if (model.isPerformance() || model.isHidden())
            continue;

        int value = emulator->getModelValue( model.id );
		
		if (model.isGraphicChip()) {
			auto oldValue = settings->get<int>( _underscore( model.name ), model.defaultValue, model.range );
			regionChange = value != oldValue;            
            
		} else if (!resamplerChange && model.isAudioResampler()) {
            auto oldValue = settings->get<int>(_underscore( model.name ), model.defaultValue, model.range );            
            resamplerChange = value != oldValue;                
        }

        if (model.isSwitch() )
            settings->set<bool>( _underscore( model.name), (bool)value );
        else
            settings->set<int>( _underscore( model.name), value );                        
    }

    cfgView->systemLayout->modelLayout.updateWidgets();
    cfgView->systemLayout->driveModelLayout.updateWidgets();
    if (cfgView->audioLayout)
        cfgView->audioLayout->settingsLayout.updateWidgets();
    
    if (regionChange) {
        auto cfgView = EmuConfigView::TabWindow::getView( emulator );
    
        cfgView->videoLayout->updatePresets();  
    }
    
    if (regionChange || resamplerChange) {
        audioManager->power();

        if (activeVideoManager)
            activeVideoManager->initFpsLimit();
    }
}

auto States::updateExpansionJumper() -> void {
        
    for( auto& mediaGroup : emulator->mediaGroups ) {
        
        if (!mediaGroup.isExpansion() || !mediaGroup.selected || (mediaGroup.expansion->jumpers.size() == 0) )
            continue;        
                    
        EmuConfigView::TabWindow::getView( emulator )->mediaLayout->updateJumper( mediaGroup.selected );
    }        
}

auto States::updateWriteProtection(std::vector<Emulator::Interface::Media*> loadedMedia) -> void {
    
    for (auto media : loadedMedia) {
        
        auto file = (GUIKIT::File*)media->guid;
        
        bool forceWp = file && (file->isArchived() || file->isReadOnly());
        
        if (forceWp)
            // override write protection of state, i.e. file permissions were changed between saving and loading a state
            emulator->writeProtect( media, true );
        
        EmuConfigView::TabWindow::getView( activeEmulator )->mediaLayout->updateWriteProtection( media, emulator->isWriteProtected(media) );
    }       
}

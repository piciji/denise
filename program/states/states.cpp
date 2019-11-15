
#include "states.h"
#include "../firmware/manager.h"

std::vector<States*> states;

States::States(Emulator::Interface* emulator) {
    saveSettings = new GUIKIT::Settings;
    this->emulator = emulator;
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
            
    if (!activeEmulator || (activeEmulator != emulator) )
        program->power( emulator, !imageFileLoaded );      
    
    errorPaths.clear();
    
    if (imageFileLoaded) {
        loadImagePaths( &loadSettings );        
        loadFirmwarePaths( &loadSettings );
    }
        
    program->showOpenError( errorPaths, true );

    emulator->loadstate( data, file.getSize() );

    updateFeatures();

    updateConnectedDevices();
    
    updateTapeMenu();
    
    updateRegion();
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
            settings->set("fast_save_emu", emulator->ident);
        }                            
    }            

    statusMessage( langKey, file.getFileName() );    
}    

auto States::loadFirmwarePaths( GUIKIT::Settings* loadSettings ) -> void {
    
    auto setting = new FileSetting( loadSettings );
    
    for( auto& firmware : emulator->firmwares ) {
        
        setting->ident = firmware.name;
        setting->update();

        if (setting->path.empty()) {
            // should never happen. don't change firmware setup
            continue;
        }

        InsertFirmware* inserted = findFirmware( &firmware );

        if (inserted) {
            if ((inserted->setting->path == setting->path)
                && (inserted->setting->id == setting->id))
                // ok, already inserted
                continue;
        }
        
        // savestate was generated with different firmware
        auto firmwareManager = FirmwareManager::getInstance( emulator );
        // use a store level not used for preconfigured firmware sets
        unsigned storeLevel = firmwareManager->maxSets + 10;
        
        FileSetting* storeSetting = firmwareManager->getSetting( &firmware, storeLevel );
        storeSetting->id = setting->id;
        storeSetting->path = setting->path;
        storeSetting->setSaveable( false );
                
        if (firmwareManager->loadImage( &firmware, storeLevel ))
            firmwareManager->useImage( &firmware, storeLevel );
        else {
            if (!GUIKIT::Vector::find(errorPaths, setting->path))
                errorPaths.push_back(setting->path);
        }
    }
}

auto States::loadImagePaths( GUIKIT::Settings* loadSettings ) -> void {
        
    auto setting = new FileSetting( loadSettings );

    for( auto& driveGroup : emulator->driveGroups ) {

        for( auto& drive : driveGroup.drives ) {

            setting->ident = drive.name;
            setting->update();

            if (setting->path.empty()) {
                emulator->ejectMedium( driveGroup.type, drive.id );
                drive.guid = uintptr_t(nullptr);
                filePool->assign(program->ident(emulator, drive.name), nullptr);  
                updateImage( nullptr, &drive );
                continue;
            }
            
            InsertImage* inserted = findImage( &drive );
            
            if (inserted) {
                if ((inserted->setting->path == setting->path)
                    && (inserted->setting->id == setting->id))
                    continue;
            }
            
            GUIKIT::File* file = filePool->get( setting->path );
            
            if (!file)
                continue;                           

            uint8_t* data;

            if (!program->loadImageDataWhenOk( file, setting->id, &driveGroup, data )) {
                if ( !GUIKIT::Vector::find( errorPaths, setting->path ) )
                    errorPaths.push_back(setting->path);
                continue;
            }            
            
            drive.guid = uintptr_t(file);
            if (driveGroup.isHardDrive()) {                
                emulator->setHardDrive( drive.id, file->getSize() );                
            } else {                
                emulator->ejectMedium( driveGroup.type, drive.id );
                emulator->insertMedium( driveGroup.type, drive.id, data, file->archiveDataSize( setting->id ));
                emulator->writeProtect( driveGroup.type, drive.id, file->isArchived() ? true : setting->writeProtect);
            }            
            filePool->assign(program->ident(emulator, drive.name), file);  
            updateImage( setting, &drive );
        }
    }

    filePool->unloadOrphaned();

    delete setting;
}

auto States::saveImagePaths( std::string path ) -> bool {

    updateSaveable();
    
    return saveSettings->save( path );       
}

auto States::findImage( Emulator::Interface::Drive* drive ) -> InsertImage* {

    for (auto& insert : inserted) {

        if (insert.drive == drive)            
            return &insert;        
    }
    
    return nullptr;
}

auto States::updateImage( FileSetting* setting, Emulator::Interface::Drive* drive ) -> void {
    
    InsertImage* insert = findImage( drive );
    
    if ( insert ) {
        copySetting( insert->setting, setting );
        return;
    }    

    auto fileSetting = new FileSetting( saveSettings );
    
    fileSetting->ident = drive->name;
    copySetting( fileSetting, setting );    
    
    inserted.push_back( {fileSetting, drive} );
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
        return;
    }    

    auto fileSetting = new FileSetting( saveSettings );
    
    fileSetting->ident = firmware->name;
    copySetting( fileSetting, setting );    
    
    insertedFirmware.push_back( {fileSetting, firmware} );
}

auto States::copySetting( FileSetting* target, FileSetting* src ) -> void {     

    target->setPath( src ? src->path : "" );

    target->setId( src ? src->id : 0 );

    target->setWriteProtect( src ? src->writeProtect : true );
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
    std::string ident = settings->get<std::string>("fast_save_emu", "");
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

    unsigned pos = settings->get<unsigned>(program->ident(emulator, "save_slot"), 0);
    if (down && pos == 0)
        return;
    
    pos += down ? -1 : 1;
    settings->set<unsigned>(program->ident(emulator, "save_slot"), pos);
    
    statusMessage( "slot_changed", std::to_string(pos) );
}

auto States::statusMessage( std::string langKey, std::string replacer ) -> void {
    status->addMessage(trans->get(langKey,{
        {"%ident%", replacer}
    }), 4, GUIKIT::String::foundSubStr(langKey, "error") || GUIKIT::String::foundSubStr(langKey, "incompatible") );
}

auto States::statesFolder() -> std::string {
    auto path = settings->get<std::string>( program->ident(emulator, "states_folder"), "");

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
    auto ident = settings->get<std::string>( program->ident(emulator, "save_ident"), "savestate");
    if (ident == "")
        ident = "savestate";
    auto pos = settings->get<unsigned>( program->ident(emulator, "save_slot"), 0);

    return statesFolder() + ident + "_" + std::to_string( pos ) + ".sav";
}

auto States::updateTapeMenu() -> void {
    
    auto drive = activeEmulator->getTapeDrive(0);
    if (!drive)
        return;

    unsigned count = emulator->getDrivesConnected( drive->group->id );
    
    Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed;
    if (count)
        mode = emulator->getTapeControl( drive->id );
    
    view->showTapeMenu( count ? true : false, mode );
}

auto States::updateSaveable() -> void {
    
    for( auto& driveGroup : emulator->driveGroups ) {
        
        unsigned maxCount = emulator->getDrivesConnected( driveGroup.id );
        
        for( auto& drive : driveGroup.drives ) {
            
            auto insert = findImage( &drive );
            
            if (!insert)
                continue;
            
            insert->setting->setSaveable( maxCount > 0 );
            
            if (maxCount)
                maxCount--;
        }
    }
}

auto States::updateConnectedDevices() -> void {
    
    std::vector<unsigned> deviceIds;
    
    for( auto& connector : emulator->connectors ) {
        auto deviceId = settings->get<unsigned>( program->ident(emulator, connector.name), 0);
        deviceIds.push_back( deviceId );
    }
    
    for( auto& connector : emulator->connectors ) {
                
        auto device = emulator->getConnectedDevice( &connector );
        
        GUIKIT::Vector::eraseVectorElement( deviceIds, device->id );        
        
        settings->set<unsigned>( program->ident(emulator, connector.name), device->id);

        view->checkInputDevice( emulator, &connector, device );
    }
    
    if (deviceIds.size() > 0)
        InputManager::getManager( emulator )->updateMappingsInUse();
    EmuConfigView::TabWindow::getView(emulator)->inputLayout->updateConnectorButtons();
    view->setCursor( emulator );
}

auto States::updateFeatures() -> void {

    auto cfgView = EmuConfigView::TabWindow::getView( emulator );

    for(auto& feature : emulator->features) {

        if (!feature.runtimeChangeable)
            continue;                        

        int value = emulator->getFeature( feature.id );

        if (feature.isSwitch() )
            settings->set<bool>( program->ident(emulator, feature.name), (bool)value );
        else
            settings->set<int>( program->ident(emulator, feature.name), value );                        
    }

    cfgView->systemLayout->updateRuntimeFeatureWidgets();
}

auto States::updateRegion() -> void {
    
    unsigned stateRegion = emulator->getRegion();
    
    unsigned systemRegion = settings->get<unsigned>( program->ident(emulator, "video_region"), 0, {0u, 1u});
        
    if (stateRegion == systemRegion)
        return;
    
    settings->set<unsigned>(program->ident(emulator, "video_region"), stateRegion);
    
    auto cfgView = EmuConfigView::TabWindow::getView( emulator );
    
    if (stateRegion == 0) {
        cfgView->videoLayout->base.mode.pal.setChecked();
        view->getSysMenu(emulator)->pal->setChecked();
        
    } else {
        cfgView->videoLayout->base.mode.ntsc.setChecked();
        view->getSysMenu(emulator)->ntsc->setChecked();        
    }
    
    cfgView->videoLayout->updatePresets();
    
    VideoManager::getInstance( this->emulator )->reloadSettings();
    
    audioManager->power();
}

#include "program.h"
#include "../data/fonts.h"

auto Program::showOpenError( std::vector<std::string>& paths, bool warning ) -> void {
    if ( paths.empty() )
        return;
    
    std::string transKey = "file_open_error";
    std::string replaceIdent = "%path%";
    std::string replace = "\"" + paths[0] + "\"";    
    
    if (paths.size() > 1) {
        transKey = "files_open_error";
        replaceIdent = "%paths%";
        replace = "\n\n" + GUIKIT::String::unsplit(paths, "\n");
    }
    
    if (warning) 
        view->message->warning(trans->get(transKey, { {replaceIdent, replace} }));
    else
        view->message->error(trans->get(transKey, { {replaceIdent, replace} }));
}

auto Program::loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::MediaGroup* group, uint8_t*& data ) -> bool {
    
    if (!file)
        return false;
    
    // hard disks will not preloaded
    // we check only for max size
    if ( group->isHardDisk() ) {        
        if (!file->exists() || file->isArchived() ||
            !file->isSizeValid(MAX_HARDDISK_SIZE) || 
            !file->open(GUIKIT::File::Mode::Update)) {
            
            return false;
        }
        
        return true;
    }
    
    if (!file->exists() || !file->isSizeValid(MAX_MEDIUM_SIZE))
        return false;
    
    // non archived tape images will be loaded in chunks when needed
    if ( group->isTape() && !file->isArchived() ) {        
		data = nullptr;
		auto items = file->scanArchive();
		return !items.empty();
	}
	
    // when archive, we extract requested file from archive 
	data = file->archiveData( fileId );
	
	return data != nullptr;
}

auto Program::errorOpen(GUIKIT::File* file, Message* message ) -> void {

    message->error(trans->get( "file_open_error", {
            { "%path%", file->getFile() }
    }));

    filePool->unloadOrphaned();
}

auto Program::errorOpen(GUIKIT::File* file, GUIKIT::File::Item* item, Message* message ) -> void {
           
    message->error(trans->get( (file->isArchived() && !item) ? "archive_error" : "file_open_error", {
        { "%path%", item ? item->info.name : file->getFile() }
    }));
    
    filePool->unloadOrphaned();
}

auto Program::errorMediumSize(GUIKIT::File* file, Message* message ) -> void {

    message->error(trans->get("file_size_error",{
        {"%path%", file->getPath()},
        {"%size%", GUIKIT::File::SizeFormated(MAX_MEDIUM_SIZE)}
    }));
    
    filePool->unloadOrphaned();
}

auto Program::errorFirmwareSize(GUIKIT::File::Item* item, Message* message ) -> void {

    message->error(trans->get("file_size_error",{
        { "%path%", item->info.name},
        { "%size%", GUIKIT::File::SizeFormated( MAX_FIRMWARE_SIZE )}
    }));
    
    filePool->unloadOrphaned();
}


auto Program::readMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
	if (!media->guid)
		return 0;
    
    auto file = (GUIKIT::File*)media->guid;
    return file->read(buffer, length, offset);
}

auto Program::writeMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
	if (!activeEmulator || !media->guid)
		return 0;
    
    auto file = (GUIKIT::File*)media->guid;
    return file->write(buffer, length, offset);
}

auto Program::truncateMedia(Emulator::Interface::Media* media) -> bool {
    if (!activeEmulator || !media->guid)
		return false;
    
    auto file = (GUIKIT::File*)media->guid;
    
    return file->truncate();
}

auto Program::getFileNameFromMedia(Emulator::Interface::Media* media) -> std::string {
	if (!media->guid)
		return "";
	
	auto file = (GUIKIT::File*)media->guid;	
	
	return file->getFileName(true);
}

auto Program::setExpansionSelection( Emulator::Interface* emulator ) -> void {
    
    auto settings = getSettings( emulator );
    
    for( auto& mediaGroup : emulator->mediaGroups ) {
        
        if ( mediaGroup.selected ) {

            auto mediaId = settings->get<unsigned>( _underscore( mediaGroup.name ) + "_selected", mediaGroup.media[0].id );
            
            auto media = emulator->getMedia( mediaGroup, mediaId );
            
            if (media && !media->secondary)
                mediaGroup.selected = media;
        }                
    }
    
    for ( auto& expansion : emulator->expansions ) {
        
        if (!expansion.mediaGroup || (expansion.pcbs.size() == 0) )
            continue;
        
        for(auto& media : expansion.mediaGroup->media) {

            if (!media.pcbLayout || media.secondary) {
                continue;
            }

            auto pcbId = settings->get<unsigned>( _underscore( media.name ) + "_pcb", expansion.pcbs[0].id );

            auto pcbLayout = emulator->getPCB( expansion, pcbId );

            media.pcbLayout = pcbLayout ? pcbLayout : &expansion.pcbs[0];
        }
    }
}

auto Program::updateSaveIdent(Emulator::Interface::Media* media, std::string file) -> void {
    
    static Emulator::Interface::Media* _media = nullptr;
    
    if (!media) {
        _media = nullptr;
        return;
    }        
    
    if ( (media->group->isExpansion() && !media->secondary) || (!_media && !media->group->isProgram())
    || (media->group->isDisk() && !_media->group->isDisk() && !_media->group->isExpansion())
    || (media->group->isTape() && !_media->group->isDisk() && !_media->group->isExpansion())) {
        updateSaveIdent( activeEmulator, file );
        _media = media;
    }
}

auto Program::updateSaveIdent( Emulator::Interface* emulator, std::string fileName ) -> void {
    auto settings = getSettings( emulator );
    std::size_t end = fileName.find_last_of(".");
    if (end != std::string::npos)
        fileName = fileName.erase(end);

    // for wav record
    settings->set<std::string>( "record_ident", fileName, false);

    if (!settings->get<bool>( "auto_save_ident", true))
        return;

    settings->set<std::string>( "save_ident", fileName);
    settings->set<unsigned>( "save_slot", 0);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->configurationsLayout)
        emuView->configurationsLayout->updateSaveIdent( fileName );
}

auto Program::removeExpansion( bool bootableOnly ) -> void {
    
    if (!activeEmulator)
        return;
    
    if (bootableOnly && !activeEmulator->isExpansionBootable())
        return;
    
    auto expansion = activeEmulator->getExpansion();
    
    if (!expansion || expansion->isEmpty())
        return;
    
	auto medias = expansion->mediaGroup->media;
	if (expansion->mediaGroupExpanded)
		medias = GUIKIT::Vector::concat( medias, expansion->mediaGroupExpanded->media );
	
    for( auto& media : medias) {
        filePool->assign( _ident(activeEmulator, media.name), nullptr);
        activeEmulator->ejectMedium( &media );
		auto state = States::getInstance( activeEmulator );
		if (state)
			state->updateImage( nullptr, &media );
    }
    
    activeEmulator->unsetExpansion();
    
	program->getSettings( activeEmulator )->set<unsigned>("expansion", 0);
	
	auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
	if (emuView && emuView->systemLayout)
        emuView->systemLayout->setExpansion( nullptr );

	if (activeEmulator)
        activeEmulator->powerOff();
    activeEmulator->power();
}

auto Program::prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void {

    std::string port = "";

    auto parts = GUIKIT::String::split( address, ':' );

    if (parts.size() > 1) {

        port = parts[ parts.size() - 1 ];

        address = parts[0];
    }

    emulator->prepareSocket( media, address, port );
}

// settings
auto Program::undockSettings() -> bool {
    for (auto settings : settingsStorage) {
        auto guid = settings->getGuid();

        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;

            settings->save( settingsFileFromEmuFolder(emulator->ident + "_") );
        } else {
            if (!settings->save( settingsFileFromEmuFolder("global_") ))
                return false;
        }
    }
    return true;
}

auto Program::saveSettings(bool onExit) -> void {
    bool errorShown = false;

    for (auto settings : settingsStorage) {

        auto guid = settings->getGuid();
        
        std::string path;
        
        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;
                                   
            path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");

            if (path == "") {
                path = settingsFileFromEmuFolder(emulator->ident + "_");

                GUIKIT::File file(path);
                if (!file.exists())
                    path = settingsFile(emulator->ident + "_");

            } else if (onExit)
                continue;
            else {
                // remove absolute path (deprecated)
                path = GUIKIT::String::getFileName(path);

                path = getCustomSettingsFolder(emulator) + path;
            }
            
        } else {
            path = settingsFileFromEmuFolder("global_");

            GUIKIT::File file(path);
            if (!file.exists())
                path = settingsFile("global_");
        }

        if (!settings->save( path )) {
            if (!errorShown) {
                view->message->warning(trans->get("cfg_not_save", {{"%path%", path}}));
                errorShown = true;
            }
        }
    }
}

auto Program::loadSettings() -> void {
    
    for(auto settings : settingsStorage) {
        
        auto guid = settings->getGuid();

        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;
            bool lastUsed = globalSettings->get<bool>( emulator->ident + "_load_last_settings" );

            if (lastUsed) {
                std::string path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");
                if (path != "") {
                    // remove absolute path (deprecated)
                    path = GUIKIT::String::getFileName(path);

                    path = getCustomSettingsFolder(emulator) + path;

                    if (settings->load(path))
                        continue;
                }
            }

            globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");

            if (!settings->load(settingsFileFromEmuFolder(emulator->ident + "_")))
                settings->load(settingsFile(emulator->ident + "_"));

        } else {
            if (!settings->load(settingsFileFromEmuFolder("global_")))
                settings->load(settingsFile("global_"));
            else
                settings->set<bool>("undock", true, false);
        }
    }
}

auto Program::forceSavingSomeGlobalSettings( ) -> void {
	GUIKIT::Settings tempSettings;
    std::string path;
    bool useEmuFolder = true;

    if (!tempSettings.load(settingsFileFromEmuFolder("global_"))) {
        useEmuFolder = false;
        if (!tempSettings.load(settingsFile("global_")))
            return;
    }
	
	tempSettings.set<bool>("save_settings_on_exit", false);

    for( auto emulator : emulators ) {
        std::string _emuIdent = emulator->ident;

        auto state = globalSettings->get<bool>( _emuIdent + "_load_last_settings", false );
        auto customSetting = globalSettings->get<std::string>( _emuIdent + "_custom_settings", "");
        path = globalSettings->get<std::string>( _emuIdent + "_settings_path", "");

        tempSettings.set<bool>(_emuIdent + "_load_last_settings", state);
        tempSettings.set<std::string>(_emuIdent + "_custom_settings", customSetting);
        tempSettings.set<std::string>(_emuIdent + "_settings_path", path);
    }

    tempSettings.save( useEmuFolder ? settingsFileFromEmuFolder("global_") : settingsFile("global_") );
}

auto Program::getCustomSettingsFolder( Emulator::Interface* emulator, bool createFolder ) -> std::string {

    std::string _emuIdent = emulator->ident;
    std::string path = globalSettings->get<std::string>( _emuIdent + "_settings_path", "");
    std::string basePath;

    if (path == "") {
        path = program->appFolder() + "/settings/" + GUIKIT::String::toLowerCase(_emuIdent);
        basePath = GUIKIT::System::getUserDataFolder();

        if (createFolder)
            GUIKIT::File::createDir( path, basePath );

        path = basePath + path;
    }

    return GUIKIT::File::beautifyPath(path);
}

auto Program::getSettings( Emulator::Interface* emulator ) -> GUIKIT::Settings* {
    
    for(auto settings : settingsStorage) {        
        if ( (Emulator::Interface*)(settings->getGuid()) == emulator )
            return settings;
    }
    // global setting
    return settingsStorage[0];
}

auto Program::addCustomFont() -> void {
    GUIKIT::CustomFont* font;

    for(auto emulator : emulators) {
        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            font = new GUIKIT::CustomFont;
            font->name = "C64 Pro";
            font->refPtr = (void*)emulator;
            font->data = (uint8_t*) Fonts::c64Pro;
            font->size = sizeof(Fonts::c64Pro);
            font->filePath = fontFolder() + "C64_Pro-STYLE121.ttf";
            font->modifier = 0xee << 8;
            bool useCustomFont = GUIKIT::Window::addCustomFont( font );
            ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode(useCustomFont);
            ((LIBC64::Interface*) emulator)->loadWithColumn( getSettings(emulator)->get<bool>("autostart_load_with_column") );

        } else if ( dynamic_cast<LIBAMI::Interface*>(emulator)) {
            font = new GUIKIT::CustomFont;
            font->name = "TopazPlus a500a1000a2000";
            //font->name = "Topaz a500a1000a2000";
            font->refPtr = (void*)emulator;

            font->data = (uint8_t*)Fonts::amigaTopazPlus;
            font->size = sizeof(Fonts::amigaTopazPlus);
            font->filePath = fontFolder() + "TopazPlus_a500_v1.0.ttf";

//            font->data = (uint8_t*)Fonts::amigaTopaz;
//            font->size = sizeof(Fonts::amigaTopaz);
//            font->filePath = fontFolder() + "Topaz_a500_v1.0.ttf";

            font->sizeAdjust = 1;
            font->modifier = 0;
            GUIKIT::Window::addCustomFont( font );
        }
    }
}
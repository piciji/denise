
#include "program.h"

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
        if (file->isArchived() ||
            !file->isSizeValid(MAX_HARDDISK_SIZE) || 
            !file->open(GUIKIT::File::Mode::Update)) {
            
            return false;
        }
        
        return true;
    }
    
    if (!file->isSizeValid(MAX_MEDIUM_SIZE))
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
	if (!activeEmulator || !media->guid)
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
    
    if ( media->group->isExpansion() || (!_media && !media->group->isProgram())
    || (media->group->isDisk() && !_media->group->isDisk() && !_media->group->isExpansion())
    || (media->group->isTape() && !_media->group->isDisk() && !_media->group->isExpansion())) {
        EmuConfigView::TabWindow::getView( activeEmulator )->configurationsLayout->updateSaveIdent( file );
        _media = media;
    }
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
	if (emuView)
		emuView->systemLayout->setExpansion( nullptr );
    
    activeEmulator->power();
}

// settings
auto Program::saveSettings() -> void {

    bool errorShown = false;
    
    for (auto settings : settingsStorage) {

        auto guid = settings->getGuid();
        
        std::string path = "";
        
        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;
                                   
            path = settings->get<std::string>("custom_settings", "");
            if (path == "")
                path = settingsFile( emulator->ident + "_" );
            
        } else
            path = settingsFile( "global_" );
        
        if (!settings->save( path )) {
            if (!errorShown)
                view->message->warning(trans->get("cfg_not_save",{{"%path%", path}}));
            errorShown = true;
        }
    }
}

auto Program::loadSettings() -> void {
    
    for(auto settings : settingsStorage) {
        
        auto guid = settings->getGuid();

        std::string ident = "global_";

        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;

            ident = emulator->ident + "_";
        }
        
        settings->load( settingsFile( ident ) );         
    }
    
    convertSettings();
}

auto Program::convertSettings() -> void {
        
    if (globalSettings->get("convert_to_v2", false) )
        return;
    
    GUIKIT::Settings oldSettings;
    
    if (!oldSettings.load( settingsFile() )) {
        globalSettings->set("convert_to_v2", true);
        return;
    }
    
    auto settingsC64 = getSettings( getEmulator( "C64" ) );
    
    std::vector<GUIKIT::Setting*> list = oldSettings.getList();
    
    for( auto setting : list ) {
        
        std::string ident = setting->getIdent();
        
        std::string identIgnoreCase = ident;
        
        GUIKIT::String::toLowerCase( identIgnoreCase );
        
        if (GUIKIT::String::foundSubStr( identIgnoreCase, "amiga_" ))
            continue;
            
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "port1_" ))
            continue;

        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "port2_" ))
            continue;
        
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "system_" ))
            continue;
        
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "wp_enabled" ))
            continue;

        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "_file" ) && (setting->value == "") )
            continue;
        
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "_path" ) && (setting->value == "") )
            continue;
        
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "_id" ) && (setting->value == "0") )
            continue;

        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "_wp" ) )
            continue;
        
        else if (GUIKIT::String::foundSubStr( identIgnoreCase, "c64_" )) {
            
            oldSettings.remove( ident );
            
            GUIKIT::String::replace( ident, "c64_", "" );                        
            
            GUIKIT::String::replace( ident, "C64_", "" );     
            
            setting->setIdent( ident );
            
            settingsC64->add( setting );
        }        
        else {
            
            oldSettings.remove( ident );
            
            globalSettings->add( setting );
        }
    }
    
    globalSettings->set("convert_to_v2", true);
    
    if (list.size())
        saveSettings();
}

auto Program::rememberNotToSaveSettings() -> void {
	GUIKIT::Settings tempSettings;
	
	if (!tempSettings.load( settingsFile("global_") ))
		return;
	
	tempSettings.set<bool>("save_settings_on_exit", false);
	
	tempSettings.save( settingsFile("global_") );
}

auto Program::getSettings( Emulator::Interface* emulator ) -> GUIKIT::Settings* {
    
    for(auto settings : settingsStorage) {        
        if ( (Emulator::Interface*)(settings->getGuid()) == emulator )
            return settings;
    }
    // global setting
    return settingsStorage[0];
}
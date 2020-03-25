
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

auto Program::setExpansionSelection( Emulator::Interface* emulator ) -> void {
    
    for( auto& mediaGroup : emulator->mediaGroups ) {
        
        if ( mediaGroup.selected ) {
            
            auto mediaId = settings->get<unsigned>( ident(emulator, mediaGroup.name + "_selected"), mediaGroup.media[0].id );
            
            auto media = emulator->getMedia( mediaGroup, mediaId );
            
            if (media && !media->memoryDump)
                mediaGroup.selected = media;
        }                
    }
    
    for ( auto& expansion : emulator->expansions ) {
        
        if (!expansion.mediaGroup || (expansion.pcbs.size() == 0) )
            continue;
        
        for(auto& media : expansion.mediaGroup->media) {

            auto pcbId = settings->get<unsigned>( ident(emulator, media.name + "_pcb"), expansion.pcbs[0].id );

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
    
    if (view->ddControl.mediaGroups.size()) {
        // drag'n'drop happened
        if (view->ddControl.mediaGroups[0] == media->group)
            EmuConfigView::TabWindow::getView( activeEmulator )->statesLayout->updateSaveIdent( file );
        
        return;
    }        
    
    if ( media->group->isExpansion() || (!_media  && !media->group->isProgram())
    || (media->group->isDisk() && !_media->group->isDisk() && !_media->group->isExpansion())
    || (media->group->isTape() && !_media->group->isDisk() && !_media->group->isExpansion())) {
        EmuConfigView::TabWindow::getView( activeEmulator )->statesLayout->updateSaveIdent( file );
        _media = media;
    }
}

auto Program::removeBootableExpansion() -> void {
    
    if (!activeEmulator || !activeEmulator->isExpansionBootable())
        return;
    
    auto expansion = activeEmulator->getExpansion();
    
    if (!expansion)
        return;
    
    for( auto& media : expansion->mediaGroup->media) {
        filePool->assign( ident(activeEmulator, media.name), nullptr);
        activeEmulator->ejectMedium( &media );
        States::getInstance( activeEmulator )->updateImage( nullptr, &media );
    }
    
    activeEmulator->unsetExpansion();
    
    EmuConfigView::TabWindow::getView(activeEmulator)->systemLayout->setExpansion( nullptr );
    
    activeEmulator->power();
}
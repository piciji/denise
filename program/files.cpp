
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

auto Program::loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::DriveGroup* group, uint8_t*& data ) -> bool {
    
    if (!file)
        return false;
    
    // hard disks will not preloaded
    // we check only for max size
    if ( group->isHardDrive() ) {        
        if (file->isArchived() ||
            !file->isSizeValid(MAX_HARDDISK_SIZE) || 
            !file->open(GUIKIT::File::Mode::Update)) {
            
            return false;
        }
        
        return true;
    }
    
    if (!file->isSizeValid(MAX_ARCHIVE_SIZE))
        return false;

    // check single file size
    if ( !group->isTapeDrive() && !file->isSizeValid(fileId, MAX_MEDIUM_SIZE) )
        return false;    
    
    // non archived tape images will loaded in chunks when needed
    if ( group->isTapeDrive() && !file->isArchived() ) {        
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

auto Program::errorArchiveSize(GUIKIT::File* file, Message* message ) -> void {

    message->error(trans->get("file_size_error",{
        {"%path%", file->getPath()},
        {"%size%", GUIKIT::File::SizeFormated(MAX_ARCHIVE_SIZE)}
    }));
    
    filePool->unloadOrphaned();
}

auto Program::errorMediumSize(GUIKIT::File::Item* item, Message* message ) -> void {

    message->error(trans->get("file_size_error",{
        { "%path%", item->info.name},
        { "%size%", GUIKIT::File::SizeFormated( MAX_MEDIUM_SIZE )}
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


auto Program::readDrive(unsigned groupId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
	if (!activeEmulator)
		return 0;
	
    auto& drive = activeEmulator->driveGroups[groupId].drives[driveId];
    if (!drive.guid) return 0;
    auto file = (GUIKIT::File*)drive.guid;
    return file->read(buffer, length, offset);
}

auto Program::writeDrive(unsigned groupId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
	if (!activeEmulator)
		return 0;

    auto& drive = activeEmulator->driveGroups[groupId].drives[driveId];
    if (!drive.guid) return 0;
    auto file = (GUIKIT::File*)drive.guid;
    return file->write(buffer, length, offset);
}

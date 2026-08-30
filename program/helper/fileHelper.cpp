
#include "fileHelper.h"

#include "settingsHelper.h"
#include "../view/view.h"
#include "../tools/filepool.h"
#include "../media/fileloader.h"
#include "../tools/filesetting.h"
#include "../emuconfig/layouts/configurations.h"

auto FileHelper::errorOpen(GUIKIT::File* file, GUIKIT::File::Item* item, Message* message ) -> void {
    message->error(trans->get( (file->isArchived() && !item) ? "archive_error" : "file_open_error", {
        { "%path%", item ? item->info.name : file->getFile() }
    }));

    filePool->unloadOrphaned();
}

auto FileHelper::errorOpen( const std::vector<std::string>& paths, bool warning ) -> void {
    if ( paths.empty() || !view )
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

auto FileHelper::loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::MediaGroup* group, uint8_t*& data ) -> bool {
    if (!file || !file->exists() || !file->getSize())
        return false;

    // non archived hard disk images will be loaded in chunks when needed
    if (group->isHardDisk() && !file->isArchived() ) {
        data = nullptr;
        auto items = file->scanArchive();
        return !items.empty();
    }

    data = file->archiveData( fileId );

    return data != nullptr;
}

auto FileHelper::readMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
    if (!media->guid)
        return 0;

    auto file = reinterpret_cast<GUIKIT::File*>(media->guid);
    return file->read(buffer, length, offset);
}

auto FileHelper::readAssignedMedia(Emulator::Interface::Media* media, uint8_t*& buffer, bool preview) -> unsigned {
    std::string path;
    if (!activeEmulator)
        return 0;

    if (preview)
        path = generatedFolder(activeEmulator, "disksave_folder", "disksave") + fileloader->queuePreview.fileName + ".sav";
    else
        path = getAssignedSaveFile(media);

    if (path.empty())
        return 0;

    GUIKIT::File f( path, true );
    if (f.open()) {
        buffer = f.read();
        return f.getSize();
    }
    return 0;
}

auto FileHelper::writeMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
    if (!activeEmulator || !media->guid)
        return 0;

    auto file = (GUIKIT::File*)media->guid;
    if (file->isArchived() || file->isReadOnly())
        return 0;

    return file->write(buffer, length, offset);
}

auto FileHelper::writeAssignedMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length) -> unsigned {
    if (!activeEmulator)
        return 0;

    auto path = getAssignedSaveFile(media, true);
    if (path.empty())
        return 0;

    GUIKIT::File f( path );
    if (f.open(GUIKIT::File::Mode::Write, true)) {
        auto file = (GUIKIT::File*)media->guid;
        if (file)
            file->forceDataChange(); // otherwise UI doesn't refresh preview listing
        return f.write(buffer, length, 0);
    }
    return 0;
}

auto FileHelper::getAssignedSaveFile(Emulator::Interface::Media* media, bool createFolder) -> std::string {
    auto fSetting = FileSetting::getInstance( activeEmulator, _underscore(media->name ) );
    if (!fSetting || fSetting->file.empty())
        return "";
    auto path = generatedFolder(activeEmulator, "disksave_folder", "disksave", createFolder ? FileHelper::FLAG_CREATE : 0);
    return path + fSetting->file + ".sav";
}

auto FileHelper::truncateMedia(Emulator::Interface::Media* media) -> bool {
    if (!activeEmulator || !media->guid)
        return false;

    auto file = (GUIKIT::File*)media->guid;

    return file->truncate();
}

auto FileHelper::getFileNameFromMedia(Emulator::Interface::Media* media) -> std::string {
    if (!media->guid)
        return "";

    auto file = (GUIKIT::File*)media->guid;

    return file->getFileName(true);
}

auto FileHelper::unloadMedia(Emulator::Interface::Media* media) -> void {
    if (!media->guid)
        return;

    auto file = (GUIKIT::File*)media->guid;

    file->unload();

    filePool->assign(_ident(activeEmulator, media->name + "store"), nullptr);
    filePool->assign(_ident(activeEmulator, media->name), nullptr);

    //filePool->unloadOrphaned();
}

auto FileHelper::updateSaveIdent(Emulator::Interface::Media* media, FileSetting* fSetting) -> void {
    static Emulator::Interface::Media* _media = nullptr;

    if (!media) {
        _media = nullptr;
        return;
    }

    if ( (media->group->isExpansion() && !media->group->expansion->isFastloader() && !media->group->expansion->isTurboCart() && !media->group->expansion->isRam() && !media->parent)
    || (!_media && !media->group->isProgram())
    || (media->group->isDisk() && !_media->group->isDisk() && !_media->group->isExpansion())
    || (media->group->isTape() && !_media->group->isDisk() && !_media->group->isExpansion())) {
        updateSaveIdent( activeEmulator, fSetting );
        _media = media;
    }
}

auto FileHelper::updateSaveIdent( Emulator::Interface* emulator, FileSetting* fSetting ) -> void {
    auto filePath = GUIKIT::File::getPath(fSetting->path);
    auto settings = Program::getSettings( emulator );
    auto autoSaveMode = settings->get<unsigned>( "auto_save_mode", 2);
    std::string fileName;
    std::string fileNameCompare;

    if (autoSaveMode == 2) {
        fileName = fSetting->path;
        fileNameCompare = GUIKIT::String::getFileNameA(fileName, false);
    } else
        fileName = fSetting->file;

    fileName = GUIKIT::String::getFileNameA(fileName, true);

    // for audio record
    settings->set<std::string>( "audio_record_ident", fileName);

    if (!autoSaveMode)
        return;

    if (autoSaveMode == 2) {
        auto list = GUIKIT::File::getFolderListAlt( filePath, {fileName}, true, 10 );
        int matches = list.size();

        std::string tempFn = fileName;
        while(true) {
            if (tempFn.size() < 5)
                break;

            tempFn.pop_back();

            auto list = GUIKIT::File::getFolderListAlt( filePath, {tempFn}, true, 10 );
            if (list.size() != matches) {

                for(auto& str : list) {
                    if (str != fileNameCompare) {
                        if (str.size() == fileNameCompare.size()) {
                            fileName = GUIKIT::String::trim( tempFn );
                            goto End;
                        }
                    }
                }
            }
        }
    }
End:
    settings->set<std::string>( "save_ident", fileName);
    settings->set<unsigned>( "save_slot", 0);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->configurationsLayout)
        emuView->configurationsLayout->updateSaveIdent( fileName );
}

auto FileHelper::updateSaveIdentFromSav( Emulator::Interface* emulator, GUIKIT::File* file ) -> void {
    auto settings = Program::getSettings( emulator );
    std::string fileName = file->getFileName(true, true);

    std::size_t end = fileName.find_last_of('_');

    if (end != std::string::npos)
        fileName = fileName.erase(end);

    settings->set<std::string>( "save_ident", fileName);
    settings->set<unsigned>( "save_slot", 0);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->configurationsLayout)
        emuView->configurationsLayout->updateSaveIdent( fileName );
}

auto FileHelper::generatedFolder(const std::string& subPath, unsigned flags) -> std::string {
    std::string _path;
    std::string _basePath;

    if (program->portable) {
        _path = "portable/" + subPath;
        if ((flags & FLAG_VIEW) == 0)
            _basePath = program->installFolder();
    } else {
        _path = program->appFolder() + "/" + subPath; // may not be created and therefore should not be part of base path
        _basePath = program->userFolder();
    }

    if ((flags & (FLAG_CREATE | FLAG_VIEW)) == FLAG_CREATE)
        GUIKIT::File::createDir(_path, _basePath); // creations starts at base path

    _path = _basePath + _path;


    return GUIKIT::File::beautifyPath(_path);
}

auto FileHelper::generatedFolder(Emulator::Interface* emulator, const std::string& settingIdent, const std::string& subPath, unsigned flags) -> std::string {
    std::string _path;

    if (!settingIdent.empty()) {
        auto settings = Program::getSettings(emulator);
        _path = settings->get<std::string>(settingIdent, "");
        if ((flags & FLAG_VIEW) == 0)
            _path = GUIKIT::File::resolveRelativePath(_path);
    }

    if (_path.empty()) {
        std::string _sub;
        if (!subPath.empty()) {
            std::string _emuIdent = emulator->ident;
            _sub = subPath + "/" + GUIKIT::String::toLowerCase(_emuIdent);
        }
        return generatedFolder(_sub, flags);
    }

    return GUIKIT::File::beautifyPath(_path);
}

auto FileHelper::getSettingsFolder( Emulator::Interface* emulator, unsigned flags ) -> std::string {
    std::string _emuIdent = emulator->ident;
    auto path = globalSettings->get<std::string>( _emuIdent + "_settings_path", "");

    if (path.empty())
        return generatedFolder("settings/" + GUIKIT::String::toLowerCase(_emuIdent), flags);

    if ((flags & FLAG_VIEW) == 0)
        path = GUIKIT::File::resolveRelativePath(path);

    return GUIKIT::File::beautifyPath(path);
}

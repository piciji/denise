
#include "program.h"
#include "tools/chronos.h"

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

    if (!file->exists())
        return false;

    if (!group->isHardDisk() && !file->isSizeValid(MAX_MEDIUM_SIZE))
        return false;
    if (group->isHardDisk() && !file->isSizeValid(MAX_HARDDISK_SIZE))
        return false;
    
    // non archived tape or harddisk images will be loaded in chunks when needed
    if ((group->isTape() || group->isHardDisk()) && !file->isArchived() ) {        
		data = nullptr;
		auto items = file->scanArchive();
		return !items.empty();
	}

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

auto Program::errorFileSize(uint64_t maxSize, std::string filePath, Message* message) -> void {

    message->error(trans->get("file_size_error", {
        { "%path%", filePath},
        { "%size%", GUIKIT::File::SizeFormated(maxSize)}
        }));

    filePool->unloadOrphaned();
}

auto Program::readMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
	if (!media->guid)
		return 0;
    
    auto file = (GUIKIT::File*)media->guid;
    return file->read(buffer, length, offset);
}

auto Program::readAssignedMedia(Emulator::Interface::Media* media, uint8_t*& buffer, bool preview) -> unsigned {
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

auto Program::writeMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
	if (!activeEmulator || !media->guid)
		return 0;
    
    auto file = (GUIKIT::File*)media->guid;
    if (file->isArchived() || file->isReadOnly())
        return 0;

    return file->write(buffer, length, offset);
}

auto Program::writeAssignedMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length) -> unsigned {
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

auto Program::getAssignedSaveFile(Emulator::Interface::Media* media, bool createFolder) -> std::string {
    auto fSetting = FileSetting::getInstance( activeEmulator, _underscore(media->name ) );
    if (!fSetting || fSetting->file.empty())
        return "";
    auto path = generatedFolder(activeEmulator, "disksave_folder", "disksave", createFolder);
    return path + fSetting->file + ".sav";
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

auto Program::unloadMedia(Emulator::Interface::Media* media) -> void {
    if (!media->guid)
        return;

    auto file = (GUIKIT::File*)media->guid;

    file->unload();

    filePool->assign(_ident(activeEmulator, media->name + "store"), nullptr);
    filePool->assign(_ident(activeEmulator, media->name), nullptr);

    //filePool->unloadOrphaned();
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

        if (mediaGroup.isHardDisk()) {
            for (auto& media : mediaGroup.media) {
                auto pcbId = settings->get<unsigned>(_underscore(media.name) + "_pcb", mediaGroup.expansion->pcbs[0].id);

                auto pcbLayout = emulator->getPCB(*mediaGroup.expansion, pcbId);

                media.pcbLayout = pcbLayout ? pcbLayout : &mediaGroup.expansion->pcbs[0];
            }
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

auto Program::updateSaveIdent(Emulator::Interface::Media* media, FileSetting* fSetting) -> void {
    
    static Emulator::Interface::Media* _media = nullptr;
    
    if (!media) {
        _media = nullptr;
        return;
    }        
    
    if ( (media->group->isExpansion() && !media->group->expansion->isFastloader() && !media->group->expansion->isTurboCart() && !media->secondary)
    || (!_media && !media->group->isProgram())
    || (media->group->isDisk() && !_media->group->isDisk() && !_media->group->isExpansion())
    || (media->group->isTape() && !_media->group->isDisk() && !_media->group->isExpansion())) {
        updateSaveIdent( activeEmulator, fSetting );
        _media = media;
    }
}

auto Program::updateSaveIdentFromSav( Emulator::Interface* emulator, GUIKIT::File* file ) -> void {
    auto settings = getSettings( emulator );
    std::string fileName = file->getFileName(true, true);

    std::size_t end = fileName.find_last_of("_");

    if (end != std::string::npos)
        fileName = fileName.erase(end);

    settings->set<std::string>( "save_ident", fileName);
    settings->set<unsigned>( "save_slot", 0);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->configurationsLayout)
        emuView->configurationsLayout->updateSaveIdent( fileName );
}

auto Program::updateSaveIdent( Emulator::Interface* emulator, FileSetting* fSetting ) -> void {
    auto filePath = GUIKIT::File::getPath(fSetting->path);
    auto settings = getSettings( emulator );
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
    portable = true;
    return true;
}

auto Program::saveSettings(bool onExit) -> void {
    bool errorShown = false;

    for (auto settings : settingsStorage) {

        auto guid = settings->getGuid();
        
        std::string path;
        
        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;

            path = cmd->getCustomConfig(emulator);
            if (!path.empty()) {
                if (onExit)
                    continue;
            } else {
                path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");

                if (path.empty()) {
                    path = settingsFileFromEmuFolder(emulator->ident + "_");

                    GUIKIT::File file(path);
                    if (!file.exists())
                        path = settingsFile(emulator->ident + "_");

                } else if (onExit) {
                    continue;
                } else {
                    path = getSettingsFolder(emulator) + path;
                }
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

            std::string customConfig = cmd->getCustomConfig(emulator);

            if (!customConfig.empty()) {
                if (settings->load(customConfig)) {
                    globalSettings->set("last_used_emu", emulator->ident);
                    continue;
                } else
                    cmd->removeCustomConfig(emulator);
            }

            bool lastUsed = globalSettings->get<bool>( emulator->ident + "_load_last_settings" );

            if (lastUsed) {
                std::string path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");
                if (!path.empty()) {
                    path = getSettingsFolder(emulator) + path;

                    if (settings->load(path))
                        continue;
                }
            }

            globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");

            if (!settings->load(settingsFileFromEmuFolder(emulator->ident + "_")))
                settings->load(settingsFile(emulator->ident + "_"));

            unsetObsoleteConfigs(settings, emulator);

        } else {
            if (!settings->load(settingsFileFromEmuFolder("global_"))) {
                settings->load(settingsFile("global_"));
                portable = false;
            } else
                portable = true;

            unsetObsoleteConfigs(settings, nullptr);
        }
    }
}

auto Program::unsetObsoleteConfigs(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void {
    if (!emulator) {
        if (GUIKIT::Application::isWinApi()) {
            if (!settings->get("unset_ds", false)) {
                if (settings->get<std::string>("audio_driver", "") == "DirectSound") {
                    settings->remove("audio_driver");
                    settings->set<unsigned>("audio_latency", 30);
                }
                settings->set("unset_ds", true);
            }
        }
    } else if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        if (!settings->get("update_key_acute", false)) {
            if (settings->get<std::string>("keyboard_95", "") == "")
                settings->set<std::string>("keyboard_95", "0|0|0|13|0");
            settings->set("update_key_acute", true);
        }

        if (!settings->get("update_joy_but3", false)) {
            settings->changeIdent("joypad#1_15", "joypad#1_18");
            settings->changeIdent("joypad#1_15_alt", "joypad#1_18_alt");
            settings->changeIdent("joypad#1_14", "joypad#1_17");
            settings->changeIdent("joypad#1_14_alt", "joypad#1_17_alt");
            settings->changeIdent("joypad#1_13", "joypad#1_16");
            settings->changeIdent("joypad#1_13_alt", "joypad#1_16_alt");
            settings->changeIdent("joypad#1_12", "joypad#1_15");
            settings->changeIdent("joypad#1_12_alt", "joypad#1_15_alt");
            settings->changeIdent("joypad#1_11", "joypad#1_14");
            settings->changeIdent("joypad#1_11_alt", "joypad#1_14_alt");
            settings->changeIdent("joypad#1_10", "joypad#1_13");
            settings->changeIdent("joypad#1_10_alt", "joypad#1_13_alt");

            settings->changeIdent("joypad#1_9", "joypad#1_10");
            settings->changeIdent("joypad#1_9_alt", "joypad#1_10_alt");
            settings->changeIdent("joypad#1_8", "joypad#1_9");
            settings->changeIdent("joypad#1_8_alt", "joypad#1_9_alt");
            settings->changeIdent("joypad#1_7", "joypad#1_8");
            settings->changeIdent("joypad#1_7_alt", "joypad#1_8_alt");
            settings->changeIdent("joypad#1_6", "joypad#1_7");
            settings->changeIdent("joypad#1_6_alt", "joypad#1_7_alt");

            settings->changeIdent("joypad#2_15", "joypad#2_18");
            settings->changeIdent("joypad#2_15_alt", "joypad#2_18_alt");
            settings->changeIdent("joypad#2_14", "joypad#2_17");
            settings->changeIdent("joypad#2_14_alt", "joypad#2_17_alt");
            settings->changeIdent("joypad#2_13", "joypad#2_16");
            settings->changeIdent("joypad#2_13_alt", "joypad#2_16_alt");
            settings->changeIdent("joypad#2_12", "joypad#2_15");
            settings->changeIdent("joypad#2_12_alt", "joypad#2_15_alt");
            settings->changeIdent("joypad#2_11", "joypad#2_14");
            settings->changeIdent("joypad#2_11_alt", "joypad#2_14_alt");
            settings->changeIdent("joypad#2_10", "joypad#2_13");
            settings->changeIdent("joypad#2_10_alt", "joypad#2_13_alt");

            settings->changeIdent("joypad#2_9", "joypad#2_10");
            settings->changeIdent("joypad#2_9_alt", "joypad#2_10_alt");
            settings->changeIdent("joypad#2_8", "joypad#2_9");
            settings->changeIdent("joypad#2_8_alt", "joypad#2_9_alt");
            settings->changeIdent("joypad#2_7", "joypad#2_8");
            settings->changeIdent("joypad#2_7_alt", "joypad#2_8_alt");
            settings->changeIdent("joypad#2_6", "joypad#2_7");
            settings->changeIdent("joypad#2_6_alt", "joypad#2_7_alt");
            settings->set("update_joy_but3", true);
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

auto Program::generatedFolder(const std::string& subPath, bool createFolder) -> std::string {
    std::string _path;
    std::string _basePath;

    if (portable) {
        _path = "portable/" + subPath;
        _basePath = installFolder();
    } else {
        _path = appFolder() + "/" + subPath; // may not be created and therefore should not be part of base path
        _basePath = userFolder();
    }

    if (createFolder)
        GUIKIT::File::createDir(_path, _basePath); // creations starts at base path

    _path = _basePath + _path;
    

    return GUIKIT::File::beautifyPath(_path);
}

auto Program::generatedFolder(Emulator::Interface* emulator, const std::string& settingIdent, const std::string& subPath, bool createFolder) -> std::string {
    std::string _path = "";    

    if (!settingIdent.empty()) {
        auto settings = getSettings(emulator);
        _path = settings->get<std::string>(settingIdent, "");
        _path = GUIKIT::File::resolveRelativePath(_path);
    }

    if (_path.empty()) {
        std::string _sub = "";
        if (!subPath.empty()) {
            std::string _emuIdent = emulator->ident;
            _sub = subPath + "/" + GUIKIT::String::toLowerCase(_emuIdent);
        }
        return generatedFolder(_sub, createFolder);
    }

    return GUIKIT::File::beautifyPath(_path);
}

auto Program::getSettingsFolder( Emulator::Interface* emulator, bool createFolder ) -> std::string {
    std::string _emuIdent = emulator->ident;
    std::string path = globalSettings->get<std::string>( _emuIdent + "_settings_path", "");

    if (path.empty())
        return generatedFolder("settings/" + GUIKIT::String::toLowerCase(_emuIdent), createFolder);

    path = GUIKIT::File::resolveRelativePath(path);

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
    for(auto emulator : emulators) {
        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            GUIKIT::CustomFont font;
            font.name = "C64 Pro";
            font.refPtr = (void*)emulator;
            font.filePath = fontFolder() + "C64_Pro-STYLE121.ttf";
            font.modifier = 0xee << 8;
            bool useCustomFont = GUIKIT::Window::addCustomFont( font );
            ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode(useCustomFont);
            ((LIBC64::Interface*) emulator)->loadWithColumn( getSettings(emulator)->get<bool>("autostart_load_with_column") );

        } else if ( dynamic_cast<LIBAMI::Interface*>(emulator)) {
            GUIKIT::CustomFont font;
            font.name = "TopazPlus a500a1000a2000";
            //font.name = "Topaz a500a1000a2000";
            font.refPtr = (void*)emulator;
            font.filePath = fontFolder() + "TopazPlus_a500_v1.0.ttf";
            //font.filePath = fontFolder() + "Topaz_a500_v1.0.ttf";

            font.sizeAdjust = 1;
            font.modifier = 0;
            GUIKIT::Window::addCustomFont( font );
        }
    }
}

auto Program::libraryMissing(std::string plugin) -> void {
    static unsigned ts = 0;
    if (!initialized)
        return;

    unsigned tsTemp = Chronos::getTimestampInMilliseconds();
    if (tsTemp - ts < 1000)
        return;

    if (plugin == "CAPS") {
        static int informed = 0;
        if (view && (informed < 20) ) {
            view->message->error(trans->get("SPS plugin missing"));
            ts = Chronos::getTimestampInMilliseconds();
            informed++;
        }
    }
}

auto Program::initExpansionRom(Emulator::Interface* emulator, const std::string& ident, const std::string& file) -> void {
    auto fSetting = FileSetting::getInstance( emulator, _underscore(ident) );
    fSetting->setPath(dataFolder() + file);
    fSetting->setFile(file);
    fSetting->setId(0);
    fSetting->setSaveable(false);
}

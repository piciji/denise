
SwapperControlLayout::SwapperControlLayout() {
	append(writeProtect,{0u, 0u}, 10);
    append(insertButton,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(ejectAllButton,{0u, 0u}, 10);
    append(ejectButton,{0u, 0u}, 10);
    append(openButton,{0u, 0u});
	writeProtect.setChecked();
	//writeProtect.setEnabled(false);
    setAlignment(0.5);
}

SwapperInfoLayout::SwapperInfoLayout() {
    append(info1, {0u, 0u}, 20);
    append(info2, {0u, 0u}, 0);
    info2.setForegroundColor(0xff4500);
    setAlignment(0.5);
}

SwapperLayout::SwapperLayout( MediaLayout* mediaLayout ) {
    this->mediaLayout = mediaLayout;
    this->emulator = mediaLayout->emulator;
    
    setMargin(10);
	listView.setHeaderVisible();
	listView.setHeaderText({"", "", ""});

    append(listView,{~0u, ~0u}, 5);
    append(info,{0u, 0u}, 10);
    append(controls,{~0u, 0u});
	
	listView.onChange = [this]() {
		auto pos = listView.selection() + 1;
		auto fSetting = getSetting( pos );
        GUIKIT::File* file = filePool->get(fSetting->path);

        if (file)
            (file->isArchived() || file->isReadOnly()) ? forceWP() : updateWP(fSetting->writeProtect);
        else
            forceWP();
	};
	
	listView.onActivate = [this](){
		controls.openButton.onActivate();	
	};
	
	controls.openButton.onActivate = [this](){
		if(!listView.selected()) return;
        bool errorShown = false;
        
        std::string suffix = "*";
        std::vector<std::string> suffixList;
        auto groups = emulator->getDriveMediaGroups();
        std::string filterIdent = groups.size() > 1 ? "drive image" : "disk_image";
        std::string title = groups.size() > 1 ? "select drive images" : "select disk images";

        for(auto group : groups) {
            GUIKIT::Vector::combine(suffixList, group->suffix);
        }

        GUIKIT::Vector::combine(suffixList, GUIKIT::File::suppportedCompressionExtensions());
        suffix = GUIKIT::BrowserWindow::transformFilter(trans->get(filterIdent), suffixList );

		std::vector<std::string> filePaths = GUIKIT::BrowserWindow()
			.setWindow( *(this->mediaLayout->tabWindow) )
			.setTitle( trans->get(title) )
			.setPath( preselectPath( ) )
			.setFilters({ suffix,
				trans->get("all_files")})
            .showOrderControlForMultipleSelections( this->mediaLayout->settings->get<bool>( "swapper_order_selected", false ), trans->get("order selected"), [this](bool checked) {
                this->mediaLayout->settings->set<bool>( "swapper_order_selected", checked );
            } )
			.openMulti();

		if (!filePaths.size() || filePaths[0].empty()) return;

        unsigned startPos = listView.selection() + 1;
        unsigned pos = startPos;

        for(auto& filePath : filePaths) {
            clearSlot( pos );

            GUIKIT::File* file = filePool->get(filePath);

            savePath(file->getPath());

            if (!file->exists() || !file->isSizeValid(MAX_MEDIUM_SIZE)) {
                if (!errorShown) {
                    errorShown = true;
                    program->errorMediumSize(file, this->mediaLayout->message);
                }
                continue;
            }

            auto& items = file->scanArchive();

            if (filePaths.size() == 1) {
                archiveViewer->onCallback = [this, file](GUIKIT::File::Item* item) {
                    emuThread->lock();
                    if (!item || (item->info.size == 0))
                        return this->mediaLayout->message->error( trans->get(file->isArchived() ? "archive_error" : "file_open_error",
                            {{"%path%", file->getFile()}}));

                    if (!listView.selected()) return;
                    auto pos = listView.selection() + archiveViewer->filesSelected + 1;

                    filePool->assign(_ident(emulator, "swapper_" + std::to_string(pos)), file);

                    auto fSetting = getSetting(pos);
                    fSetting->setPath(file->getFile());
                    fSetting->setFile(item->info.name);
                    fSetting->setId(item->id);
                    listView.setText(pos - 1, {std::to_string(pos), file->getFile(), item->info.name});
                    (file->isArchived() || file->isReadOnly()) ? forceWP() : updateWP(false);

                    if (++pos == SWAPPER_SLOTS)
                        archiveViewer->setVisible(false);

                    emuThread->unlock();
                };
                archiveViewer->setView(items, true);
            } else {
                for(auto& item : items) {
                    if (item.info.size == 0)
                        continue;

                    std::string _fn = item.info.name;
                    GUIKIT::String::toLowerCase( _fn );

                    std::string extension = GUIKIT::String::getExtension(_fn, "exe");
                    GUIKIT::String::toLowerCase( extension );

                    if (!GUIKIT::Vector::find(suffixList, extension))
                        continue;

                    filePool->assign(_ident(emulator, "swapper_" + std::to_string(pos)), file);

                    auto fSetting = getSetting(pos);
                    fSetting->setPath(file->getFile());
                    fSetting->setFile(item.info.name);
                    fSetting->setId(item.id);
                    listView.setText(pos - 1, {std::to_string(pos), file->getFile(), item.info.name});

                    if (pos == startPos)
                        (file->isArchived() || file->isReadOnly()) ? forceWP() : updateWP(false);

                    if (++pos == SWAPPER_SLOTS)
                        return;
                }
            }
        }
	};
	
	controls.ejectButton.onActivate = [this]() {
		if(!listView.selected()) return;
        clearSlot( listView.selection() + 1 );
	};

    controls.insertButton.onActivate = [this]() {
        if(!listView.selected()) return;

        emuThread->lock();
        Emulator::Interface::Media* media = fileloader->insertSwapDisk( emulator, listView.selection() + 1 );
        if (media) {
            std::string traps = "autostart_traps_on_dblclick";
            if (media->group->isTape())
                traps = "autostart_tape_traps_on_dblclick";

            auto settings = program->getSettings(emulator);
            fileloader->autoload(emulator, media, 0, settings->get<bool>(traps, false));
        }
        emuThread->unlock();
    };

    controls.ejectAllButton.onActivate = [this]() {
        for (unsigned i = 1; i < SWAPPER_SLOTS; i++)
            clearSlot( i );
    };

	controls.writeProtect.onToggle = [this](bool checked) {
		if(!listView.selected()) return;
        auto fSetting = getSetting( listView.selection() + 1 );

        fSetting->setWriteProtect( checked );
	};
	
	for(unsigned i = 1; i < SWAPPER_SLOTS; i++) {
		auto fSetting = getSetting( i );		
		listView.append({std::to_string(i), fSetting->path, fSetting->file });
	}        
}

auto SwapperLayout::loadSettings() -> void {
    listView.reset();
    
    for (unsigned i = 1; i < SWAPPER_SLOTS; i++) {
        auto fSetting = getSetting(i);
        fSetting->update();
        listView.append({std::to_string(i), fSetting->path, fSetting->file});
    }
}

auto SwapperLayout::translate() -> void {
    listView.setHeaderText({"#", trans->get("path"), trans->get("file")});
    controls.openButton.setText(trans->get("open"));
    controls.openButton.setTooltip(trans->get("swapper open hint"));
    controls.insertButton.setText(trans->get("insert and load"));
    controls.ejectButton.setText(trans->get("eject"));
    controls.ejectAllButton.setText(trans->get("eject all"));
	controls.writeProtect.setText(trans->get("write_protected"));

    info.info1.setText( trans->get("swapper multi hint") );
    info.info1.setTooltip( trans->get("swapper multi hint tooltip") );

    info.info2.setText( trans->get( emulator->getTapeMediaGroup() ? "guess media" : "guess disks") );
    info.info2.setTooltip( trans->get("guess media tooltip") );
}

auto SwapperLayout::getSetting( unsigned pos ) -> FileSetting* {
	return FileSetting::getInstance( emulator, "swapper_" + std::to_string( pos ) );
}

auto SwapperLayout::preselectPath( ) -> std::string {
	
	auto path = mediaLayout->settings->get<std::string>( "disk_folder_swap", "" );	
	
	return path;
}

auto SwapperLayout::savePath( std::string path ) -> void {
	
	mediaLayout->settings->set<std::string>("disk_folder_swap", path);
}

auto SwapperLayout::clearSlot(unsigned pos) -> void {
    filePool->assign( _ident(emulator, "swapper_" + std::to_string(pos)), nullptr);
    filePool->unloadOrphaned();

    auto fSetting = getSetting( pos );
    fSetting->init();

    listView.setText(pos - 1, {std::to_string(pos), "", ""});
    forceWP();
}

auto SwapperLayout::updateWP(bool state, bool force) -> void {
    if (force) state = true;

    if (controls.writeProtect.checked() != state)
        controls.writeProtect.setChecked(state);

   // if (controls.writeProtect.enabled() != !force)
     //   controls.writeProtect.setEnabled(!force);
}

SwapperControlLayout::SwapperControlLayout() {
	append(writeProtect,{0u, 0u});
    append(spacer,{~0u, 0u});	
    append(openButton,{0u, 0u}, 10);
    append(ejectButton,{0u, 0u});
	writeProtect.setChecked();
	writeProtect.setEnabled(false);
}

SwapperLayout::SwapperLayout( TabWindow* tabWindow ) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);
	listView.setHeaderVisible();
	listView.setHeaderText({"", "", ""});	

    append(listView,{~0u, ~0u}, 10);
    append(controls,{~0u, 0u});
	
	listView.onChange = [this]() {
		auto pos = listView.selection();
		auto setting = getSetting( pos );
		controls.writeProtect.setChecked( setting->writeProtect );
		controls.writeProtect.setEnabled( setting->wpEnabled );
	};
	
	listView.onActivate = [this](){
		controls.openButton.onActivate();	
	};
	
	controls.openButton.onActivate = [this](){
		if(!listView.selected()) return;
        
        std::string suffix = "*";
        std::string folder = "";
        for(auto& mediaGroup : emulator->mediaGroups) {
            if (mediaGroup.isDisk()) {
                auto _suffix = mediaGroup.suffix;
                GUIKIT::Vector::combine(_suffix, GUIKIT::File::suppportedCompressionExtensions());    
                suffix = GUIKIT::BrowserWindow::transformFilter(trans->get("disk_image"), _suffix );
                folder = mediaGroup.name;
                break;
            }                
        }
                	
		std::string filePath = GUIKIT::BrowserWindow()
			.setWindow( *this->tabWindow )
			.setTitle( trans->get("select_disk_image") )
			.setPath( preselectPath( folder ) )
			.setFilters({ suffix,
				trans->get("all_files")})
			.open();
		if (filePath.empty()) return;
		
		controls.ejectButton.onActivate();			
		GUIKIT::File* file = filePool->get(filePath);
        
        savePath( folder, file->getPath() );

		if (!file->isSizeValid(MAX_MEDIUM_SIZE))
            return program->errorMediumSize( file, mes );  
		
		auto& items = file->scanArchive();

		archiveViewer->onCallback = [this, file](GUIKIT::File::Item* item) {
			if (!item || (item->info.size == 0))
				return mes->error(trans->get(file->isArchived() ? "archive_error" : "file_open_error", {{"%path%", file->getFile()}}));			
            
			if(!listView.selected()) return;
			auto pos = listView.selection();
			
			filePool->assign( this->tabWindow->ident("swapper_" + std::to_string(pos)), file);
			
			auto setting = getSetting( pos );
			setting->setPath( file->getFile() );
			setting->setFile( item->info.name );
			setting->setId( item->id );
			setting->setWriteProtect( true );
			setting->setWpEnabled( !file->isArchived() );

			listView.setText(pos, {std::to_string(pos), file->getFile(), item->info.name});
			controls.writeProtect.setEnabled();	
		};
		archiveViewer->setView(items);
	};
	
	controls.ejectButton.onActivate = [this]() {
		if(!listView.selected()) return;
		auto pos = listView.selection();
		filePool->assign( this->tabWindow->ident("swapper_" + std::to_string(pos)), nullptr);
        filePool->unloadOrphaned();
		
		auto setting = getSetting( pos );
		setting->init();
		
		listView.setText(pos, {std::to_string(pos), "", ""});
		controls.writeProtect.setChecked();
		controls.writeProtect.setEnabled(false);		
	};

	controls.writeProtect.onToggle = [&]() {
		if(!listView.selected()) return;
		auto pos = listView.selection();
        auto setting = getSetting( pos );
		
		bool state = controls.writeProtect.checked();
		if (!state) {			
			if ( !setting->wpEnabled ) {
				controls.writeProtect.setChecked();
				controls.writeProtect.setEnabled(false);
				mes->warning(trans->get("archive_wp_tooltip"));
				return;
			}
		}
        setting->setWriteProtect( state );        
	};
	
	for(unsigned i = 0; i < 15; i++) {
		auto setting = getSetting( i );		
		listView.append({std::to_string(i), setting->path, setting->file });
	}
}

auto SwapperLayout::translate() -> void {
    listView.setHeaderText({"#", trans->get("path"), trans->get("file")});
    controls.openButton.setText(trans->get("open"));
    controls.ejectButton.setText(trans->get("eject"));
	controls.writeProtect.setText(trans->get("write_protected"));
}

auto SwapperLayout::getSetting( unsigned pos ) -> FileSetting* {
	return FileSetting::getInstance( tabWindow->ident("swapper_" + std::to_string( pos ) ) );
}

auto SwapperLayout::preselectPath( std::string& groupName ) -> std::string {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );

	auto path = settings->get<std::string>( baseFolderIdent, "" );
	
	if ( path == "" )
		path = settings->get<std::string>( baseFolderIdent + "_swap", "" );
	
	return path;
}

auto SwapperLayout::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );
	
	settings->set<std::string>(baseFolderIdent + "_swap", path);
}
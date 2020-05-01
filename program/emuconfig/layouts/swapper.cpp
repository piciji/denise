
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
	};
	
	listView.onActivate = [this](){
		controls.openButton.onActivate();	
	};
	
	controls.openButton.onActivate = [this](){
		if(!listView.selected()) return;
        
        std::string suffix = "*";

        for(auto& mediaGroup : emulator->mediaGroups) {
            if (mediaGroup.isDisk()) {
                auto _suffix = mediaGroup.suffix;
                GUIKIT::Vector::combine(_suffix, GUIKIT::File::suppportedCompressionExtensions());    
                suffix = GUIKIT::BrowserWindow::transformFilter(trans->get("disk_image"), _suffix );
                break;
            }                
        }
                	
		std::string filePath = GUIKIT::BrowserWindow()
			.setWindow( *this->tabWindow )
			.setTitle( trans->get("select_disk_image") )
			.setPath( preselectPath( ) )
			.setFilters({ suffix,
				trans->get("all_files")})
			.open();
		if (filePath.empty()) return;
		
		controls.ejectButton.onActivate();			
		GUIKIT::File* file = filePool->get(filePath);
        
        savePath( file->getPath() );

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
            listView.setText(pos, {std::to_string(pos), file->getFile(), item->info.name});
            
            bool forceWP = file->isArchived() || file->isReadOnly();
            
			setting->setWriteProtect( forceWP );		
			controls.writeProtect.setEnabled( !forceWP );
            controls.writeProtect.setChecked( forceWP );
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

auto SwapperLayout::preselectPath( ) -> std::string {
	
	auto baseFolderIdent = tabWindow->ident( "disk_folder_swap" );

	auto path = settings->get<std::string>( baseFolderIdent, "" );	
	
	return path;
}

auto SwapperLayout::savePath( std::string path ) -> void {
	
	auto baseFolderIdent = tabWindow->ident( "disk_folder_swap" );
	
	settings->set<std::string>(baseFolderIdent, path);
}
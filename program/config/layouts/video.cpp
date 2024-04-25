
VideoSettingsLayout::VideoSettingsLayout() {
    append(exclusiveFullscreen, {0u, 0u}, 10);
    append(hardSync, {0u, 0u}, 10);
    append(threadedRenderer, {0u, 0u}, 10);
    append(trOn, {0u, 0u}, 10);
    append(trAuto, {0u, 0u});
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

VideoLayout::VideoLayout() {
    setMargin(10);

    bool showExclusiveFullscreenCheck = false;
    bool showHardSync = false;
    
	auto selectedDriver = program->getVideoDriver();
	unsigned i = 0;
	for(auto& driver : videoDriver->available()) {
		driverLayout.combo.append( driver );
		if (driver == selectedDriver) {
			driverLayout.combo.setSelection( i );
		}
        if(GUIKIT::String::foundSubStr(driver, "Direct3D"))
            showExclusiveFullscreenCheck = true;
        
        if(GUIKIT::String::foundSubStr(driver, "GL") || GUIKIT::String::foundSubStr(driver, "Direct3D11"))
            showHardSync = true;
        
		i++;
	}
	
	driverLayout.combo.onChange = [this]() {
		globalSettings->set<std::string>("video_driver", driverLayout.combo.text() );

        emuThread->lock();
        // disable fastforward before, otherwise if threaded CPU CRT is selected, the midline callback would be active with fastforward.
        // fastforward will not render all frames but mid scanline callback runs without finishing the frame
        // todo: solve this better, more self-acting
        program->fastForward(false);
        program->initVideo(true);
        updateDriverPropsVisibility();
        emuThread->unlock();
	};

    append(driverLayout, {~0u, 0u}, 5);
    append(videoSettingsLayout, {~0u, 0u}, 5);
    
	if (driverLayout.combo.rows() > 0) append(driverLayout, {~0u, 0u}, 5);
    if (driverLayout.combo.rows() == 1) driverLayout.setEnabled(false);
	
    if( showExclusiveFullscreenCheck ) {        
        videoSettingsLayout.exclusiveFullscreen.setEnabled(false);

        if (videoDriver->canExclusiveFullscreen()) {
            videoSettingsLayout.exclusiveFullscreen.setEnabled( !globalSettings->get<bool>("threaded_emu", false) );
            videoSettingsLayout.exclusiveFullscreen.setChecked(globalSettings->get("exclusive_fullscreen", false));
        }
    } else
        videoSettingsLayout.remove( videoSettingsLayout.exclusiveFullscreen );
    
    if( showHardSync ) {
        videoSettingsLayout.hardSync.setEnabled(videoDriver->canHardSync());
        videoSettingsLayout.hardSync.setChecked(globalSettings->get("hardsync", false));
    } else
        videoSettingsLayout.remove( videoSettingsLayout.hardSync );
	
	videoSettingsLayout.exclusiveFullscreen.onToggle = [](bool checked) {
        emuThread->lock();
		globalSettings->set("exclusive_fullscreen", checked);

		program->hintExclusiveFullscreen();
        emuThread->unlock();
	};
    
    videoSettingsLayout.hardSync.onToggle = [](bool checked) {
        emuThread->lock();
		globalSettings->set("hardsync", checked);
		videoDriver->hardSync( checked );
        emuThread->unlock();
	};

    videoSettingsLayout.trOn.onToggle = [this](bool checked) {
        emuThread->lock();
        globalSettings->set("threaded_renderer", checked);
        VideoManager::setSynchronize();
        emuThread->unlock();

        videoSettingsLayout.trAuto.setEnabled( !checked );
        view->threadedRendererWasToggled(checked);
    };

    videoSettingsLayout.trOn.setChecked( globalSettings->get("threaded_renderer", true) );

    videoSettingsLayout.trAuto.onToggle = [this](bool checked) {
        emuThread->lock();
        globalSettings->set("adaptive_sync", checked);
        program->fastForward( false );
        VideoManager::setSynchronize();
        emuThread->unlock();

        view->threadedRendererWasToggled( videoSettingsLayout.trOn.checked() );
    };

    videoSettingsLayout.trAuto.setChecked( globalSettings->get("adaptive_sync", false) );
    videoSettingsLayout.trAuto.setEnabled( !globalSettings->get("threaded_renderer", true) );
}

auto VideoLayout::translate() -> void {
	
	driverLayout.name.setText( trans->get("driver", {}, true) );
	videoSettingsLayout.exclusiveFullscreen.setText( trans->get("exclusive_fullscreen") );
	videoSettingsLayout.exclusiveFullscreen.setTooltip( trans->get("exclusive_fullscreen_tooltip") );
    videoSettingsLayout.hardSync.setText( trans->get("hard_sync") );
	videoSettingsLayout.hardSync.setTooltip( trans->get("hard_sync_tooltip") );
    videoSettingsLayout.threadedRenderer.setText( trans->getA("Threaded Renderer", true) );
    videoSettingsLayout.trOn.setText( trans->getA("enabled") );
    videoSettingsLayout.trOn.setTooltip( trans->getA("Threaded Renderer tooltip") );
    videoSettingsLayout.trAuto.setText( trans->getA("Threaded Renderer Auto") );
    videoSettingsLayout.setText( trans->get("driver_properties") );
    
    driverLayout.name.setText( trans->get("driver", {}, true) );
}


auto VideoLayout::updateDriverPropsVisibility() -> void {
    auto threadedEmu = globalSettings->get<bool>("threaded_emu", false);

    videoSettingsLayout.exclusiveFullscreen.setEnabled( !threadedEmu && videoDriver->canExclusiveFullscreen() );
    videoSettingsLayout.hardSync.setEnabled( videoDriver->canHardSync() );
}

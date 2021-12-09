
CrtEmulationLayout::CrtEmulationLayout() {
    append( threadMode, {0u, 0u}, 5 );
    append( shaderInputPrecision, {0u, 0u} );
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

InScreenTextLayout::InScreenTextLayout() {
    append(option1, {0u, 0u}, 5);
    append(option2, {0u, 0u}, 5);
    append(option3, {0u, 0u});
    
    GUIKIT::RadioBox::setGroup(option1, option2, option3 );
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

VideoGeometryLayout::VideoGeometryLayout() {
	append( aspectCorrect, {0u, 0u}, 5 );
	append( integerScaling, {0u, 0u} );
	
	setPadding(10);
	setFont(GUIKIT::Font::system("bold"));
}

PathsLayout::Block::Block() {
    edit.setEditable(false);
    append(label, {0u, 0u}, 10);
    append(edit, {~0u, 0u}, 10);
    append(empty, {0u, 0u}, 10);
    append(select, {0u, 0u});
    setAlignment(0.5);
}

PathsLayout::PathsLayout() {
    setPadding(10);
	append(shader, {~0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
}

VideoSettingsLayout::VideoSettingsLayout() {
    
    append(exclusiveFullscreen, {0u, 0u}, 10);
    append(hardSync, {0u, 0u}, 10);    
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

VideoResolutionLayout::VideoResolutionLayout() : displaySettings(true) {
    append(active, {0u, 0u}, 10 );
    append(display, {0u, 0u}, 10 );
    append(displaySettings, {0u, 0u} );
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

VideoFpsLayout::Options::Options() {
    append(labelDecimalPlace, {0u, 0u}, 5);
    append(Zero, {0u, 0u}, 5);
    append(One, {0u, 0u}, 5);
    append(Two, {0u, 0u}, 5);
    append(Three, {0u, 0u});

    GUIKIT::RadioBox::setGroup( Zero, One, Two, Three );

    setAlignment(0.5);
}

VideoFpsLayout::VideoFpsLayout() : updateDelay("ms") {

    append(updateDelay, {~0u, 0u}, 2);
    append(options, {~0u, 0u}, 2);

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    updateDelay.slider.setLength(25);
    updateDelay.updateValueWidth( "5000 " + updateDelay.unit );
}

VideoLayout::VideoLayout() {
    setMargin(10);
	
    // append only if direct3d is present
    bool showExclusiveFullscreenCheck = false;
    // append only if GL is present
    bool showHardSync = false;
    
	auto selectedDriver = program->getVideoDriver();
	unsigned i = 0;
	for(auto& driver : videoDriver->available()) {
		driverLayout.combo.append( driver );
		if (driver == selectedDriver) {
			driverLayout.combo.setSelection( i );
		}
        if(driver == "Direct3D")
            showExclusiveFullscreenCheck = true;
        
        if(GUIKIT::String::foundSubStr(driver, "GL"))
            showHardSync = true;
        
		i++;
	}
	
	driverLayout.combo.onChange = [this]() {
		globalSettings->set<std::string>("video_driver", driverLayout.combo.text() );
        
		globalSettings->set("exclusive_fullscreen", false);
		videoSettingsLayout.exclusiveFullscreen.setEnabled(false);
		videoSettingsLayout.exclusiveFullscreen.setChecked(false);
        videoSettingsLayout.hardSync.setEnabled(false);
		
        auto selected = driverLayout.combo.text();
        
		if (selected == "Direct3D") {
			videoSettingsLayout.exclusiveFullscreen.setEnabled();			
            
		} else if(GUIKIT::String::foundSubStr(selected, "GL")) {
            videoSettingsLayout.hardSync.setEnabled();
        }

        for (auto emulator : emulators) {
            program->getSettings(emulator)->set<std::string>( "shader", "");
            auto vManager = VideoManager::getInstance(emulator);
            vManager->shader.loadExternal();
		}
		
        VideoManager::unlockDriver();
		view->updateShader();        
        program->initVideo();    		
	};

    append(videoResolution, {~0u, 0u}, 10);
    append(paths, {~0u, 0u}, 10);
    append(driverLayout, {~0u, 0u}, 5);
    append(videoSettingsLayout, {~0u, 0u}, 5);

    videoResolution.display.onChange = [this]() {

        auto displayId = videoResolution.display.userData();

        videoResolution.displaySettings.reset();

        for( auto& resolution : GUIKIT::Monitor::getSettings( displayId ) )
            videoResolution.displaySettings.append( resolution.name, resolution.id );

        globalSettings->set<unsigned>("fullscreen_display", (unsigned)videoResolution.display.userData() );

        globalSettings->set<unsigned>("fullscreen_setting", 0 );

        program->updateFullscreenSetting();

        videoResolution.synchronizeLayout();
    };

    videoResolution.displaySettings.onChange = [this]() {

        globalSettings->set<unsigned>("fullscreen_setting", videoResolution.displaySettings.userData() );

        program->updateFullscreenSetting();
    };

    videoResolution.active.onToggle = [this](bool checked) {
        globalSettings->set<bool>("fullscreen_setting_active", checked);

        program->updateFullscreenSetting();

        videoResolution.display.setEnabled( checked );
        videoResolution.displaySettings.setEnabled( checked );
    };

    videoResolution.active.setChecked( globalSettings->get<bool>("fullscreen_setting_active", false) );

    for( auto& display : GUIKIT::Monitor::getDisplays() )
        videoResolution.display.append(display.name, display.id);

    auto displayId = globalSettings->get<unsigned>("fullscreen_display", 0 );

    videoResolution.display.setSelectionByUserId( displayId );

    for( auto& resolution : GUIKIT::Monitor::getSettings( displayId ) )
        videoResolution.displaySettings.append( resolution.name, resolution.id );

    videoResolution.displaySettings.setSelectionByUserId( globalSettings->get<unsigned>("fullscreen_setting", 0 ) );

    videoResolution.display.setEnabled( videoResolution.active.checked() );
    videoResolution.displaySettings.setEnabled( videoResolution.active.checked() );
    
	if (driverLayout.combo.rows() > 0) append(driverLayout, {~0u, 0u}, 5);
    if (driverLayout.combo.rows() == 1) driverLayout.setEnabled(false);
	
    if( showExclusiveFullscreenCheck ) {        
        videoSettingsLayout.exclusiveFullscreen.setEnabled(false);

        if (selectedDriver == "Direct3D") {
            videoSettingsLayout.exclusiveFullscreen.setEnabled();
            videoSettingsLayout.exclusiveFullscreen.setChecked(globalSettings->get("exclusive_fullscreen", false));
        }
    } else
        videoSettingsLayout.remove( videoSettingsLayout.exclusiveFullscreen );
    
    if( showHardSync ) {        
        videoSettingsLayout.hardSync.setEnabled(false);
        videoSettingsLayout.hardSync.setChecked(globalSettings->get("gl_hardsync", false));
        
        if(GUIKIT::String::foundSubStr(selectedDriver, "GL")) {
            videoSettingsLayout.hardSync.setEnabled();            
        }
    } else
        videoSettingsLayout.remove( videoSettingsLayout.hardSync );
	
	videoSettingsLayout.exclusiveFullscreen.onToggle = [](bool checked) {
		globalSettings->set("exclusive_fullscreen", checked);
		program->hintExclusiveFullscreen();
	};
    
    videoSettingsLayout.hardSync.onToggle = [](bool checked) {
		globalSettings->set("gl_hardsync", checked);
		videoDriver->hardSync( checked );
	};
    
    std::function<bool (PathsLayout::Block*, const std::string&, const std::string&)> selectPath;
	
	selectPath = [&](PathsLayout::Block* block, const std::string& title, const std::string& savekey) -> bool {		
		auto path = GUIKIT::BrowserWindow()
        .setTitle( trans->get( title ) )
        .setWindow( *configView )
        .directory();

        if(!path.empty()) {
            globalSettings->set<std::string>( savekey, path);
            block->edit.setText( path );
			return true;
        }
		return false;
	};
	
	paths.shader.select.onActivate = [&, selectPath]() {
		if (selectPath(&paths.shader, "select_shader_folder", "shader_folder")) {
            for (auto emulator : emulators) {
                program->getSettings(emulator)->set<std::string>( "shader", "");
                auto vManager = VideoManager::getInstance(emulator);
                vManager->shader.loadExternal();
			}
                			
			view->updateShader();
			
			if (activeVideoManager)
				activeVideoManager->shader.sendToDriver();			
		}
	};
	
	paths.shader.empty.onActivate = [&]() {
        globalSettings->set<std::string>("shader_folder", "");
        paths.shader.edit.setText( "" );
        
        for (auto emulator : emulators) {
            program->getSettings(emulator)->set<std::string>("shader", "");
            auto vManager = VideoManager::getInstance(emulator);
            vManager->shader.loadExternal();
		}

        view->updateShader();
        
		if (activeVideoManager)
			activeVideoManager->shader.sendToDriver();	
    };

	paths.shader.edit.setText( globalSettings->get<std::string>("shader_folder", "") );
    
	hLayout.append(videoGeometry, {~0u, 0u}, 20);
    hLayout.append(screenTextLayout, {~0u, 0u}, 20);
    hLayout.append(crtEmulation, {~0u, 0u});
    append(hLayout, {~0u, 0u}, 10);
    append(videoFps, {~0u, 0u});

    videoFps.updateDelay.slider.onChange = [this]() {

        unsigned position = videoFps.updateDelay.slider.position();
        position = (position + 1) * 200;

        globalSettings->set<unsigned>("fps_update", position);

        videoFps.updateDelay.value.setText( std::to_string(position) + " " + videoFps.updateDelay.unit );
    };

    unsigned fpsUpdate = globalSettings->get<unsigned>("fps_update", 1000u, {200u, 5000u});
    videoFps.updateDelay.slider.setPosition( fpsUpdate / 200 - 1 );
    videoFps.updateDelay.value.setText( std::to_string( fpsUpdate ) + " " + videoFps.updateDelay.unit );

    videoFps.options.Zero.setText("0");
    videoFps.options.Zero.onActivate = [this]() {
        globalSettings->set<unsigned>("fps_decimal_point", 0);
        statusHandler->statusBar->updateDimension( 15, "1000" );
    };
    videoFps.options.One.setText("1");
    videoFps.options.One.onActivate = [this]() {
        globalSettings->set<unsigned>("fps_decimal_point", 1);
        statusHandler->statusBar->updateDimension( 15, "1000.9" );
    };
    videoFps.options.Two.setText("2");
    videoFps.options.Two.onActivate = [this]() {
        globalSettings->set<unsigned>("fps_decimal_point", 2);
        statusHandler->statusBar->updateDimension( 15, "1000.99" );
    };
    videoFps.options.Three.setText("3");
    videoFps.options.Three.onActivate = [this]() {
        globalSettings->set<unsigned>("fps_decimal_point", 3);
        statusHandler->statusBar->updateDimension( 15, "1000.999" );
    };

    unsigned countDecimal = globalSettings->get<unsigned>("fps_decimal_point", 3, {0u, 3u});
    switch (countDecimal) {
        case 0: videoFps.options.Zero.setChecked(); break;
        case 1: videoFps.options.One.setChecked(); break;
        case 2: videoFps.options.Two.setChecked(); break;
        case 3: videoFps.options.Three.setChecked(); break;
    }

    screenTextLayout.option1.onActivate = [this]() {
        globalSettings->set("video_screen_text", 0);
        statusHandler->transferToOSD("");
    };
    
    screenTextLayout.option2.onActivate = [this]() {
        globalSettings->set("video_screen_text", 1);
        statusHandler->transferToOSD("");
    };
    
    screenTextLayout.option3.onActivate = [this]() {
        globalSettings->set("video_screen_text", 2);
        statusHandler->transferToOSD("");
    };
    
    if(globalSettings->get("video_screen_text", 0) == 0) screenTextLayout.option1.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 1) screenTextLayout.option2.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 2) screenTextLayout.option3.setChecked();

    videoGeometry.aspectCorrect.setChecked( globalSettings->get<bool>("aspect_correct", true) );
    videoGeometry.aspectCorrect.onToggle = [&](bool checked) {
        globalSettings->set<bool>("aspect_correct", checked);
		VideoManager::setAspectCorrect( checked );
        view->updateViewport();
    };
	
	videoGeometry.integerScaling.setChecked( globalSettings->get<bool>("integer_scaling", false) );
    videoGeometry.integerScaling.onToggle = [&](bool checked) {
        globalSettings->set<bool>("integer_scaling", checked);
		VideoManager::setIntegerScaling( checked );
        view->updateViewport();
    };
	
	crtEmulation.threadMode.setChecked( globalSettings->get<bool>("crt_threaded", true) );
	crtEmulation.threadMode.onToggle = [this](bool checked) {
        globalSettings->set<bool>("crt_threaded", checked);
        VideoManager::setThreaded( checked );
    };
    
	crtEmulation.shaderInputPrecision.setChecked( globalSettings->get<bool>("crt_shader_input_precision", false) );
    crtEmulation.shaderInputPrecision.onToggle = [this](bool checked) {
        globalSettings->set<bool>("crt_shader_input_precision", checked);
        VideoManager::setShaderInputPrecision( checked );
    };
}

auto VideoLayout::translate() -> void {
	
	driverLayout.name.setText( trans->get("driver", {}, true) );
	videoSettingsLayout.exclusiveFullscreen.setText( trans->get("exclusive_fullscreen") );
	videoSettingsLayout.exclusiveFullscreen.setTooltip( trans->get("exclusive_fullscreen_tooltip") );
    videoSettingsLayout.hardSync.setText( trans->get("hard_sync") );
	videoSettingsLayout.hardSync.setTooltip( trans->get("hard_sync_tooltip") );
    videoSettingsLayout.setText( trans->get("driver_properties") );
    
    screenTextLayout.option1.setText( trans->get("disabled") );
    screenTextLayout.option2.setText( trans->get("intelligent") );
    screenTextLayout.option2.setTooltip( trans->get("tip_intelligent_screentext") );
    screenTextLayout.option3.setText( trans->get("enabled") );
    screenTextLayout.setText( trans->get("screen_status") );
    
    crtEmulation.threadMode.setText( trans->get("crt_threaded") );
    crtEmulation.shaderInputPrecision.setText( trans->get("color_channel_32bit") );
    crtEmulation.setText( trans->get("crt_emulation") );
	
	videoGeometry.setText(trans->get("geometry"));
	videoGeometry.aspectCorrect.setText(trans->get("aspect_ratio"));
	videoGeometry.integerScaling.setText(trans->get("integer_scaling"));
    
    driverLayout.name.setText( trans->get("driver", {}, true) );
    
    paths.setText( trans->get("paths") );
    paths.shader.label.setText(trans->get("Shader",{}, true));
	paths.shader.select.setText(trans->get("select"));
	paths.shader.empty.setText(trans->get("remove"));

    videoResolution.setText( trans->get("preselect fullscreen resolution") );
    videoResolution.active.setText( trans->get("enable") );

    videoFps.setText( trans->get("FPS") );
    videoFps.updateDelay.name.setText( trans->get("Refresh", {}, true) );
    videoFps.options.labelDecimalPlace.setText( trans->get("Decimal Place", {}, true) );

}


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

VideoFrameAdjustLayout::VideoFrameAdjustLayout() {
    append(overrideExactFrequency, {0u, 0u}, 10);
    append(pal, {0u, 0u}, 5);
    append(palFrequency, {60u, 0u}, 10);
    append(ntsc, {0u, 0u}, 5);
    append(ntscFrequency, {60u, 0u}, 10);
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

VideoSettingsLayout::VideoSettingsLayout() {
    
    append(exclusiveFullscreen, {0u, 0u}, 10);
    append(hardSync, {0u, 0u}, 10);    
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
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
    
    append(paths, {~0u, 0u}, 10);
    append(videoFrameAdjust, {~0u, 0u}, 10);
    append(driverLayout, {~0u, 0u}, 5);
    append(videoSettingsLayout, {~0u, 0u}, 5);
    
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
	
	videoSettingsLayout.exclusiveFullscreen.onToggle = [this]() {
		globalSettings->set("exclusive_fullscreen", videoSettingsLayout.exclusiveFullscreen.checked());
		program->hintExclusiveFullscreen();
	};
    
    videoSettingsLayout.hardSync.onToggle = [this]() {
		globalSettings->set("gl_hardsync", videoSettingsLayout.hardSync.checked());
		videoDriver->hardSync( videoSettingsLayout.hardSync.checked() );
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
    append(hLayout, {~0u, 0u});
        
    screenTextLayout.option1.onActivate = [this]() {
        globalSettings->set("video_screen_text", 0);
        view->setStatusText("");
    };
    
    screenTextLayout.option2.onActivate = [this]() {
        globalSettings->set("video_screen_text", 1);
        view->setStatusText("");
    };
    
    screenTextLayout.option3.onActivate = [this]() {
        globalSettings->set("video_screen_text", 2);
        view->setStatusText("");
    };
    
    if(globalSettings->get("video_screen_text", 0) == 0) screenTextLayout.option1.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 1) screenTextLayout.option2.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 2) screenTextLayout.option3.setChecked();
    
    videoFrameAdjust.overrideExactFrequency.onToggle = [this]() {        
        globalSettings->set<bool>("video_override_exact", videoFrameAdjust.overrideExactFrequency.checked()); 
        audioManager->setResampler();
        if (activeVideoManager)
            activeVideoManager->initFpsLimit();
        updateFrequencyLayout();
    };
    
    if ( globalSettings->get<bool>("video_override_exact", true) )
        videoFrameAdjust.overrideExactFrequency.setChecked();
    
    videoFrameAdjust.palFrequency.onChange = [this]() {
         globalSettings->set<std::string>("video_pal", videoFrameAdjust.palFrequency.text() );
         audioManager->setResampler();
         if (activeVideoManager)
            activeVideoManager->initFpsLimit();
    };
    
    videoFrameAdjust.ntscFrequency.onChange = [this]() {
        globalSettings->set<std::string>("video_ntsc", videoFrameAdjust.ntscFrequency.text() );
        audioManager->setResampler();
        if (activeVideoManager)
            activeVideoManager->initFpsLimit();
    };    
            
    videoFrameAdjust.palFrequency.setText( GUIKIT::String::formatFloatingPoint( globalSettings->get<double>("video_pal", 50.0, {25.0, 100.0}) ) );
    videoFrameAdjust.ntscFrequency.setText( GUIKIT::String::formatFloatingPoint( globalSettings->get<double>("video_ntsc", 60.0, {30.0, 120.0}) ) );
		
    videoGeometry.aspectCorrect.setChecked( globalSettings->get<bool>("aspect_correct", true) );
    videoGeometry.aspectCorrect.onToggle = [&]() {
		bool state = videoGeometry.aspectCorrect.checked();		
        globalSettings->set<bool>("aspect_correct", state);
		VideoManager::setAspectCorrect( state );
        view->updateViewport();
    };
	
	videoGeometry.integerScaling.setChecked( globalSettings->get<bool>("integer_scaling", false) );
    videoGeometry.integerScaling.onToggle = [&]() {
		bool state = videoGeometry.integerScaling.checked();		
        globalSettings->set<bool>("integer_scaling", state);
		VideoManager::setIntegerScaling( state );
        view->updateViewport();
    };
	
	crtEmulation.threadMode.setChecked( globalSettings->get<bool>("crt_threaded", true) );
	crtEmulation.threadMode.onToggle = [this]() {
		bool state = crtEmulation.threadMode.checked();		
        globalSettings->set<bool>("crt_threaded", state);
        VideoManager::setThreaded( state );
    };
    
	crtEmulation.shaderInputPrecision.setChecked( globalSettings->get<bool>("crt_shader_input_precision", false) );
    crtEmulation.shaderInputPrecision.onToggle = [this]() {
		bool state = crtEmulation.shaderInputPrecision.checked();		
        globalSettings->set<bool>("crt_shader_input_precision", state);
        VideoManager::setShaderInputPrecision( state );
    };
    
    updateFrequencyLayout();
}

auto VideoLayout::updateFrequencyLayout() -> void {
    auto state = videoFrameAdjust.overrideExactFrequency.checked();
    
    videoFrameAdjust.pal.setEnabled( state );
    videoFrameAdjust.palFrequency.setEnabled( state );
    videoFrameAdjust.ntsc.setEnabled( state );
    videoFrameAdjust.ntscFrequency.setEnabled( state );    
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

    videoFrameAdjust.setText( trans->get("frequency_correction") );
    videoFrameAdjust.pal.setText("PAL:");
    videoFrameAdjust.ntsc.setText("NTSC:");
    videoFrameAdjust.overrideExactFrequency.setText( trans->get("override_exact_frequency") );
    videoFrameAdjust.overrideExactFrequency.setTooltip( trans->get("override_exact_frequency_tooltip") );
}

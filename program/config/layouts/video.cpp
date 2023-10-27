
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

VideoGeometryLayout::Dimension::Dimension() {
    append(label, {0u, 0u}, 5);
    append(width, {50, 0u}, 10);
    append(height, {50, 0u});

    setAlignment(0.5);
}

VideoGeometryLayout::Control::Control() {
    append(refresh, {0u, 0u}, 10);
    append(apply, {0u, 0u});

    setAlignment(0.5);
}

VideoGeometryLayout::VideoGeometryLayout() {
    append( aspectCorrectResizing, {0u, 0u}, 5 );
    append( dimension, {0u, 0u}, 5 );
    append( control, {0u, 0u} );
	
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
    append(threadedRenderer, {0u, 0u}, 10);
    append(trOn, {0u, 0u}, 10);
    append(trAuto, {0u, 0u});
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
        for (auto emulator : emulators) {
            program->getSettings(emulator)->set<std::string>( "shader", "");
            auto vManager = VideoManager::getInstance(emulator);
            vManager->shader.loadExternal();
		}

		view->updateShader(false);
        program->initVideo(true);
        updateDriverPropsVisibility();
        emuThread->unlock();
	};

    append(paths, {~0u, 0u}, 10);
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
        videoSettingsLayout.hardSync.setChecked(globalSettings->get("hardsync", true));
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
        emuThread->lock();
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
        emuThread->unlock();
	};
	
	paths.shader.empty.onActivate = [&]() {
        emuThread->lock();
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

        emuThread->unlock();
    };

	paths.shader.edit.setText( globalSettings->get<std::string>("shader_folder", "") );
    
	hLayout.append(videoGeometry, {~0u, 0u}, 20);
    hLayout.append(screenTextLayout, {~0u, 0u}, 20);
    hLayout.append(crtEmulation, {~0u, 0u});
    append(hLayout, {~0u, 0u}, 10);
    append(videoFps, {~0u, 0u});

    videoFps.updateDelay.slider.onChange = [this](unsigned position) {
        emuThread->lock();

        position = (position + 1) * 200;

        globalSettings->set<unsigned>("fps_update", position);

        videoFps.updateDelay.value.setText( std::to_string(position) + " " + videoFps.updateDelay.unit );
        emuThread->unlock();
    };

    unsigned fpsUpdate = globalSettings->get<unsigned>("fps_update", 1000u, {200u, 5000u});
    videoFps.updateDelay.slider.setPosition( fpsUpdate / 200 - 1 );
    videoFps.updateDelay.value.setText( std::to_string( fpsUpdate ) + " " + videoFps.updateDelay.unit );

    videoFps.options.Zero.setText("0");
    videoFps.options.Zero.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("fps_decimal_point", 0);
        statusHandler->statusBar->updateDimension( 0, "1000" );
        emuThread->unlock();
    };
    videoFps.options.One.setText("1");
    videoFps.options.One.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("fps_decimal_point", 1);
        statusHandler->statusBar->updateDimension( 0, "1000.9" );
        emuThread->unlock();
    };
    videoFps.options.Two.setText("2");
    videoFps.options.Two.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("fps_decimal_point", 2);
        statusHandler->statusBar->updateDimension( 0, "1000.99" );
        emuThread->unlock();
    };
    videoFps.options.Three.setText("3");
    videoFps.options.Three.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("fps_decimal_point", 3);
        statusHandler->statusBar->updateDimension( 0, "1000.999" );
        emuThread->unlock();
    };

    unsigned countDecimal = globalSettings->get<unsigned>("fps_decimal_point", 3, {0u, 3u});
    switch (countDecimal) {
        case 0: videoFps.options.Zero.setChecked(); break;
        case 1: videoFps.options.One.setChecked(); break;
        case 2: videoFps.options.Two.setChecked(); break;
        case 3: videoFps.options.Three.setChecked(); break;
    }

    screenTextLayout.option1.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set("video_screen_text", 0);
        statusHandler->setMessage("");
        emuThread->unlock();
    };
    
    screenTextLayout.option2.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set("video_screen_text", 1);
        statusHandler->setMessage("");
        emuThread->unlock();
    };
    
    screenTextLayout.option3.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set("video_screen_text", 2);
        statusHandler->setMessage("");
        emuThread->unlock();
    };
    
    if(globalSettings->get("video_screen_text", 0) == 0) screenTextLayout.option1.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 1) screenTextLayout.option2.setChecked();
    if(globalSettings->get("video_screen_text", 0) == 2) screenTextLayout.option3.setChecked();

    videoGeometry.aspectCorrectResizing.setChecked( globalSettings->get<bool>("aspect_correct_resizing", false) );
    videoGeometry.aspectCorrectResizing.onToggle = [&](bool checked) {
        emuThread->lock();
        globalSettings->set<bool>("aspect_correct_resizing", checked);
        if (checked)
            view->setAspectRatio( {4, 3} );
        else
            view->setAspectRatio( {0, 0} );
        view->updateViewport();
        emuThread->unlock();
    };

    videoGeometry.dimension.width.onChange = [this]() {
        int val = videoGeometry.dimension.width.value();
        if (val < 100) val = 100;
        globalSettings->set<unsigned>("view_hold_width", val);
    };
    videoGeometry.dimension.width.setValue( globalSettings->get<unsigned>("view_hold_width", 800) );

    videoGeometry.dimension.height.onChange = [this]() {
        int val = videoGeometry.dimension.height.value();
        if (val < 100) val = 100;
        globalSettings->set<unsigned>("view_hold_height", val);
    };
    videoGeometry.dimension.height.setValue( globalSettings->get<unsigned>("view_hold_height", 600) );

    videoGeometry.control.refresh.onActivate = [this]() {
        if (view->fullScreen())
            return;

        emuThread->lock();
        auto w = globalSettings->get<unsigned>("screen_width", 800);
        auto h = globalSettings->get<unsigned>("screen_height", 600);

        globalSettings->set<unsigned>("view_hold_width", w);
        globalSettings->set<unsigned>("view_hold_height", h);

        videoGeometry.dimension.width.setValue(w);
        videoGeometry.dimension.height.setValue(h);
        emuThread->unlock();
    };

    videoGeometry.control.apply.onActivate = [this]() {
        if (view->fullScreen())
            return;

        emuThread->lock();
        globalSettings->set<unsigned>("screen_width", globalSettings->get<unsigned>("view_hold_width", 800));
        globalSettings->set<unsigned>("screen_height", globalSettings->get<unsigned>("view_hold_height", 600));

        view->updateGeometry(true);
        emuThread->unlock();
    };

	crtEmulation.threadMode.setChecked( globalSettings->get<bool>("crt_threaded", true) );
	crtEmulation.threadMode.onToggle = [this](bool checked) {
        emuThread->lock();
        globalSettings->set<bool>("crt_threaded", checked);
        VideoManager::setCrtThreaded( checked );
        emuThread->unlock();
    };
    
	crtEmulation.shaderInputPrecision.setChecked( globalSettings->get<bool>("crt_shader_input_precision", false) );
    crtEmulation.shaderInputPrecision.onToggle = [this](bool checked) {
        emuThread->lock();
        globalSettings->set<bool>("crt_shader_input_precision", checked);
        VideoManager::setShaderInputPrecision( checked );
        emuThread->unlock();
    };
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
    
    screenTextLayout.option1.setText( trans->get("disabled") );
    screenTextLayout.option2.setText( trans->get("intelligent") );
    screenTextLayout.option2.setTooltip( trans->get("tip_intelligent_screentext") );
    screenTextLayout.option3.setText( trans->get("enabled") );
    screenTextLayout.setText( trans->get("screen_status") );
    
    crtEmulation.threadMode.setText( trans->get("crt_threaded") );
    crtEmulation.shaderInputPrecision.setText( trans->get("color_channel_32bit") );
    crtEmulation.setText( trans->get("crt_emulation") );
	
	videoGeometry.setText(trans->get("geometry"));
    videoGeometry.aspectCorrectResizing.setText(trans->get("resize aspect corrected"));
    videoGeometry.dimension.label.setText(trans->getA("resolution", true));
    videoGeometry.control.refresh.setText(trans->getA("refresh"));
    videoGeometry.control.apply.setText(trans->getA("apply"));
    
    driverLayout.name.setText( trans->get("driver", {}, true) );
    
    paths.setText( trans->get("paths") );
    paths.shader.label.setText(trans->get("Shader",{}, true));
	paths.shader.select.setText(trans->get("select"));
	paths.shader.empty.setText(trans->get("remove"));

    videoFps.setText( trans->get("FPS") );
    videoFps.updateDelay.name.setText( trans->get("Refresh", {}, true) );
    videoFps.options.labelDecimalPlace.setText( trans->get("Decimal Place", {}, true) );
}

auto VideoLayout::updateDriverPropsVisibility() -> void {
    auto threadedEmu = globalSettings->get<bool>("threaded_emu", false);

    videoSettingsLayout.exclusiveFullscreen.setEnabled( !threadedEmu && videoDriver->canExclusiveFullscreen() );
    videoSettingsLayout.hardSync.setEnabled( videoDriver->canHardSync() );
}

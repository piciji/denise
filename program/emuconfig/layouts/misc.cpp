
SpeedLayout::SpeedLayout() {
    append(label, {0u, 0u}, 5);
    append(fps, {0u, 0u}, 5 );
    append(percent, {0u, 0u}, 10 );
    GUIKIT::LineEdit test;
    test.setText("999.99");
    append(speed, {test.minimumSize().width, 0u}, 10 );
    append(apply, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( fps, percent );

    setAlignment(0.5);
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

InputSamplingLayout::Options::Options() {
    append(staticMode, {0u, 0u}, 10);
    append(restrictedDynamicMode, {0u, 0u}, 10);
    append(dynamicMode, {0u, 0u});

    setAlignment(0.5);
    GUIKIT::RadioBox::setGroup( staticMode, restrictedDynamicMode, dynamicMode );
}

InputSamplingLayout::InputSamplingLayout() : control("ms") {
    setPadding(10);

    append(options, {0u, 0u}, 5 );
    append(control, {~0u, 0u} );

    control.slider.setLength(10);

    control.updateValueWidth( "10 " + control.unit );

    setFont(GUIKIT::Font::system("bold"));
}

RunAheadLayout::RunAheadLayout() : control("") {
    
    setPadding(10);
    
    append(control, {~0u, 0u}, 10 );
    append(options, {0u, 0u} );
    
    control.slider.setLength(11);
    
    control.updateValueWidth( "10" );
    
    setFont(GUIKIT::Font::system("bold"));   
}

AutostartLayout::AutoWarp::AutoWarp(Emulator::Interface* emulator) {
    append(label, {0u, 0u}, 10 );
    append(off, {0u, 0u}, 10 );
    append(normal, {0u, 0u}, 10 );
    append(aggressive, {0u, 0u}, 25 );
    append(diskFirstFile, {0u, 0u}, 10);

    if (emulator->getTapeMediaGroup())
        append(tapeFirstFile, {0u, 0u}, 10);

    append(disableWarpWhenInput, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( off, normal, aggressive );

    setAlignment( 0.5 );
}

AutostartLayout::StartWrapper::StartWrapper() {
    append(start, {0u, 0u}, 10);
    append(option, {0u, 0u});
}

AutostartLayout::StartWrapper::Start::Start() {
    append(diskTrapsOnDblClick, {0u, 0u}, 5 );
    append(tapeTrapsOnDblClick, {0u, 0u} );
}

AutostartLayout::StartWrapper::Option::Option() {
    append(diskOptions, {0u, 0u}, 5 );
    append(tapeWithStandardKernal, {0u, 0u} );
}

AutostartLayout::StartWrapper::Option::DiskOptions::DiskOptions() {
    append(loadWithColumn, {0u, 0u}, 5 );
    append(speederTraps, {0u, 0u}, 10 );
    setAlignment( 0.5 );
}

AutostartLayout::AutostartLayout(Emulator::Interface* emulator) : autoWarp(emulator) {
    setPadding(10);

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        append(autoWarp, {0u, 0u}, 5 );
        startWrapper = new StartWrapper;
        append(*startWrapper, {0u, 0u} );
    } else
        append(autoWarp, {0u, 0u} );

    setFont(GUIKIT::Font::system("bold"));
}

RunAheadLayout::Options::Options() {
    
    append(performanceMode, {0u, 0u}, 10 );
    append(disableOnPower, {0u, 0u}, 10 );
    append(preventDynamic, {0u, 0u} );
    
    setAlignment( 0.5 );
}

MiscLayout::MiscLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);

    append( speedLayout, {~0u, 0u}, 10 );
    append( inputSamplingLayout, {~0u, 0u}, 10 );
    append( runAheadLayout, {~0u, 0u}, 10 );

    autostartLayout = new AutostartLayout(this->emulator);
    append(*autostartLayout, {~0u, 0u});

    autostartLayout->autoWarp.off.onActivate = [this]() {

        _settings->set<unsigned>("auto_warp", 0);

        initAutowarp();
    };

    autostartLayout->autoWarp.normal.onActivate = [this]() {

        _settings->set<unsigned>("auto_warp", 1);

        initAutowarp();
    };

    autostartLayout->autoWarp.aggressive.onActivate = [this]() {

        _settings->set<unsigned>("auto_warp", 2);

        initAutowarp();
    };

    autostartLayout->autoWarp.diskFirstFile.onToggle = [this](bool checked) {

        _settings->set<bool>("auto_warp_disk_first_file", checked);

        autostartLayout->autoWarp.disableWarpWhenInput.setEnabled( !checked );

        initAutowarp( emulator->getDiskMediaGroup() );
    };

    autostartLayout->autoWarp.disableWarpWhenInput.onToggle = [this](bool checked) {

        _settings->set<bool>("auto_warp_off_input", checked);

        initAutowarp( emulator->getDiskMediaGroup() );
    };

    autostartLayout->autoWarp.tapeFirstFile.onToggle = [this](bool checked) {

        _settings->set<bool>("auto_warp_tape_first_file", checked);

        initAutowarp( emulator->getTapeMediaGroup() );
    };

    if (autostartLayout->startWrapper) {
        autostartLayout->startWrapper->option.tapeWithStandardKernal.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_tape_standard_kernal", checked);
        };

        autostartLayout->startWrapper->option.diskOptions.loadWithColumn.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_load_with_column", checked);
        };

        autostartLayout->startWrapper->option.diskOptions.speederTraps.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_speeder_traps", checked);
        };

        autostartLayout->startWrapper->start.diskTrapsOnDblClick.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_traps_on_dblclick", checked);
        };

        autostartLayout->startWrapper->start.tapeTrapsOnDblClick.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_tape_traps_on_dblclick", checked);
        };
    }

    runAheadLayout.control.slider.onChange = [this](unsigned position) {

        runAheadLayout.control.value.setText( std::to_string(position) );
        
        _settings->set<unsigned>( "runahead", position);

        emuThread->lock();
        audioManager->drive.reset();
        this->emulator->runAhead( position );
        emuThread->unlock();
    };
    
    runAheadLayout.options.performanceMode.onToggle = [this](bool checked) {
        _settings->set<bool>( "runahead_performance", checked);

        emuThread->lock();
        this->emulator->runAheadPerformance( checked );
        emuThread->unlock();
    };
    
    runAheadLayout.options.disableOnPower.onToggle = [this](bool checked) {
        
        _settings->set<bool>( "runahead_disable", checked );
    };

    runAheadLayout.options.preventDynamic.onToggle = [this](bool checked) {
        _settings->set<bool>( "runahead_prevent_jit", checked );

        emuThread->lock();
        this->emulator->runAheadPreventJit( checked );
        emuThread->unlock();
    };

    inputSamplingLayout.control.slider.onChange = [this](unsigned position) {
        position += 1;

        inputSamplingLayout.control.value.setText( std::to_string(position) + " " + inputSamplingLayout.control.unit );

        _settings->set<unsigned>( "input_jit_delay", position);

        auto manager = InputManager::getManager(this->emulator);

        manager->jit.rescanDelay = position;
    };

    inputSamplingLayout.options.staticMode.onActivate = [this]() {
        _settings->set<unsigned>("input_sampling", 0);

        emuThread->lock();
        this->emulator->setInputSampling( 0 );
        InputManager::resetJit();
        emuThread->unlock();

        inputSamplingLayout.control.setEnabled(false);
    };

    inputSamplingLayout.options.restrictedDynamicMode.onActivate = [this]() {
        _settings->set<unsigned>("input_sampling", 1);

        emuThread->lock();
        this->emulator->setInputSampling( 1 );
        InputManager::resetJit();
        emuThread->unlock();

        inputSamplingLayout.control.setEnabled(false);
    };

    inputSamplingLayout.options.dynamicMode.onActivate = [this]() {
        _settings->set<unsigned>("input_sampling", 2);

        emuThread->lock();
        this->emulator->setInputSampling( 2 );
        InputManager::resetJit();
        emuThread->unlock();

        inputSamplingLayout.control.setEnabled(true);
    };

    speedLayout.speed.onChange = [this]() {
        std::string userInput = speedLayout.speed.text();

        GUIKIT::String::replace(userInput, ",", ".");

        if (userInput.empty() || !GUIKIT::String::isFloatNumber( userInput ) ) {
            return;
        }

        std::stringstream ss( userInput );
        float out = 0.0;
        ss >> out;

        if (out < 1.0)
            return;

        _settings->set<std::string>("custom_speed", userInput);

        if (view->isCustomSpeed()) {
            emuThread->lock();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
        }
        view->updateSpeedLabels(true);
    };

    speedLayout.percent.onActivate = [this]() {
        _settings->set<bool>("custom_speed_percent", true);

        if (view->isCustomSpeed()) {
            emuThread->lock();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
        }
        view->updateSpeedLabels(true);
    };

    speedLayout.fps.onActivate = [this]() {
        _settings->set<bool>("custom_speed_percent", false);

        if (view->isCustomSpeed()) {
            emuThread->lock();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
        }
        view->updateSpeedLabels(true);
    };

    speedLayout.apply.onActivate = [this]() {
        view->activateCustomSpeed();
    };
                                       
    loadSettings();
}

auto MiscLayout::setRunAheadPerformance(bool state) -> void {
    
    runAheadLayout.options.performanceMode.setChecked(state);              
}

auto MiscLayout::setRunAhead(unsigned pos, bool force) -> void {

    if (!force) {
        auto _pos = runAheadLayout.control.slider.position();

        if (pos == _pos)
            return;
    }
    runAheadLayout.control.slider.setPosition(pos);

    runAheadLayout.control.value.setText(std::to_string(pos));    
}

auto MiscLayout::translate() -> void {
    
    runAheadLayout.setText( trans->get("runAhead") );
    
    runAheadLayout.options.performanceMode.setText( trans->get("performance mode") );
    
    runAheadLayout.options.performanceMode.setTooltip( trans->get("runAhead performance info") );
    
    runAheadLayout.control.name.setText( trans->get("frames") );
    
    runAheadLayout.options.disableOnPower.setText( trans->get("disable runAhead on power") );

    runAheadLayout.options.preventDynamic.setText( trans->get("prevent dynamic sampling") );

    inputSamplingLayout.setText( trans->get("Input sampling") );
    inputSamplingLayout.options.staticMode.setText( trans->get("static sampling") );
    inputSamplingLayout.options.staticMode.setTooltip( trans->get("static sampling tooltip") );
    inputSamplingLayout.options.restrictedDynamicMode.setText( trans->get("restricted dynamic sampling") );
    inputSamplingLayout.options.restrictedDynamicMode.setTooltip( trans->get("restricted dynamic sampling tooltip") );
    inputSamplingLayout.options.dynamicMode.setText( trans->get("dynamic sampling") );
    inputSamplingLayout.options.dynamicMode.setTooltip( trans->get("dynamic sampling tooltip") );

    inputSamplingLayout.control.name.setText( trans->get("minimum rescan time", {}, true) );

    if (autostartLayout) {
        autostartLayout->setText(trans->get("Autostart"));
        autostartLayout->autoWarp.label.setText(trans->get("Auto Warp", {}, true));
        autostartLayout->autoWarp.aggressive.setText(trans->get("aggressive"));
        autostartLayout->autoWarp.normal.setText(trans->get("normal"));
        autostartLayout->autoWarp.off.setText(trans->get("off"));

        autostartLayout->autoWarp.diskFirstFile.setText(trans->get("disk warp first file"));
        autostartLayout->autoWarp.diskFirstFile.setTooltip(trans->get("warp first file tooltip"));
        autostartLayout->autoWarp.tapeFirstFile.setText(trans->get("tape warp first file"));
        autostartLayout->autoWarp.tapeFirstFile.setTooltip(trans->get("warp first file tooltip"));
        autostartLayout->autoWarp.disableWarpWhenInput.setText(trans->get("disable warp when input"));
        autostartLayout->autoWarp.disableWarpWhenInput.setTooltip( trans->get("disable warp when input tooltip") );

        if (autostartLayout->startWrapper) {
            autostartLayout->startWrapper->option.tapeWithStandardKernal.setText(trans->get("tape default kernal"));
            autostartLayout->startWrapper->option.diskOptions.loadWithColumn.setText( "Load \":*\"" );
            autostartLayout->startWrapper->option.diskOptions.speederTraps.setText( trans->get("Speeder Traps") );
            autostartLayout->startWrapper->option.diskOptions.speederTraps.setTooltip( trans->get("Speeder Traps tooltip") );
            autostartLayout->startWrapper->start.diskTrapsOnDblClick.setText(trans->get("VDT Disk Autostart on dblclick"));
            autostartLayout->startWrapper->start.tapeTrapsOnDblClick.setText(trans->get("VDT Tape Autostart on dblclick"));
        }
    }

    speedLayout.setText( trans->get("Speed") );
    speedLayout.label.setText( trans->get("Set speed", {}, true) );
    speedLayout.fps.setText( trans->get("FPS") );
    speedLayout.percent.setText( trans->get("Percent") );
    speedLayout.apply.setText( trans->get("enable") );
}

auto MiscLayout::initAutowarp(Emulator::Interface::MediaGroup* forGroup) -> void {
    if (!activeEmulator)
        return;

    emuThread->lock();
    if (activeEmulator == emulator) {
        auto autoStartedMediaGroup = emulator->autoStartedByMediaGroup();
        if (autoStartedMediaGroup && autoStartedMediaGroup->isDrive()) {
            if (!forGroup || (forGroup == autoStartedMediaGroup) )
                program->initAutoWarp(autoStartedMediaGroup, true);
        }
    }
    emuThread->unlock();
}

auto MiscLayout::loadSettings() -> void {

    if (autostartLayout) {
        unsigned autoWarp = _settings->get<unsigned>("auto_warp", 0);

        if (autoWarp == 0)
            autostartLayout->autoWarp.off.setChecked();
        else if (autoWarp == 1)
            autostartLayout->autoWarp.normal.setChecked();
        else if (autoWarp == 2)
            autostartLayout->autoWarp.aggressive.setChecked();

        autostartLayout->autoWarp.diskFirstFile.setChecked(_settings->get<bool>("auto_warp_disk_first_file", true));

        autostartLayout->autoWarp.disableWarpWhenInput.setChecked(_settings->get<bool>("auto_warp_off_input", false));

        autostartLayout->autoWarp.disableWarpWhenInput.setEnabled( !autostartLayout->autoWarp.diskFirstFile.checked() );

        autostartLayout->autoWarp.tapeFirstFile.setChecked(_settings->get<bool>("auto_warp_tape_first_file", false));

        if (autostartLayout->startWrapper) {
            autostartLayout->startWrapper->option.tapeWithStandardKernal.setChecked( _settings->get<bool>("autostart_tape_standard_kernal", false));
            autostartLayout->startWrapper->option.diskOptions.loadWithColumn.setChecked(_settings->get<bool>("autostart_load_with_column", false));
            autostartLayout->startWrapper->option.diskOptions.speederTraps.setChecked(_settings->get<bool>("autostart_speeder_traps", false));
            autostartLayout->startWrapper->start.diskTrapsOnDblClick.setChecked(_settings->get<bool>("autostart_traps_on_dblclick", false));
            autostartLayout->startWrapper->start.tapeTrapsOnDblClick.setChecked(_settings->get<bool>("autostart_tape_traps_on_dblclick", false));
        }
    }

    setRunAheadPerformance(_settings->get<bool>("runahead_performance", false));

    runAheadLayout.options.disableOnPower.setChecked(_settings->get<bool>("runahead_disable", true));

    unsigned pos = _settings->get<unsigned>( "runahead", 0, {0u, 10u});
    
    setRunAhead( pos );

    runAheadLayout.options.preventDynamic.setChecked(_settings->get<bool>("runahead_prevent_jit", true));

    unsigned inputSamplingMode = _settings->get<unsigned>("input_sampling", 2, {0, 2});
    if (inputSamplingMode == 0) inputSamplingLayout.options.staticMode.setChecked();
    else if (inputSamplingMode == 1) inputSamplingLayout.options.restrictedDynamicMode.setChecked();
    else if (inputSamplingMode == 2) inputSamplingLayout.options.dynamicMode.setChecked();

    unsigned jitDelay = _settings->get<unsigned>( "input_jit_delay", 5, {1, 10});

    inputSamplingLayout.control.slider.setPosition(jitDelay - 1);

    inputSamplingLayout.control.value.setText(std::to_string(jitDelay) + " " + inputSamplingLayout.control.unit);

    inputSamplingLayout.control.setEnabled( inputSamplingMode == 2 );

    speedLayout.speed.setText( _settings->get<std::string>("custom_speed", "59.95") );
    if (_settings->get<bool>("custom_speed_percent", false))
        speedLayout.percent.setChecked();
    else
        speedLayout.fps.setChecked();
}
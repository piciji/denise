
SpeedLayout::SpeedLayout() {
    append(label, {0u, 0u}, 5);
    append(speed, {GUIKIT::Font::scale(55), 0u}, 10 );
    append(fps, {0u, 0u}, 5 );
    append(percent, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( fps, percent );

    setAlignment(0.5);
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

JitLayout::JitLayout() : control("ms") {
    setPadding(10);

    append(active, {0u, 0u}, 10 );
    append(control, {~0u, 0u} );

    control.slider.setLength(8);

    control.updateValueWidth( "10 " + control.unit );

    setFont(GUIKIT::Font::system("bold"));

    setAlignment( 0.5 );
}

RunAheadLayout::RunAheadLayout() : control("") {
    
    setPadding(10);
    
    append(control, {~0u, 0u}, 10 );
    append(options, {0u, 0u} );
    
    control.slider.setLength(11);
    
    control.updateValueWidth( "10" );
    
    setFont(GUIKIT::Font::system("bold"));   
}

AutostartLayout::AutoWarp::AutoWarp() {
    append(label, {0u, 0u}, 10 );
    append(off, {0u, 0u}, 10 );
    append(normal, {0u, 0u}, 10 );
    append(aggressive, {0u, 0u}, 25 );
    append(diskFirstFile, {0u, 0u}, 10 );
    append(tapeFirstFile, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( off, normal, aggressive );

    setAlignment( 0.5 );
}

AutostartLayout::Options::Options() {
    append(tapeWithStandardKernal, {0u, 0u}, 10 );
    append(loadWithColumn, {0u, 0u}, 10 );
    append(trapsOnDblClick, {0u, 0u} );

    setAlignment( 0.5 );
}

AutostartLayout::AutostartLayout() {
    setPadding(10);

    append(autoWarp, {0u, 0u}, 5 );
    append(options, {0u, 0u} );

    setFont(GUIKIT::Font::system("bold"));
}

RunAheadLayout::Options::Options() {
    
    append(performanceMode, {0u, 0u}, 10 );
    append(disableOnPower, {0u, 0u}, 10 );
    append(preventJit, {0u, 0u} );
    
    setAlignment( 0.5 );
}

MiscLayout::MiscLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);

    append( speedLayout, {~0u, 0u}, 10 );
    append( jitLayout, {~0u, 0u}, 10 );
    append( runAheadLayout, {~0u, 0u}, 10 );

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        autostartLayout = new AutostartLayout;
        append(*autostartLayout, {~0u, 0u});

        autostartLayout->autoWarp.off.onActivate = [this]() {

            _settings->set<unsigned>("auto_warp", 0);
        };

        autostartLayout->autoWarp.normal.onActivate = [this]() {

            _settings->set<unsigned>("auto_warp", 1);
        };

        autostartLayout->autoWarp.aggressive.onActivate = [this]() {

            _settings->set<unsigned>("auto_warp", 2);
        };

        autostartLayout->autoWarp.diskFirstFile.onToggle = [this](bool checked) {

            _settings->set<bool>("auto_warp_disk_first_file", checked);
        };

        autostartLayout->autoWarp.tapeFirstFile.onToggle = [this](bool checked) {

            _settings->set<bool>("auto_warp_tape_first_file", checked);
        };

        autostartLayout->options.tapeWithStandardKernal.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_tape_standard_kernal", checked);
        };

        autostartLayout->options.loadWithColumn.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_load_with_column", checked);
        };

        autostartLayout->options.trapsOnDblClick.onToggle = [this](bool checked) {
            _settings->set<bool>("autostart_traps_on_dblclick", checked);
        };
    }

    runAheadLayout.control.slider.onChange = [this]() {
        
        unsigned pos = runAheadLayout.control.slider.position();
        
        runAheadLayout.control.value.setText( std::to_string(pos) );
        
        _settings->set<unsigned>( "runahead", pos);

        audioManager->drive.reset();

        this->emulator->runAhead( pos );
    };
    
    runAheadLayout.options.performanceMode.onToggle = [this](bool checked) {
        _settings->set<bool>( "runahead_performance", checked);
        
        this->emulator->runAheadPerformance( checked );
    };
    
    runAheadLayout.options.disableOnPower.onToggle = [this](bool checked) {
        
        _settings->set<bool>( "runahead_disable", checked );
    };

    runAheadLayout.options.preventJit.onToggle = [this](bool checked) {
        _settings->set<bool>( "runahead_prevent_jit", checked );

        this->emulator->runAheadPreventJit( checked );
    };

    jitLayout.control.slider.onChange = [this]() {
        unsigned pos = jitLayout.control.slider.position();
        pos += 1;

        jitLayout.control.value.setText( std::to_string(pos) + " " + jitLayout.control.unit );

        _settings->set<unsigned>( "input_jit_delay", pos);

        auto manager = InputManager::getManager(this->emulator);

        manager->jit.rescanDelay = pos;
    };

    jitLayout.active.onToggle = [this](bool checked) {
        _settings->set<bool>("input_jit", checked);

        this->emulator->enableJit( checked );
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
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
        }
        view->updateSpeedLabels(true);
    };

    speedLayout.percent.onActivate = [this]() {
        _settings->set<bool>("custom_speed_percent", true);

        if (view->isCustomSpeed()) {
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
        }
        view->updateSpeedLabels(true);
    };

    speedLayout.fps.onActivate = [this]() {
        _settings->set<bool>("custom_speed_percent", false);

        if (view->isCustomSpeed()) {
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
        }
        view->updateSpeedLabels(true);
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

    runAheadLayout.options.preventJit.setText( trans->get("prevent JIT temporary") );

    jitLayout.setText( trans->get("JIT") );
    jitLayout.active.setText( trans->get("enable") );
    jitLayout.active.setTooltip( trans->get("JIT tooltip") );
    jitLayout.control.name.setText( trans->get("minimum rescan time", {}, true) );

    if (autostartLayout) {
        autostartLayout->setText(trans->get("Autostart"));
        autostartLayout->autoWarp.label.setText(trans->get("Auto Warp", {}, true));
        autostartLayout->autoWarp.aggressive.setText(trans->get("aggressive"));
        autostartLayout->autoWarp.normal.setText(trans->get("normal"));
        autostartLayout->autoWarp.off.setText(trans->get("off"));

        autostartLayout->autoWarp.diskFirstFile.setText(trans->get("disk warp first file"));
        autostartLayout->autoWarp.tapeFirstFile.setText(trans->get("tape warp first file"));

        autostartLayout->options.tapeWithStandardKernal.setText(trans->get("tape default kernal"));

        autostartLayout->options.loadWithColumn.setText( "Load \":*\"" );
        autostartLayout->options.trapsOnDblClick.setText(trans->get("VDT Autostart on dblclick"));
    }

    speedLayout.setText( trans->get("Speed") );
    speedLayout.label.setText( trans->get("Set speed", {}, true) );
    speedLayout.fps.setText( trans->get("FPS") );
    speedLayout.percent.setText( trans->get("Percent") );
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

        autostartLayout->autoWarp.tapeFirstFile.setChecked(_settings->get<bool>("auto_warp_tape_first_file", false));

        autostartLayout->options.tapeWithStandardKernal.setChecked( _settings->get<bool>("autostart_tape_standard_kernal", false));

        autostartLayout->options.loadWithColumn.setChecked(_settings->get<bool>("autostart_load_with_column", false));

        autostartLayout->options.trapsOnDblClick.setChecked(_settings->get<bool>("autostart_traps_on_dblclick", false));
    }

    setRunAheadPerformance(_settings->get<bool>("runahead_performance", false));

    runAheadLayout.options.disableOnPower.setChecked(_settings->get<bool>("runahead_disable", true));

    unsigned pos = _settings->get<unsigned>( "runahead", 0, {0u, 10u});
    
    setRunAhead( pos );

    runAheadLayout.options.preventJit.setChecked(_settings->get<bool>("runahead_prevent_jit", true));

    jitLayout.active.setChecked( _settings->get<bool>("input_jit", true) );

    unsigned jitDelay = _settings->get<unsigned>( "input_jit_delay", 3, {1, 8});

    jitLayout.control.slider.setPosition(jitDelay - 1);

    jitLayout.control.value.setText(std::to_string(jitDelay) + " " + jitLayout.control.unit);

    speedLayout.speed.setText( _settings->get<std::string>("custom_speed", "123.456") );
    if (_settings->get<bool>("custom_speed_percent", false))
        speedLayout.percent.setChecked();
    else
        speedLayout.fps.setChecked();
}
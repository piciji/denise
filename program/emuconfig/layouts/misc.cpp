
RunAheadLayout::RunAheadLayout() : control("") {
    
    setPadding(10);
    
    append(control, {~0u, 0u}, 10 );
    append(options, {0u, 0u} );
    
    control.slider.setLength(11);
    
    control.updateValueWidth( "10" );
    
    setFont(GUIKIT::Font::system("bold"));   
}

WarpLayout::WarpLayout() {

    setPadding(10);

    append(off, {0u, 0u}, 10 );
    append(normal, {0u, 0u}, 10 );
    append(aggressive, {0u, 0u}, 10 );
 //   append(oneTime, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( off, normal, aggressive );

    setAlignment( 0.5 );

    setFont(GUIKIT::Font::system("bold"));
}

RunAheadLayout::Options::Options() {
    
    append(performanceMode, {0u, 0u}, 20 );
    append(disableOnPower, {0u, 0u} );
    
    setAlignment( 0.5 );
}

MiscLayout::MiscLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);
    
    append( runAheadLayout, {~0u, 0u}, 10 );

    append( warpLayout, {~0u, 0u} );

    warpLayout.off.onActivate = [this]() {

        _settings->set<unsigned>( "auto_warp", 0);
    };

    warpLayout.normal.onActivate = [this]() {

        _settings->set<unsigned>( "auto_warp", 1);
    };

    warpLayout.aggressive.onActivate = [this]() {

        _settings->set<unsigned>( "auto_warp", 2);
    };

    warpLayout.oneTime.onToggle = [this]() {

        _settings->set<bool>( "auto_warp_onetime", warpLayout.oneTime.checked());
    };

    runAheadLayout.control.slider.onChange = [this]() {
        
        unsigned pos = runAheadLayout.control.slider.position();
        
        runAheadLayout.control.value.setText( std::to_string(pos) );
        
        _settings->set<unsigned>( "runahead", pos);
    
        this->emulator->runAhead( pos );
    };
    
    runAheadLayout.options.performanceMode.onToggle = [this]() {
        
        bool state = runAheadLayout.options.performanceMode.checked();
        
        _settings->set<bool>( "runahead_performance", state);
        
        this->emulator->runAheadPerformance( state );
    };
    
    runAheadLayout.options.disableOnPower.onToggle = [this]() {
        
        _settings->set<bool>( "runahead_disable", runAheadLayout.options.disableOnPower.checked() );
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

    warpLayout.setText( trans->get("warp on autostart") );
    warpLayout.aggressive.setText( trans->get("aggressive") );
    warpLayout.normal.setText( trans->get("normal") );
    warpLayout.off.setText( trans->get("off") );

    warpLayout.oneTime.setText( trans->get("onetime") );
}

auto MiscLayout::loadSettings() -> void {

    unsigned autoWarp = _settings->get<unsigned>( "auto_warp", 0);

    if (autoWarp == 0)
        warpLayout.off.setChecked();
    else if (autoWarp == 1)
        warpLayout.normal.setChecked();
    else if (autoWarp == 2)
        warpLayout.aggressive.setChecked();

    warpLayout.oneTime.setChecked( _settings->get<bool>( "auto_warp_onetime", true) );

    setRunAheadPerformance( _settings->get<bool>( "runahead_performance", false) );
    
    runAheadLayout.options.disableOnPower.setChecked( _settings->get<bool>( "runahead_disable", true) );
    
    unsigned pos = _settings->get<unsigned>( "runahead", 0, {0u, 10u});
    
    setRunAhead( pos );    
}
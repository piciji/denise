
RunAheadLayout::RunAheadLayout() : control("") {
    
    setPadding(10);
    
    append(control, {~0u, 0u}, 10 );
    append(options, {0u, 0u} );
    
    control.slider.setLength(11);
    
    control.updateValueWidth( "10" );
}

RunAheadLayout::Options::Options() {
    
    append(performanceMode, {0u, 0u}, 20 );
    append(disableOnPower, {0u, 0u} );
}

MiscLayout::MiscLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);
    
    append( runAheadLayout, {~0u, 0u} );
    
    runAheadLayout.control.slider.onChange = [this]() {
        
        unsigned pos = runAheadLayout.control.slider.position();
        
        runAheadLayout.control.value.setText( std::to_string(pos) );
        
        settings->set<unsigned>( this->tabWindow->ident("runahead"), pos);
    
        this->emulator->runAhead( pos );
    };
    
    runAheadLayout.options.performanceMode.onToggle = [this]() {
        
        bool state = runAheadLayout.options.performanceMode.checked();
        
        settings->set<bool>( this->tabWindow->ident("runahead_performance"), state);
        
        this->emulator->runAheadPerformance( state );
    };
    
    runAheadLayout.options.disableOnPower.onToggle = [this]() {
        
        settings->set<bool>( this->tabWindow->ident("runahead_disable"), runAheadLayout.options.disableOnPower.checked() );
    };
    
    setRunAheadPerformance( settings->get<bool>( this->tabWindow->ident("runahead_performance"), false) );
    
    runAheadLayout.options.disableOnPower.setChecked( settings->get<bool>( this->tabWindow->ident("runahead_disable"), true) );
    
    unsigned pos = settings->get<unsigned>( this->tabWindow->ident("runahead"), 0, {0u, 10u});
    
    setRunAhead( pos );
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
}

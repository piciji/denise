
AudioRecordLayout::Location::Location() {
    
    append(label, {0u, 0u}, 5 );
    append(path, {~0u, 0u}, 5  );
    append(select, {0u, 0u} );
    
    path.setEditable( false );
    
    setAlignment( 0.5 );
}

AudioRecordLayout::Duration::Duration() : minutesSlider(""), secondsSlider("") {
    
    append(useTimeLimit, {0u, 0u}, 10 );
    append(minutesSlider, {~0u, 0u}, 10);
    append(secondsSlider, {~0u, 0u}, 20);
    append(record, {0u, 0u} );
    
    minutesSlider.slider.setLength( 121 );
    secondsSlider.slider.setLength( 60 );
    
    minutesSlider.updateValueWidth("999");
    secondsSlider.updateValueWidth("99");
    
    setAlignment( 0.5 );
}

AudioRecordLayout::AudioRecordLayout() {
    
    setPadding(10);
    
    append(location, {~0u, 0u}, 10 );    
    append(duration, {~0u, 0u} );
    
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
    
    append( audioRecordLayout, {~0u, 0u} );
    
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
    
    audioRecordLayout.duration.useTimeLimit.onToggle = [this]() {
        
        bool state = audioRecordLayout.duration.useTimeLimit.checked();
        
        settings->set<bool>( this->tabWindow->ident("audio_record_timelimit"), state, false);
        
        audioRecordLayout.duration.setEnabled( state );
        
        audioRecordLayout.duration.useTimeLimit.setEnabled();            
        audioRecordLayout.duration.record.setEnabled();
        
        audioManager->record.setTimeLimit();
    };
    
    audioRecordLayout.duration.setEnabled( false );
    audioRecordLayout.duration.useTimeLimit.setEnabled();
    audioRecordLayout.duration.record.setEnabled();
    
    audioRecordLayout.location.select.onActivate = [this]() {
        
        auto path = GUIKIT::BrowserWindow()
            .setTitle( trans->get( "record path" ) )
            .setWindow( *this->tabWindow )
            .directory();
        
        if (path.empty())
            return;
        
        audioRecordLayout.location.path.setText( path );
        
        settings->set<std::string>( this->tabWindow->ident("audio_record_path"), path );
    };
    
    audioRecordLayout.location.path.setText( settings->get<std::string>( this->tabWindow->ident("audio_record_path"), "" ) );
    
    audioRecordLayout.duration.minutesSlider.slider.onChange = [this]() {
        
        unsigned pos = audioRecordLayout.duration.minutesSlider.slider.position();
        
        settings->set<unsigned>( this->tabWindow->ident("audio_record_minutes"), pos );
        
        audioRecordLayout.duration.minutesSlider.value.setText( std::to_string(pos) );
        
        audioManager->record.setTimeLimit();
    };
    
    unsigned value = settings->get<unsigned>( this->tabWindow->ident("audio_record_minutes"), 0, {0, 120} );
    
    audioRecordLayout.duration.minutesSlider.value.setText( std::to_string(value) );
    
    audioRecordLayout.duration.minutesSlider.slider.setPosition( value );                        

    audioRecordLayout.duration.secondsSlider.slider.onChange = [this]() {

        unsigned pos = audioRecordLayout.duration.secondsSlider.slider.position();
        
        settings->set<unsigned>( this->tabWindow->ident("audio_record_seconds"), pos );
        
        audioRecordLayout.duration.secondsSlider.value.setText( std::to_string(pos) );
        
        audioManager->record.setTimeLimit();
    };
    
    value = settings->get<unsigned>( this->tabWindow->ident("audio_record_seconds"), 0, {0, 59} );
    
    audioRecordLayout.duration.secondsSlider.value.setText( std::to_string(value) );
    
    audioRecordLayout.duration.secondsSlider.slider.setPosition( value );                        
    
    
    audioRecordLayout.duration.record.onToggle = [this]() {
                
        bool state = audioRecordLayout.duration.record.checked();                
        
        if (state) {                      
            std::string errorText;
            if (!audioManager->record.record(this->emulator, errorText)) {
                mes->error( errorText );
                audioRecordLayout.duration.record.setChecked(false);
                return;
            }                        
        } else
            audioManager->record.finish();
        
        status->record = state;
        
        audioRecordLayout.duration.record.setText( trans->get( state ? "Stop" : "Record" ) );        
    };
}

auto MiscLayout::toggleRecord() -> void {
    audioRecordLayout.duration.record.toggle();
}

auto MiscLayout::stopRecord() -> void {
    
    if (audioRecordLayout.duration.record.checked())
        audioRecordLayout.duration.record.toggle();
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
    
    audioRecordLayout.setText( trans->get("Audio Record") );
    audioRecordLayout.location.label.setText( trans->get("WAV Folder") );
    audioRecordLayout.location.select.setText( "..." );
    
    audioRecordLayout.duration.useTimeLimit.setText( trans->get("Recording time") );
    audioRecordLayout.duration.minutesSlider.name.setText( trans->get("Minutes", {}, true) );
    audioRecordLayout.duration.secondsSlider.name.setText( trans->get("Seconds", {}, true) );
    audioRecordLayout.duration.record.setText( trans->get("Record") );
}

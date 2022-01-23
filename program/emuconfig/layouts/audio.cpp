
AudioDriveLayout::Selection::Selection() {
    append( floppyLabel, {0u, 0u}, 10 );
    append( floppyCombo, {0u, 0u}, 10 );
    append( reload, {0u, 0u} );

    setAlignment( 0.5 );
}

AudioDriveLayout::AudioDriveLayout() : floppyVolume("%", true) {
    append( floppyVolume, {~0u, 0u}, 10 );
    append( selection, {0u, 0u} );


    setPadding(10);

    floppyVolume.slider.setLength( 301 );
    floppyVolume.updateValueWidth( "100 %" );
}

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

BassControlLayout::TopLayout::TopLayout() :
frequency( "Hz" ) {    
    append( active, {0u, 0u}, 10 );
    append( frequency, {~0u, 0u} );
    
    frequency.slider.setLength( 181 );    
    frequency.updateValueWidth( "200 Hz" );
    
    setAlignment( 0.5 );
}

BassControlLayout::BottomLayout::BottomLayout() :
gain( "" ),
reduceClipping( "" ) {    
    append( gain, {~0u, 0u}, 10 );
    append( reduceClipping, {~0u, 0u} );
    
    gain.slider.setLength( 41 );
    reduceClipping.slider.setLength( 11 );
    
    gain.updateValueWidth( "30" );
    reduceClipping.updateValueWidth( "0.9" );
    
    setAlignment( 0.5 );
}

BassControlLayout::BassControlLayout() {
    
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

ReverbControlLayout::TopLayout::TopLayout() :
dryTime( "" ),
wetTime( "" )
{    
    append( active, {0u, 0u}, 10 );
    append( dryTime, {~0u, 0u}, 10 );
    append( wetTime, {~0u, 0u} );
    
    dryTime.slider.setLength( 101 );    
    dryTime.updateValueWidth( "0.99" );
    wetTime.slider.setLength( 101 );    
    wetTime.updateValueWidth( "0.99" );
    
    setAlignment( 0.5 );
}

ReverbControlLayout::BottomLayout::BottomLayout() :
roomWidth( "" ),
roomSize( "" ),
damping( "" ) {    
    append( damping, {~0u, 0u}, 10 );
    append( roomWidth, {~0u, 0u}, 10 );
    append( roomSize, {~0u, 0u} );    
    
    roomWidth.slider.setLength( 101 );
    roomSize.slider.setLength( 101 );
    damping.slider.setLength( 101 );
    
    roomWidth.updateValueWidth( "0.99" );
    roomSize.updateValueWidth( "0.99" );
    damping.updateValueWidth( "0.99" );
    
    setAlignment( 0.5 );
}

ReverbControlLayout::ReverbControlLayout() {
    
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

PanningControlLayout::TopLayout::TopLayout() :
leftMix( "" ),
rightMix( "" ) {
    append( active, {0u, 0u}, 10 );
    append( leftChannel, {0u, 0u}, 10);
    append( leftMix, {~0u, 0u}, 10);
    append( rightMix, {~0u, 0u});
    
    leftMix.slider.setLength( 101 );
    rightMix.slider.setLength( 101 );
    
    leftMix.updateValueWidth( "0.99" );
    rightMix.updateValueWidth( "0.99" );
    
    leftChannel.setFont(GUIKIT::Font::system("bold"));
    
    setAlignment( 0.5 );
}

PanningControlLayout::BottomLayout::BottomLayout() :
leftMix( "" ),
rightMix( "" ) {
    append( rightChannel, {0u, 0u}, 10);
    append( leftMix, {~0u, 0u}, 10);
    append( rightMix, {~0u, 0u});
    
    leftMix.slider.setLength( 101 );
    rightMix.slider.setLength( 101 );
    
    leftMix.updateValueWidth( "0.99" );
    rightMix.updateValueWidth( "0.99" );
    
    rightChannel.setFont(GUIKIT::Font::system("bold"));
    
    setAlignment( 0.5 );
}

PanningControlLayout::PanningControlLayout() {
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

AudioLayout::AudioLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    
    moduleList.setHeaderText( { "" } );
    moduleList.setHeaderVisible( false );
    
    recordAudioImage.loadPng((uint8_t*)Icons::recordAudio, sizeof(Icons::recordAudio));
    sineImage.loadPng((uint8_t*)Icons::sine, sizeof(Icons::sine));
    processorImage.loadPng((uint8_t*)Icons::processor, sizeof(Icons::processor));
    driveImage.loadPng((uint8_t*)Icons::drive, sizeof(Icons::drive));
    
    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        moduleList.append( {"SID"} );
        moduleList.setImage(0, 0, processorImage);
    }

    moduleList.append( {"Drive"} );
    moduleList.setImage(moduleList.rowCount() - 1, 0, driveImage);
    moduleList.append( {"DSP"} );
    moduleList.setImage(moduleList.rowCount() - 1, 0, sineImage);    
    moduleList.append( {"Record"} );    
    moduleList.setImage(moduleList.rowCount() - 1, 0, recordAudioImage);
        
    moduleList.setSelection(0);
    moduleFrame.append( moduleList, { GUIKIT::Font::scale(130), GUIKIT::Font::scale(100)} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont( GUIKIT::Font::system("bold") );
    
    moduleList.onChange = [this]() {

        if (!moduleList.selected())
            return;

        moduleSwitch.setSelection( moduleList.selection() );
    };
    
    append( moduleFrame, {0u, 0u}, 10 );    

    if (dynamic_cast<LIBC64::Interface*> (emulator)) {

        settingsLayout.build(tabWindow, emulator,{Emulator::Interface::Model::Purpose::SoundChip, Emulator::Interface::Model::Purpose::AudioSettings, Emulator::Interface::Model::Purpose::AudioResampler},
        {4, 1, 1, 3, 4, 4, 4, 4, 4, 4, 4, 4});

        settingsLayout.setEvents();
    }
    
    dspFrame.append( bass, {~0u, 0u}, 5 );
    dspFrame.append( reverb, {~0u, 0u}, 5 );
    dspFrame.append( panning, {~0u, 0u} );
    
    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        moduleSwitch.setLayout( 0, settingsLayout, {~0u, ~0u} );
        moduleSwitch.setLayout( 1, driveLayout, {~0u, ~0u} );
        moduleSwitch.setLayout( 2, dspFrame, {~0u, ~0u} );
        moduleSwitch.setLayout( 3, audioRecord, {~0u, ~0u} );
    } else {
        moduleSwitch.setLayout( 0, driveLayout, {~0u, ~0u} );
        moduleSwitch.setLayout( 1, dspFrame, {~0u, ~0u} );
        moduleSwitch.setLayout( 2, audioRecord, {~0u, ~0u} );
    }
        
    append( moduleSwitch, {~0u, ~0u} );    
    
    bass.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_bass", checked );
        
        updateVisibility();
        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.top.frequency.slider.onChange = [this]() {
        
        unsigned val = bass.top.frequency.slider.position() + 20;
        
        _settings->set<unsigned>("audio_bass_freq", val );
        
        bass.top.frequency.value.setText( std::to_string(val) + " Hz" );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.bottom.gain.slider.onChange = [this]() {
        
        unsigned val = bass.bottom.gain.slider.position();
        
        _settings->set<unsigned>("audio_bass_gain", val );
        
        bass.bottom.gain.value.setText( std::to_string(val) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.bottom.reduceClipping.slider.onChange = [this]() {
        
        float val = (float)bass.bottom.reduceClipping.slider.position() / 10.0;
        
        _settings->set<float>("audio_bass_clipping", val);
        
        bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( val, 1) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };    
    
    // reverb
    reverb.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_reverb", checked );
        
        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };           
    
    setDspEvent( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    setDspEvent( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    setDspEvent( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    setDspEvent( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    setDspEvent( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );
    
    // panning
    panning.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_panning", checked );
        
        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    setDspEvent( &panning.top.leftMix, "audio_panning_left0", 1.0 );
    setDspEvent( &panning.top.rightMix, "audio_panning_left1", 0.0 );
    setDspEvent( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
    setDspEvent( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );
        
    audioRecord.duration.useTimeLimit.onToggle = [this](bool checked) {
        _settings->set<bool>( "audio_record_timelimit", checked);
        
        audioRecord.duration.setEnabled( checked );
        
        audioRecord.duration.useTimeLimit.setEnabled();            
        audioRecord.duration.record.setEnabled();

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.location.select.onActivate = [this]() {
        
        auto path = GUIKIT::BrowserWindow()
            .setTitle( trans->get( "record path" ) )
            .setWindow( *this->tabWindow )
            .directory();
        
        if (path.empty())
            return;
        
        audioRecord.location.path.setText( path );
        
        _settings->set<std::string>( "audio_record_path", path );
    };
    
    audioRecord.duration.minutesSlider.slider.onChange = [this]() {
        
        unsigned pos = audioRecord.duration.minutesSlider.slider.position();
        
        _settings->set<unsigned>( "audio_record_minutes", pos );
        
        audioRecord.duration.minutesSlider.value.setText( std::to_string(pos) );

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.duration.secondsSlider.slider.onChange = [this]() {

        unsigned pos = audioRecord.duration.secondsSlider.slider.position();
        
        _settings->set<unsigned>( "audio_record_seconds", pos );
        
        audioRecord.duration.secondsSlider.value.setText( std::to_string(pos) );

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.duration.record.onToggle = [this]() {

        bool state = audioRecord.duration.record.checked();
        emuThread->lock();
        if (state) {
            std::string errorText;
            if (!audioManager->record.start(this->emulator, errorText)) {
                mes->error( errorText );
                audioRecord.duration.record.setChecked(false);
                return;
            }                        
        } else
            audioManager->record.finish();

        emuThread->unlock();
        audioRecord.duration.record.setText( trans->get( state ? "Stop" : "Record" ) );
    };

    driveLayout.floppyVolume.active.onToggle = [this](bool checked) {

        _settings->set<bool>( "audio_floppy", checked );

        if (emulator == activeEmulator) {
            emuThread->lock();
            audioManager->setDriveSounds();
            emuThread->unlock();
        }
    };

    driveLayout.floppyVolume.slider.onChange = [this]() {
        auto value = driveLayout.floppyVolume.slider.position();

        _settings->set<unsigned>( "audio_floppy_volume", value );

        driveLayout.floppyVolume.value.setText( std::to_string( value ) + " %" );

        if (activeEmulator) {
            emuThread->lock();
            audioManager->drive.setVolume(activeEmulator, activeEmulator->getDiskMediaGroup(), (float) value * 0.01);
            emuThread->unlock();
        }
    };

    driveLayout.selection.floppyCombo.onChange = [this]() {

        std::string folder = driveLayout.selection.floppyCombo.text();

        _settings->set<std::string>( "audio_floppy_folder", folder );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup() );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    driveLayout.selection.reload.onActivate = [this]() {
        updateFloppyProfileList();

        driveLayout.selection.floppyCombo.setSelectionByRow( _settings->get<std::string>("audio_floppy_folder", "") );

        // in case if content doesn't match setting anymore
        _settings->set<std::string>("audio_floppy_folder", driveLayout.selection.floppyCombo.text() );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup() );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    updateFloppyProfileList();

    loadSettings();
}

auto AudioLayout::updateFloppyProfileList() -> void {

    driveLayout.selection.floppyCombo.reset();

    auto baseFolder = program->soundFolder();

    auto list = GUIKIT::File::getFolderList(baseFolder + "floppy/" + emulator->ident);

    for (auto& info : list) {
        driveLayout.selection.floppyCombo.append( info.name );
    }
}

auto AudioLayout::setDspEvent(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {

    sliderLayout->slider.onChange = [this, sliderLayout, ident]() {

        float val = (float)sliderLayout->slider.position() / 100.0;

        _settings->set<float>(ident, val);

        sliderLayout->value.setText( GUIKIT::String::convertDoubleToString(val, 2) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };    
}

auto AudioLayout::initDsp(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {
    auto val = _settings->get<float>(ident, defaultVal,{0.0, 1.0});
    sliderLayout->slider.setPosition((unsigned) (val * 100.0));
    sliderLayout->value.setText(GUIKIT::String::convertDoubleToString(val, 2));
}

auto AudioLayout::translate() -> void {
    if (dynamic_cast<LIBC64::Interface*>(emulator))
        settingsLayout.translate( );

    moduleFrame.setText(trans->get("selection"));
    
    bass.setText(trans->get("Bass Boost"));
    bass.top.active.setText(trans->get("enable"));
    bass.top.frequency.name.setText(trans->get("Cutoff frequency",{}, true));
    bass.bottom.gain.name.setText(trans->get("Gain",{}, true));
    bass.bottom.reduceClipping.name.setText(trans->get("Reduce Clipping",{}, true));

    reverb.setText(trans->get("Reverb"));
    reverb.top.active.setText(trans->get("enable"));
    reverb.top.wetTime.name.setText(trans->get("Wet Time",{}, true));
    reverb.top.dryTime.name.setText(trans->get("Dry Time",{}, true));
    reverb.bottom.damping.name.setText(trans->get("Damping",{}, true));
    reverb.bottom.roomWidth.name.setText(trans->get("Room Width",{}, true));
    reverb.bottom.roomSize.name.setText(trans->get("Room Size",{}, true));

    panning.setText(trans->get("Balance"));
    panning.top.active.setText(trans->get("enable"));
    panning.top.leftChannel.setText(trans->get("left Channel"));
    panning.top.leftMix.name.setText(trans->get("mix left"));
    panning.top.rightMix.name.setText(trans->get("mix right"));
    panning.bottom.rightChannel.setText(trans->get("right Channel"));
    panning.bottom.leftMix.name.setText(trans->get("mix left"));
    panning.bottom.rightMix.name.setText(trans->get("mix right"));

    driveLayout.setText( trans->get("Drive Noise") );
    driveLayout.floppyVolume.active.setText( trans->get("Floppy") );
    driveLayout.selection.floppyLabel.setText( trans->get("Floppy Profile", {}, true) );
    driveLayout.selection.reload.setText( trans->get("Reload") );
    driveLayout.selection.reload.setTooltip( trans->get("reload samples tooltip") );

    audioRecord.setText(trans->get("Audio Record"));
    audioRecord.location.label.setText( trans->get("wav folder") );
    audioRecord.location.select.setText("...");

    audioRecord.duration.useTimeLimit.setText(trans->get("Recording time"));
    audioRecord.duration.minutesSlider.name.setText(trans->get("Minutes",{}, true));
    audioRecord.duration.secondsSlider.name.setText(trans->get("Seconds",{}, true));
    audioRecord.duration.record.setText( trans->get( audioManager->record.run(emulator) ? "Stop" : "Record") );

    if (dynamic_cast<LIBC64::Interface*> (emulator)) {
        moduleList.setText(0, 0, trans->get("SID"));
        moduleList.setText(1, 0, trans->get("Drives"));
        moduleList.setText(2, 0, trans->get("DSP"));
        moduleList.setText(3, 0, trans->get("Audio Record"));
        moduleList.setRowTooltip(2, trans->get("Digital Signal Processing"));
    } else {
        moduleList.setText(0, 0, trans->get("Drive Noise"));
        moduleList.setText(1, 0, trans->get("DSP"));
        moduleList.setText(2, 0, trans->get("Audio Record"));
        moduleList.setRowTooltip(1, trans->get("Digital Signal Processing"));
    }
}

auto AudioLayout::loadSettings() -> void {
    if (dynamic_cast<LIBC64::Interface*>(emulator))
        settingsLayout.updateWidgets();
    
    initDsp( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    initDsp( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    initDsp( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    initDsp( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    initDsp( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );
    
    initDsp( &panning.top.leftMix, "audio_panning_left0", 1.0 );
    initDsp( &panning.top.rightMix, "audio_panning_left1", 0.0 );
    initDsp( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
    initDsp( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );
    
    panning.top.active.setChecked( _settings->get<bool>("audio_panning", false ) );
    reverb.top.active.setChecked( _settings->get<bool>("audio_reverb", false ) );
    bass.top.active.setChecked(_settings->get<bool>("audio_bass", false));

    auto bassFreq = _settings->get<unsigned>("audio_bass_freq", 200, {20, 200});
    bass.top.frequency.slider.setPosition(bassFreq - 20);
    bass.top.frequency.value.setText(std::to_string(bassFreq) + " Hz");

    auto bassGain = _settings->get<unsigned>("audio_bass_gain", 10, {0, 40});
    bass.bottom.gain.slider.setPosition(bassGain);
    bass.bottom.gain.value.setText(std::to_string(bassGain));

    auto bassReduceClipping = _settings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0});
    bass.bottom.reduceClipping.slider.setPosition((unsigned) (bassReduceClipping * 10.0));
    bass.bottom.reduceClipping.value.setText(GUIKIT::String::convertDoubleToString(bassReduceClipping, 1));
    
    audioRecord.location.path.setText( _settings->get<std::string>( "audio_record_path", "" ) );
    
    unsigned value = _settings->get<unsigned>( "audio_record_minutes", 0, {0, 120} );
    
    audioRecord.duration.minutesSlider.value.setText( std::to_string(value) );
    
    audioRecord.duration.minutesSlider.slider.setPosition( value );
    
    value = _settings->get<unsigned>( "audio_record_seconds", 0, {0, 59} );

    audioRecord.duration.record.setChecked( audioManager->record.run(emulator) );

    audioRecord.duration.secondsSlider.value.setText( std::to_string(value) );
    
    audioRecord.duration.secondsSlider.slider.setPosition( value ); 
                 
    audioRecord.duration.useTimeLimit.setChecked( _settings->get<bool>( "audio_record_timelimit", false) );

    driveLayout.floppyVolume.active.setChecked( _settings->get<bool>( "audio_floppy", false) );

    unsigned floppyVolume = _settings->get<unsigned>("audio_floppy_volume", 100u, {0u, 300u});

    driveLayout.floppyVolume.slider.setPosition( floppyVolume );

    driveLayout.floppyVolume.value.setText(std::to_string(floppyVolume) + " %");

    //std::string folderBefore = driveLayout.selection.floppyCombo.text();

    std::string folder = _settings->get<std::string>("audio_floppy_folder", "");

    driveLayout.selection.floppyCombo.setSelectionByRow( folder );

    folder = driveLayout.selection.floppyCombo.text();

  //  if (!init) {
        // in case if content doesn't match setting anymore
        _settings->set<std::string>("audio_floppy_folder", folder);

      //  if (folderBefore != folder)
        //    audioManager->drive.unload(emulator, emulator->getDiskMediaGroup());
   // }

    updateVisibility();
}

auto AudioLayout::updateVisibility() -> void {
    
    reverb.setEnabled( reverb.top.active.checked() );
    
    reverb.top.active.setEnabled();
    
    panning.setEnabled( panning.top.active.checked() );
    
    panning.top.active.setEnabled();
    
    bass.setEnabled( bass.top.active.checked() );
    
    bass.top.active.setEnabled();
    
    audioRecord.duration.setEnabled( audioRecord.duration.useTimeLimit.checked() );
    audioRecord.duration.useTimeLimit.setEnabled();
    audioRecord.duration.record.setEnabled();
}

auto AudioLayout::toggleRecord() -> void {
    audioRecord.duration.record.toggle();
}

auto AudioLayout::stopRecord() -> void {

    if (audioRecord.duration.record.checked()) {
        audioRecord.duration.record.setText( trans->get( "Record" ) );
        audioRecord.duration.record.setChecked(false);
    }
}
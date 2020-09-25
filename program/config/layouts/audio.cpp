
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
    
    setPadding( 10 );
}

AudioControlLayout::AudioControlLayout() {
    GUIKIT::LineEdit test;
    test.setText( "0.0005" );
    append(frequencyLabel, {0u, 0u}, 5);
    append(frequencyCombo, {0u, 0u}, 20);
    append(priorityCheckbox, {0u, 0u}, 20);
    append(maxRateLabel, {0u, 0u}, 5);
    append(maxRateEdit, {test.minimumSize().width, 0u});    
    
    setAlignment( 0.5 );
}

AudioLayout::AudioLayout() : 
latency("ms"),
volume("%", false, true) {
    setMargin(10);

    frame.append(control, {~0u, 0u}, 20);
    frame.append(latency, {~0u, 0u}, 10);
    frame.append(volume, {~0u, 0u});
	frame.setPadding(10);
    
	append(frame, {~0u, 0u}, 10);
    append(bass, {~0u, 0u}, 10);
    append(reverb, {~0u, 0u}, 10);
    append(panning, {~0u, 0u}, 10);

    volume.slider.setLength(101);
    latency.slider.setLength(120);

    control.frequencyCombo.append( "44100 Hz", 44100 );
    control.frequencyCombo.append( "48000 Hz", 48000 );

    
    auto selectedDriver = program->getAudioDriver();
	unsigned i = 0;
	for(auto& driver : audioDriver->available()) {
		control.driverLayout.combo.append( driver );
		if (driver == selectedDriver) {
			control.driverLayout.combo.setSelection( i );
		}
		i++;
	}
    
    if (control.driverLayout.combo.rows() > 0) control.append(control.driverLayout, {~0u, 0u});
    if (control.driverLayout.combo.rows() == 1) control.driverLayout.setEnabled(false);
	
	control.driverLayout.combo.onChange = [this]() {
		globalSettings->set<std::string>("audio_driver", control.driverLayout.combo.text() );
        if (activeEmulator)
            EmuConfigView::TabWindow::getView(activeEmulator)->miscLayout->stopRecord();        
		program->initAudio();
	};
	    	
    control.frequencyCombo.onChange = [this]() {
        globalSettings->set<unsigned>("audio_frequency_v2", control.frequencyCombo.userData());
        if (activeEmulator)
            EmuConfigView::TabWindow::getView(activeEmulator)->miscLayout->stopRecord();        
        audioManager->setFrequency();
        audioManager->setAudioDsp();
    };
    
    latency.slider.onChange = [this]() {
        auto value = latency.slider.position();
        auto minimumLatency = audioDriver->getMinimumLatency();
        
        globalSettings->set<unsigned>("audio_latency", value + minimumLatency);
        updateLatencySlider();
        if (activeEmulator)
            EmuConfigView::TabWindow::getView(activeEmulator)->miscLayout->stopRecord();
        audioManager->setLatency();
    };
    
    volume.slider.onChange = [this]() {
        auto value = volume.slider.position();
        globalSettings->set<unsigned>("audio_volume", value);
        volume.value.setText( std::to_string( value ) + " %" );
        audioManager->setVolume();
    };
    
    volume.defaultButton.onActivate = [this]() {
        globalSettings->set<unsigned>("audio_volume", 100);
        volume.value.setText( std::to_string( 100 ) + " %" );
        volume.slider.setPosition( 100 );
        audioManager->setVolume();
    };    
    
    control.maxRateEdit.onChange = [this]() {
        globalSettings->set<std::string>("rate_control_delta", control.maxRateEdit.text() );
        audioManager->setRateControl();
    };
    
    control.maxRateEdit.setText( GUIKIT::String::formatFloatingPoint( globalSettings->get<double>("rate_control_delta", 0.005, {0.0, 0.010}) ) );
       
    control.priorityCheckbox.onToggle = [this]() {
        bool state = control.priorityCheckbox.checked();        
        globalSettings->set<bool>("audio_priority", state);
        audioDriver->setHighPriority( state );
    };
    
    control.priorityCheckbox.setChecked( globalSettings->get<bool>("audio_priority", false) );
    
    auto valVolume = globalSettings->get<unsigned>("audio_volume", 100u, {0u, 100u});
    volume.value.setText(std::to_string( valVolume ) + " %" );
    volume.slider.setPosition( valVolume );
        
    auto valFre = globalSettings->get<unsigned>("audio_frequency_v2", 48000);
    for(unsigned i = 0; i < control.frequencyCombo.rows(); i++) {
        if(control.frequencyCombo.userData(i) == valFre) {
            control.frequencyCombo.setSelection(i);
            break;
        }
    }
    
    bass.top.active.onToggle = [this]() {
        
        globalSettings->set<bool>("audio_bass", bass.top.active.checked() );
        
        audioManager->setAudioDsp();
    };
    
    bass.top.frequency.slider.onChange = [this]() {
        
        unsigned val = bass.top.frequency.slider.position() + 20;
        
        globalSettings->set<unsigned>("audio_bass_freq", val );
        
        bass.top.frequency.value.setText( std::to_string(val) + " Hz" );
        
        audioManager->setAudioDsp();
    };
    
    bass.bottom.gain.slider.onChange = [this]() {
        
        unsigned val = bass.bottom.gain.slider.position();
        
        globalSettings->set<unsigned>("audio_bass_gain", val );
        
        bass.bottom.gain.value.setText( std::to_string(val) );
        
        audioManager->setAudioDsp();
    };
    
    bass.bottom.reduceClipping.slider.onChange = [this]() {
        
        float val = (float)bass.bottom.reduceClipping.slider.position() / 10.0;
        
        globalSettings->set<float>("audio_bass_clipping", val);
        
        bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( val, 1) );
        
        audioManager->setAudioDsp();
    };
    
    bass.top.active.setChecked( globalSettings->get<bool>("audio_bass", false ) );
    
    auto bassFreq = globalSettings->get<unsigned>("audio_bass_freq", 200, {20, 200} );
    bass.top.frequency.slider.setPosition( bassFreq - 20 );
    bass.top.frequency.value.setText( std::to_string(bassFreq) + " Hz" );
    
    auto bassGain = globalSettings->get<unsigned>("audio_bass_gain", 10, {0, 40} );
    bass.bottom.gain.slider.setPosition( bassGain );
    bass.bottom.gain.value.setText( std::to_string(bassGain) );
    
    auto bassReduceClipping = globalSettings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0} );
    bass.bottom.reduceClipping.slider.setPosition( (unsigned)(bassReduceClipping * 10.0) );
    bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( bassReduceClipping, 1) );
    
    // reverb
    reverb.top.active.onToggle = [this]() {
        
        globalSettings->set<bool>("audio_reverb", reverb.top.active.checked() );
        
        audioManager->setAudioDsp();
    };   
    
    reverb.top.active.setChecked( globalSettings->get<bool>("audio_reverb", false ) );
    
    build100PercentSetting( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    build100PercentSetting( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    build100PercentSetting( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    build100PercentSetting( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    build100PercentSetting( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );
    
    // panning
    panning.top.active.onToggle = [this]() {
        
        globalSettings->set<bool>("audio_panning", panning.top.active.checked() );
        
        audioManager->setAudioDsp();
    };  
    
    panning.top.active.setChecked( globalSettings->get<bool>("audio_panning", false ) );
    
    build100PercentSetting( &panning.top.leftMix, "audio_panning_left0", 1.0 );
    build100PercentSetting( &panning.top.rightMix, "audio_panning_left1", 0.0 );
    build100PercentSetting( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
    build100PercentSetting( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );
}

auto AudioLayout::build100PercentSetting(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {

    sliderLayout->slider.onChange = [this, sliderLayout, ident]() {

        float val = (float)sliderLayout->slider.position() / 100.0;

        globalSettings->set<float>(ident, val);

        sliderLayout->value.setText( GUIKIT::String::convertDoubleToString(val, 2) );

        audioManager->setAudioDsp();
    };
    
    auto val = globalSettings->get<float>(ident, defaultVal, {0.0, 1.0});
    sliderLayout->slider.setPosition((unsigned) (val * 100.0));
    sliderLayout->value.setText(GUIKIT::String::convertDoubleToString(val, 2));
}

auto AudioLayout::updateLatencySlider() -> void {
    
    auto valLatency = globalSettings->get<unsigned>("audio_latency", 64u, {1u, 120u});
    auto minimumLatency = audioDriver->getMinimumLatency();
    auto maximumLatency = 120;
    valLatency = std::max( valLatency, minimumLatency );
    globalSettings->set<unsigned>("audio_latency", valLatency);
    
    latency.slider.setLength( maximumLatency - minimumLatency + 1 );    
    latency.value.setText(std::to_string( valLatency ) + " ms");
    latency.slider.setPosition(valLatency - minimumLatency);
}

auto AudioLayout::translate() -> void {    
    volume.name.setText( trans->get("volume", {}, true) );

    latency.name.setText( trans->get("latency", {}, true) );
    control.frequencyLabel.setText( trans->get("frequency", {}, true) );
    control.priorityCheckbox.setText( trans->get("audio high priority") );
    control.priorityCheckbox.setTooltip( trans->get("audio high priority tooltip") );

    volume.defaultButton.setText( trans->get("Max") );
    
    control.driverLayout.name.setText( trans->get("driver", {}, true) );
    
    control.maxRateLabel.setText( trans->get("drc_delta", {}, true) );
    control.maxRateLabel.setTooltip( trans->get("drc_delta_tooltip") );
    
    SliderLayout::scale({&latency, &volume}, "120 ms");
        
    bass.setText( trans->get("Bass Boost") );
    bass.top.active.setText( trans->get("enable") );    
    bass.top.frequency.name.setText( trans->get("Cutoff frequency", {}, true) );
    bass.bottom.gain.name.setText( trans->get("Gain", {}, true) );
    bass.bottom.reduceClipping.name.setText( trans->get("Reduce Clipping", {}, true) );
    
    reverb.setText( trans->get("Reverb") );
    reverb.top.active.setText( trans->get("enable") );    
    reverb.top.wetTime.name.setText( trans->get("Wet Time", {}, true) );
    reverb.top.dryTime.name.setText( trans->get("Dry Time", {}, true) );
    reverb.bottom.damping.name.setText( trans->get("Damping", {}, true) );
    reverb.bottom.roomWidth.name.setText( trans->get("Room Width", {}, true) );
    reverb.bottom.roomSize.name.setText( trans->get("Room Size", {}, true) );
    
    panning.setText( trans->get("Balance") );
    panning.top.active.setText( trans->get("enable") );    
    panning.top.leftChannel.setText( trans->get("left Channel") );
    panning.top.leftMix.name.setText( trans->get("mix left") );
    panning.top.rightMix.name.setText( trans->get("mix right") );
    panning.bottom.rightChannel.setText( trans->get("right Channel") );
    panning.bottom.leftMix.name.setText( trans->get("mix left") );
    panning.bottom.rightMix.name.setText( trans->get("mix right") );
}

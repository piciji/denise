
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

    volume.slider.setLength(101);
    latency.slider.setLength(120);

    control.frequencyCombo.append( "44100 Hz", 44100 );
    control.frequencyCombo.append( "48000 Hz", 48000 );

    
    auto selectedDriver = program->getAudioDriver();
	unsigned i = 0;
	for(auto& driver : audioDriver->available()) {
		driverLayout.combo.append( driver );
		if (driver == selectedDriver) {
			driverLayout.combo.setSelection( i );
		}
		i++;
	}
    
    if (driverLayout.combo.rows() > 0) append(driverLayout, {~0u, 0u});
    if (driverLayout.combo.rows() == 1) driverLayout.setEnabled(false);
	
	driverLayout.combo.onChange = [this]() {
		settings->set<std::string>("audio_driver", driverLayout.combo.text() );
		program->initAudio();
	};
	    	
    control.frequencyCombo.onChange = [this]() {
        settings->set<unsigned>("audio_frequency_v2", control.frequencyCombo.userData());
        audioManager->setFrequency();
        audioManager->setAudioDsp();
    };
    
    latency.slider.onChange = [this]() {
        auto value = latency.slider.position();
        auto minimumLatency = audioDriver->getMinimumLatency();
        
        settings->set<unsigned>("audio_latency", value + minimumLatency);
        updateLatencySlider();
        audioManager->setLatency();
    };
    
    volume.slider.onChange = [this]() {
        auto value = volume.slider.position();
        settings->set<unsigned>("audio_volume", value);
        volume.value.setText( std::to_string( value ) + " %" );
        audioManager->setVolume();
    };
    
    volume.defaultButton.onActivate = [this]() {
        settings->set<unsigned>("audio_volume", 100);
        volume.value.setText( std::to_string( 100 ) + " %" );
        volume.slider.setPosition( 100 );
        audioManager->setVolume();
    };    
    
    control.maxRateEdit.onChange = [this]() {
        settings->set<std::string>("rate_control_delta", control.maxRateEdit.text() );
        audioManager->setRateControl();
    };
    
    control.maxRateEdit.setText( GUIKIT::String::formatFloatingPoint( settings->get<double>("rate_control_delta", 0.005, {0.0, 0.010}) ) );
       
    control.priorityCheckbox.onToggle = [this]() {
        bool state = control.priorityCheckbox.checked();        
        settings->set<bool>("audio_priority", state);
        audioDriver->setHighPriority( state );
    };
    
    control.priorityCheckbox.setChecked( settings->get<bool>("audio_priority", false) );
    
    auto valVolume = settings->get<unsigned>("audio_volume", 100u, {0u, 100u});
    volume.value.setText(std::to_string( valVolume ) + " %" );
    volume.slider.setPosition( valVolume );
        
    auto valFre = settings->get<unsigned>("audio_frequency_v2", 48000);
    for(unsigned i = 0; i < control.frequencyCombo.rows(); i++) {
        if(control.frequencyCombo.userData(i) == valFre) {
            control.frequencyCombo.setSelection(i);
            break;
        }
    }
    
    bass.top.active.onToggle = [this]() {
        
        settings->set<bool>("audio_bass", bass.top.active.checked() );
        
        audioManager->setAudioDsp();
    };
    
    bass.top.frequency.slider.onChange = [this]() {
        
        unsigned val = bass.top.frequency.slider.position() + 20;
        
        settings->set<unsigned>("audio_bass_freq", val );
        
        bass.top.frequency.value.setText( std::to_string(val) + " Hz" );
        
        audioManager->setAudioDsp();
    };
    
    bass.bottom.gain.slider.onChange = [this]() {
        
        unsigned val = bass.bottom.gain.slider.position();
        
        settings->set<unsigned>("audio_bass_gain", val );
        
        bass.bottom.gain.value.setText( std::to_string(val) );
        
        audioManager->setAudioDsp();
    };
    
    bass.bottom.reduceClipping.slider.onChange = [this]() {
        
        float val = (float)bass.bottom.reduceClipping.slider.position() / 10.0;
        
        settings->set<float>("audio_bass_clipping", val);
        
        bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( val, 1) );
        
        audioManager->setAudioDsp();
    };
    
    bass.top.active.setChecked( settings->get<bool>("audio_bass", false ) );
    
    auto bassFreq = settings->get<unsigned>("audio_bass_freq", 200, {20, 200} );
    bass.top.frequency.slider.setPosition( bassFreq - 20 );
    bass.top.frequency.value.setText( std::to_string(bassFreq) + " Hz" );
    
    auto bassGain = settings->get<unsigned>("audio_bass_gain", 10, {0, 40} );
    bass.bottom.gain.slider.setPosition( bassGain );
    bass.bottom.gain.value.setText( std::to_string(bassGain) );
    
    auto bassReduceClipping = settings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0} );
    bass.bottom.reduceClipping.slider.setPosition( (unsigned)(bassReduceClipping * 10.0) );
    bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( bassReduceClipping, 1) );
    
    // reverb
    reverb.top.active.onToggle = [this]() {
        
        settings->set<bool>("audio_reverb", reverb.top.active.checked() );
        
        audioManager->setAudioDsp();
    };   
    
    reverb.top.active.setChecked( settings->get<bool>("audio_reverb", false ) );
    
    buildReverbSetting( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    buildReverbSetting( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    buildReverbSetting( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    buildReverbSetting( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    buildReverbSetting( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );
}

auto AudioLayout::buildReverbSetting(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {

    sliderLayout->slider.onChange = [this, sliderLayout, ident]() {

        float val = (float)sliderLayout->slider.position() / 100.0;

        settings->set<float>(ident, val);

        sliderLayout->value.setText( GUIKIT::String::convertDoubleToString(val, 2) );

        audioManager->setAudioDsp();
    };
    
    auto val = settings->get<float>(ident, defaultVal, {0.0, 1.0});
    sliderLayout->slider.setPosition((unsigned) (val * 100.0));
    sliderLayout->value.setText(GUIKIT::String::convertDoubleToString(val, 2));
}

auto AudioLayout::updateLatencySlider() -> void {
    
    auto valLatency = settings->get<unsigned>("audio_latency", 64u, {1u, 120u});
    auto minimumLatency = audioDriver->getMinimumLatency();
    auto maximumLatency = 120;
    valLatency = std::max( valLatency, minimumLatency );
    settings->set<unsigned>("audio_latency", valLatency);
    
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
    
    driverLayout.name.setText( trans->get("driver", {}, true) );
    
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
}

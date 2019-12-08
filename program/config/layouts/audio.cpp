
AudioControlLayout::AudioControlLayout() {
    GUIKIT::LineEdit test;
    test.setText( "0.0005" );
    append(frequencyLabel, {0u, 0u}, 10);
    append(frequencyCombo, {0u, 0u}, 20);
    append(reverb, {0u, 0u}, 20);
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
    frame.append(volume, {~0u, 0u}, 10);
	frame.setPadding(10);
    
	append(frame, {~0u, 0u}, 10);

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
    
    control.reverb.onToggle = [this]() {
        settings->set<bool>("audio_reverb", control.reverb.checked());
        audioManager->setAudioDsp();
    };
    
    control.maxRateEdit.onChange = [this]() {
        settings->set<std::string>("rate_control_delta", control.maxRateEdit.text() );
        audioManager->setRateControl();
    };
    
    control.maxRateEdit.setText( GUIKIT::String::formatFloatingPoint( settings->get<double>("rate_control_delta", 0.005, {0.0, 0.010}) ) );
    
    if (settings->get<bool>("audio_reverb", false ) )
        control.reverb.setChecked();
    
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

    volume.defaultButton.setText( trans->get("Max") );
    
    driverLayout.name.setText( trans->get("driver", {}, true) );
    
    control.reverb.setText( trans->get("Reverb") );
    control.maxRateLabel.setText( trans->get("drc_delta", {}, true) );
    control.maxRateLabel.setTooltip( trans->get("drc_delta_tooltip") );
    
    SliderLayout::scale({&latency, &volume}, "120 ms");
}

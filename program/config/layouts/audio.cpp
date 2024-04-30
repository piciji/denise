
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
latency("ms") {
    setMargin(10);

    frame.append(control, {~0u, 0u}, 20);
    frame.append(latency, {~0u, 0u}, 10);
	frame.setPadding(10);
    
	append(frame, {~0u, 0u}, 10);

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
        emuThread->lock();
		globalSettings->set<std::string>("audio_driver", control.driverLayout.combo.text() );
        audioManager->record.finish();
		program->initAudio();
        emuThread->unlock();
	};
	    	
    control.frequencyCombo.onChange = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("audio_frequency_v2", control.frequencyCombo.userData());
        audioManager->record.finish();
        audioManager->setFrequency();
        audioManager->setResampler();
        audioManager->setDriveSounds( false, true );
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    latency.slider.onChange = [this](unsigned position) {
        emuThread->lock();
        auto minimumLatency = audioDriver->getMinimumLatency();
        
        globalSettings->set<unsigned>("audio_latency", position + minimumLatency);
        updateLatencySlider();
        audioManager->record.finish();
        audioManager->setLatency();
        emuThread->unlock();
    };
    
    control.maxRateEdit.onChange = [this]() {
        emuThread->lock();
        globalSettings->set<std::string>("rate_control_delta", control.maxRateEdit.text() );
        audioManager->setRateControl();
        emuThread->unlock();
    };
    
    control.maxRateEdit.setText( GUIKIT::String::formatFloatingPoint( globalSettings->get<double>("rate_control_delta", 0.005, {0.0, 0.010}) ) );
       
    control.priorityCheckbox.onToggle = [this](bool checked) {
        emuThread->lock();
        globalSettings->set<bool>("audio_priority", checked);
        audioDriver->setHighPriority( checked );
        emuThread->unlock();
    };
    
    control.priorityCheckbox.setChecked( globalSettings->get<bool>("audio_priority", false) );
        
    auto valFre = globalSettings->get<unsigned>("audio_frequency_v2", 48000);
    for(unsigned i = 0; i < control.frequencyCombo.rows(); i++) {
        if(control.frequencyCombo.userData(i) == valFre) {
            control.frequencyCombo.setSelection(i);
            break;
        }
    }

    updateLatencySlider();
}

auto AudioLayout::updateLatencySlider() -> void {
    
    auto valLatency = globalSettings->get<unsigned>("audio_latency", 30u, {1u, 120u});
    auto minimumLatency = audioDriver->getMinimumLatency();
    auto maximumLatency = 120;
    valLatency = std::max( valLatency, minimumLatency );
    globalSettings->set<unsigned>("audio_latency", valLatency);
    
    latency.slider.setLength( maximumLatency - minimumLatency + 1 );    
    latency.value.setText(std::to_string( valLatency ) + " ms");
    latency.slider.setPosition(valLatency - minimumLatency);
}

auto AudioLayout::translate() -> void {
    latency.name.setText( trans->get("latency", {}, true) );
    control.frequencyLabel.setText( trans->get("frequency", {}, true) );
    control.priorityCheckbox.setText( trans->get("audio high priority") );
    control.priorityCheckbox.setTooltip( trans->get("audio high priority tooltip") );

    control.driverLayout.name.setText( trans->get("driver", {}, true) );
    
    control.maxRateLabel.setText( trans->get("drc_delta", {}, true) );
    control.maxRateLabel.setTooltip( trans->get("drc_delta_tooltip") );
    
    SliderLayout::scale({&latency}, "120 ms");
}

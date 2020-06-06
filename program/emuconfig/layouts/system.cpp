
FeatureLayout::Line::Block::Block(Emulator::Interface::Feature* feature) {
	this->feature = feature;
    
	if (feature->isSwitch()) {
		append(checkBox, {0u, 0u} );
		
	} else if (feature->isRadio()) {		
		append(label, {0u, 0u}, 5 );
		
		for(auto& option : feature->options) {
			auto radio = new GUIKIT::RadioBox;
			options.push_back( radio );
			append( *radio, {0u, 0u}, &feature->options.back() == &option ? 0 : 5 );	
		}
		GUIKIT::RadioBox::setGroup( options );
	
	} else {
        GUIKIT::LineEdit tester;
        tester.setText( feature->isHex() ? "0xFF" : std::to_string(feature->range[0]) );
		append(label, {0u, 0u}, 5 );
		append(lineEdit, {tester.minimumSize().width, 0u} );
	}
        
	setAlignment(0.5);
}

FeatureLayout::Line::Line() {
    setAlignment(0.5);
}

FeatureLayout::FeatureLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));    
}

AccuracyLayout::Block::Block() {    
    append(videoCycleAccuracy, {0u, 0u}, 10);
    append(videoScanlineThread, {0u, 0u}, 10);
    append(diskHighLoadThread, {0u, 0u}, 10);
    append(diskIdle, {0u, 0u}, 10);
    append(audioRealtimeThread, {0u, 0u});   
    
    setAlignment(0.5);
}

AccuracyLayout::AccuracyLayout() {
    setPadding(10);
    append( dangerLabel, {0u, 0u}, 5 );
    append( block, {0u, 0u} );
    dangerLabel.setForegroundColor(0xff4500);
    setFont(GUIKIT::Font::system("bold"));    
}

auto FeatureLayout::build( Emulator::Interface* emulator ) -> void {
    unsigned blocksPerLine = 4;
	auto& features = emulator->features;
	    
    Line* line;
    unsigned i = 0;
    unsigned lineCount = (features.size() / blocksPerLine);
    lineCount += ((features.size() % blocksPerLine) != 0) ? 1 : 0;
    
    for( auto& feature : features ) {
        
        if ((i++ % blocksPerLine) == 0) {
            line = new Line();            
            lines.push_back( line );
            append( *line, {~0u, 0u}, ( lines.size() < lineCount ) ? 10 : 0 );
        }
        
        auto block = new Line::Block( &feature );
        line->blocks.push_back( block );
        line->append(*block, {0u, 0u}, ((i % blocksPerLine) == 0) ? 0 : 15);

        
        block->checkBox.setText( feature.name );
        block->label.setText( feature.name );  
		
		unsigned j = 0;
		for (auto option : block->options) {
			option->setText( feature.options[j++] );
		}                        
    }
}

auto ExpansionLayout::build( Emulator::Interface* emulator ) -> void {
    unsigned blocksPerLine = 4;
    auto& expansions = emulator->expansions;
    
    Line* line;
    unsigned i = 0;
    unsigned lineCount = (expansions.size() / blocksPerLine);
    lineCount += ((expansions.size() % blocksPerLine) != 0) ? 1 : 0;
    std::vector<GUIKIT::RadioBox*> radios;
    
    for( auto& expansion : expansions ) {
        
        if ((i++ % blocksPerLine) == 0) {
            line = new Line();            
            lines.push_back( line );
            append( *line, {~0u, 0u}, ( lines.size() < lineCount ) ? 5 : 0 );
        }
        
        auto block = new Line::Block( );
        block->expansion = &expansion;        
        line->blocks.push_back( block );
        
        line->append( block->box, {0u, 0u}, ((i % blocksPerLine) == 0) ? 0 : 10);
        radios.push_back( &(block->box) );
        
        block->box.setText( expansion.name );
    }
    
    GUIKIT::RadioBox::setGroup( radios );
}

ExpansionLayout::Line::Line() {
    setAlignment(0.5);
}

ExpansionLayout::ExpansionLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));    
}

MemoryLayout::Block::Block() :
    sliderLayout("mb")            
{
    append(sliderLayout, {~0u, ~0u} );
}

MemoryLayout::MemoryLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

auto MemoryLayout::build( Emulator::Interface* emulator ) -> void {
    auto& memoryTypes = emulator->memoryTypes;
    
    for(auto& memoryType : memoryTypes ) {                
        auto block = new Block();
        blocks.push_back( block );
        block->memoryType = &memoryType;
        append(*block, {~0u, 0u}, &memoryType != &memoryTypes.back() ? 7 : 0);
        block->sliderLayout.slider.setLength( memoryType.memory.size() );
        block->sliderLayout.name.setText( memoryType.name + ":" );
    }          
}

DriveLayout::DriveCountFrame::DriveCount::DriveCount() {
    append(name, {0u, 0u}, 5);
    append(combo, {0u, 0u});
    setAlignment(0.5);
}

DriveLayout::DriveLayout() : speed("RPM"), wobble("RPM") {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

auto DriveLayout::build( Emulator::Interface* emulator ) -> void {
    for(auto& mediaGroup : emulator->mediaGroups) {
        if ( !mediaGroup.isDrive() )
            continue;
        
        auto driveCount = new DriveCountFrame::DriveCount;
        driveCount->mediaGroup = &mediaGroup;

        for(unsigned i = 0; i <= mediaGroup.media.size(); i++) {
            driveCount->combo.append( std::to_string(i) );
        }
        driveCountFrame.append(*driveCount, {0u,0u}, 15u);
        driveCountFrame.driveCounter.push_back(driveCount);
        
        if (mediaGroup.isDisk() && dynamic_cast<LIBC64::Interface*>(emulator) )
            driveCount->name.setForegroundColor(0xff4500);
    }
    
    append( driveCountFrame, {~0u, 0u}, 5 );
    append( speed, {~0u, 0u}, 5);
    append( wobble, {~0u, 0u});
    
    speed.slider.setLength( 501 );
    wobble.slider.setLength( 51 );
}

CpuLayout::CpuLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));	
}

auto CpuLayout::build( Emulator::Interface* emulator ) -> void {
    auto& cpus = emulator->cpus;
    if (!cpus.size())
        return;
    
    for(auto& cpu : cpus) {
        auto radio = new GUIKIT::RadioBox;
        selector.radios.push_back( radio );
        radio->setText( cpu.name );
        selector.append(*radio, {0u, 0u}, &cpu != &cpus.back() ? 15 : 0);
    }
    GUIKIT::RadioBox::setGroup( selector.radios );	
    
    append(selector, {~0u, 0u});
}

ChipsetLayout::ChipsetLayout() {
	setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    append(selector, {~0u, 0u});
}

RegionLayout::RegionLayout() {
	setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    append(pal, {0u, 0u}, 10u);
	append(ntsc, {0u, 0u});
	
	GUIKIT::RadioBox::setGroup(pal, ntsc);
}

auto ChipsetLayout::build( Emulator::Interface* emulator ) -> void {
	
	auto& chipsets = emulator->chipsets;
	
    for(auto& chipset : chipsets ) {
        auto radio = new GUIKIT::RadioBox;
        selector.radios.push_back( radio );
        radio->setText( chipset.name );
        selector.append(*radio, {0u, 0u}, &chipset != &chipsets.back() ? 10 : 0);
    }
    GUIKIT::RadioBox::setGroup( selector.radios );	
}

SystemLayout::SystemLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    memoryLayout.build( emulator );
    cpuLayout.build( emulator );
    driveLayout.build( emulator );
    featureLayout.build( emulator );
    chipsetLayout.build( emulator );
    expansionLayout.build( emulator );

    setMargin(10);
    
    leftLayout.append(expansionLayout, {~0u, 0u}, 10);
    leftLayout.append(memoryLayout, {~0u, 0u}, 10);
    
    upperLayout.append(leftLayout, {~0u, 0u}, 10);
    rightLayout.append(driveLayout, {~0u, 0u}, 10);
	
    if (emulator->cpus.size())
        bottomLayout.append(cpuLayout, {0u, 0u}, 10);
		
	bottomLayout.append(regionLayout, {0u, 0u}, 10);
	bottomLayout.append(chipsetLayout, {0u, 0u});	
	
    rightLayout.append(bottomLayout, {~0u, 0u});

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);
    
    if (featureLayout.lines.size() > 0)
        append(featureLayout, {~0u, 0u}, 10);
        
    append(accuracyLayout, {~0u, 0u});

    if (emulator->cpus.size()) {        
        unsigned i = 0;
        for ( auto radio : cpuLayout.selector.radios ) {
            radio->onActivate = [this, i, radio]() {
                settings->set<unsigned>( this->tabWindow->ident("cpu"), i );
            };
            i++;
        }

        cpuLayout.selector.radios[0]->setChecked();
        for(auto& cpu : emulator->cpus) {
            if (cpu.id == settings->get<unsigned>( tabWindow->ident("cpu"), 0)) {
                cpuLayout.selector.radios[cpu.id]->setChecked();
            }
        }
    }
		
    for( auto block : memoryLayout.blocks ) {
        auto memoryType = block->memoryType;
        
        block->sliderLayout.slider.onChange = [this, block, memoryType]() {
            unsigned id = block->sliderLayout.slider.position();
            if (id >= memoryType->memory.size() ) return;
            settings->set<unsigned>( this->tabWindow->ident(memoryType->name + "_mem"), id);
            block->sliderLayout.value.setText( getSizeString( memoryType->memory[id].size ) );
        };

        unsigned id = settings->get<unsigned>(tabWindow->ident(memoryType->name + "_mem"), memoryType->defaultMemoryId);
        if (id >= memoryType->memory.size())
            id = memoryType->defaultMemoryId;
        block->sliderLayout.slider.setPosition(id);
        block->sliderLayout.value.setText( getSizeString( memoryType->memory[id].size ) );        
    }
    
    for(auto block : driveLayout.driveCountFrame.driveCounter) {
        
        auto ident = block->mediaGroup->name + "_count";
        
        block->combo.onChange = [this, ident, block]() {
            settings->set<unsigned>( this->tabWindow->ident(ident), block->combo.selection());

            // check if media elements of group have to be rebuilt
            MediaView::MediaWindow::getView(this->emulator)->updateVisibility( block->mediaGroup, block->combo.selection() );
            settings->remove( this->tabWindow->ident("access_floppy") );
        };
        
        unsigned counter = settings->get<unsigned>( tabWindow->ident(ident), block->mediaGroup->defaultUsage());
        if (counter >= block->combo.rows())
            counter = block->mediaGroup->defaultUsage();
        
        block->combo.setSelection( counter );
        
        if (block->mediaGroup->isDisk()) {
            
            auto ident = block->mediaGroup->name;
            
            driveLayout.speed.slider.onChange = [this, block, ident]() {                
                
                auto position = driveLayout.speed.slider.position();

                double speed = (double)(position) / 10.0 + 275.0;

                driveLayout.speed.value.setText(GUIKIT::String::formatFloatingPoint(speed, 1) + " RPM");
                
                settings->set<double>(this->tabWindow->ident(ident + "_speed"), speed);
            };                       
            
            driveLayout.wobble.slider.onChange = [this, block, ident]() {
                
                auto position = driveLayout.wobble.slider.position();

                double wobble = (double) position / 10.0;

                driveLayout.wobble.value.setText(GUIKIT::String::formatFloatingPoint(wobble, 2) + " RPM");                

                settings->set<double>(this->tabWindow->ident(ident + "_wobble"), wobble);
            };
            
            double wobble = settings->get<double>(this->tabWindow->ident(ident + "_wobble"), 0.5, {0.0, 5.0});
            double speed = settings->get<double>(this->tabWindow->ident(ident + "_speed"), 300.0, {275.0, 325.0});
                        
            driveLayout.wobble.value.setText(GUIKIT::String::formatFloatingPoint(wobble, 2) + " RPM");
            driveLayout.speed.value.setText(GUIKIT::String::formatFloatingPoint(speed, 1) + " RPM");
            
            driveLayout.wobble.slider.setPosition( wobble * 10.0 );
            driveLayout.speed.slider.setPosition( (speed - 275.0) * 10.0 );
        }
    }
               
    auto expansionId = settings->get<unsigned>( this->tabWindow->ident("expansion"), 0);
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {            
            block->box.onActivate = [this, block]() {                
                settings->set<unsigned>( this->tabWindow->ident("expansion"), block->expansion->id);
                updateExpansionMemory();
            };
            if (block->expansion->id == expansionId)
                block->box.setChecked();
        }
    }   
    
	for( auto line : featureLayout.lines ) {
        
        for( auto block : line->blocks ) {

            auto feature = block->feature;
            
            if (feature->isSwitch() ) {	

                block->checkBox.onToggle = [this, block, feature]( ) {

                    settings->set<bool>( this->tabWindow->ident( feature->name ), block->checkBox.checked( ) );

                    emulator->setFeature( feature->id, block->checkBox.checked( ) );
                };

			} else if (feature->isRadio() ) {	
				unsigned val = 0;
				for( auto option : block->options ) {
					
					option->onActivate = [this, block, feature, val]() {

						settings->set<unsigned>(this->tabWindow->ident(feature->name), val);
						
						emulator->setFeature( feature->id, val );
					};
					val++;
				}
				
            } else {

                block->lineEdit.onChange = [this, block, feature]() {
                                        
                    int val;
                    auto str = block->lineEdit.text();

                    if ( feature->isHex() ) {                    
                        val = GUIKIT::String::convertHexToInt( str, feature->defaultValue );
                    } else
                        val = block->lineEdit.value();

                    auto range = feature->range;

                    if (val < range[0])
                        val = range[0];

                    if (val > range[1])
                        val = range[1];

                    settings->set<int>( this->tabWindow->ident( feature->name ), val );

                    emulator->setFeature( feature->id, val );
                };			
            }

            updateFeatureWidget( block );
        }
        
	}
	
	chipsetLayout.selector.radios[0]->setChecked();
	for(auto& chipset : emulator->chipsets) {
		auto radio = chipsetLayout.selector.radios[chipset.id];
		auto id = chipset.id;
		if (id == settings->get( tabWindow->ident("chipset"), 0))
            radio->setChecked();
		
		radio->onActivate = [this, id, radio]( ) {
			settings->set( this->tabWindow->ident( "chipset" ), id );
		};
	}
	
	regionLayout.pal.onActivate = [this]() {
        if (this->emulator == activeEmulator) {
            if (!mes->question( trans->get("setting change need reset") )) {
                regionLayout.ntsc.setChecked();
                return;
            }
        }
        
        settings->set<unsigned>(this->tabWindow->ident("video_region"), 0);
		this->tabWindow->videoLayout->updatePresets();

        view->getSysMenu( this->emulator )->pal->setChecked();
        
        if (activeEmulator)
            program->power(activeEmulator);
    };
    
    regionLayout.ntsc.onActivate = [this]() {
        if (this->emulator == activeEmulator) {
            if (!mes->question( trans->get("setting change need reset") )) {
                regionLayout.pal.setChecked();
                return;
            }
        }
        
        settings->set<unsigned>(this->tabWindow->ident("video_region"), 1);
		this->tabWindow->videoLayout->updatePresets();

        view->getSysMenu( this->emulator )->ntsc->setChecked();
        
        if (activeEmulator)
            program->power(activeEmulator);
    };
	
	if (settings->get<unsigned>( tabWindow->ident("video_region"), 0, {0u, 1u}) == 0 )
        regionLayout.pal.setChecked();
    else
        regionLayout.ntsc.setChecked();
    
    accuracyLayout.block.videoCycleAccuracy.onToggle = [this]() {

        bool state = accuracyLayout.block.videoCycleAccuracy.checked();
        
        if (this->emulator == activeEmulator) {
            if (!mes->question(trans->get("setting change need reset"))) {
                accuracyLayout.block.videoCycleAccuracy.setChecked(!state);
                return;
            }
        }                
        
        settings->set<bool>(this->tabWindow->ident("video_cycle_accuracy"), state);

        program->fastForward(false);

        emulator->videoCycleAccuracy(state);
        
        if (this->emulator == activeEmulator)
            program->power(activeEmulator);
    };
    
    accuracyLayout.block.videoCycleAccuracy.setChecked( settings->get<bool>(this->tabWindow->ident("video_cycle_accuracy"), false) );
    
    accuracyLayout.block.videoScanlineThread.onToggle = [this]() {

        bool state = accuracyLayout.block.videoScanlineThread.checked();
        
        settings->set<bool>(this->tabWindow->ident("video_scanline_thread"), state);

        program->fastForward(false);

        emulator->videoScanlineThread(state);
    };    
    
    accuracyLayout.block.videoScanlineThread.setChecked( settings->get<bool>(this->tabWindow->ident("video_scanline_thread"), false) );
    
    accuracyLayout.block.diskHighLoadThread.onToggle = [this]() {

        bool state = accuracyLayout.block.diskHighLoadThread.checked();
        
        settings->set<bool>(this->tabWindow->ident("disk_highload_thread"), state);

        program->fastForward(false);

        emulator->diskHighLoadThread(state);
    };  
    
    accuracyLayout.block.diskHighLoadThread.setChecked( settings->get<bool>(this->tabWindow->ident("disk_highload_thread"), false) );

    accuracyLayout.block.diskIdle.onToggle = [this]() {

        bool state = accuracyLayout.block.diskIdle.checked();
        
        settings->set<bool>(this->tabWindow->ident("disk_idle"), state);

        program->fastForward(false);

        emulator->diskIdle(state);
    };  
    
    accuracyLayout.block.diskIdle.setChecked( settings->get<bool>(this->tabWindow->ident("disk_idle"), false) );

    
    accuracyLayout.block.audioRealtimeThread.onToggle = [this]() {

        bool state = accuracyLayout.block.audioRealtimeThread.checked();
        
        settings->set<bool>(this->tabWindow->ident("audio_realtime_thread"), state);

        program->fastForward(false);

        emulator->audioRealtimeThread(state);
    }; 
    
    accuracyLayout.block.audioRealtimeThread.setChecked( settings->get<bool>(this->tabWindow->ident("audio_realtime_thread"), false) );
    
    updateExpansionMemory();
}

auto SystemLayout::activateDrive( Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void {

    if (requestedCount > mediaGroup->media.size())
        requestedCount = mediaGroup->media.size();
    
    for (auto block : driveLayout.driveCountFrame.driveCounter) {

        if (mediaGroup != block->mediaGroup)
            continue;
        
        auto ident = mediaGroup->name + "_count";
        
        unsigned counter = settings->get<unsigned>( tabWindow->ident(ident), mediaGroup->defaultUsage());
        
        if (counter >= requestedCount)
            break;
        
        block->combo.setSelection( requestedCount );
        settings->set<unsigned>( this->tabWindow->ident(ident), requestedCount);
        settings->remove( this->tabWindow->ident("access_floppy") );
        
        MediaView::MediaWindow::getView(this->emulator)->updateVisibility( mediaGroup, requestedCount );
    }
}

auto SystemLayout::toggleFeature(unsigned id) -> bool {
	for(auto line : featureLayout.lines) {
        for( auto block : line->blocks ) {            
            if (block->feature->id == id) {
                bool newState = block->checkBox.checked() ^ 1;
                block->checkBox.setChecked( newState );
                block->checkBox.onToggle();
                return newState;
            }
        }        
	}
	return false;
}

auto SystemLayout::updateRuntimeFeatureWidgets( ) -> void {
    for (auto line : featureLayout.lines) {
        for (auto block : line->blocks) {
                        
            updateFeatureWidget( block );
        }
    }
}

auto SystemLayout::updateFeatureWidget( FeatureLayout::Line::Block* block ) -> void {	
	auto feature = block->feature;
	
	if (feature->isSwitch() ) {
		block->checkBox.setChecked( settings->get<bool>( tabWindow->ident( feature->name ), feature->defaultValue ) );
		return;
	}
		
	if (feature->isRadio() ) {
		auto usedVal = settings->get<unsigned>( tabWindow->ident( feature->name ), feature->defaultValue );
		
		unsigned val = 0;
		for(auto option : block->options) {
			if ( val++ == usedVal) {
				option->setChecked();
				break;
			}
		}
		
		return;
	}
	
	auto _val = settings->get<int>( tabWindow->ident( feature->name ), feature->defaultValue, feature->range );

	if ( feature->isHex() )                 
		block->lineEdit.setText( GUIKIT::String::convertIntToHex( _val ) );
	else            
		block->lineEdit.setValue( _val );	
}

auto SystemLayout::stepRangeFeature(unsigned id, int step) -> int {
    for(auto line : featureLayout.lines) {
        for( auto block : line->blocks ) {
            auto feature = block->feature;
            
            if (feature->id == id) {
                auto newValue = settings->get<int>( tabWindow->ident( feature->name ), feature->defaultValue, feature->range );
                newValue += step;						
                block->lineEdit.setValue( newValue );            
                block->lineEdit.onChange();
                return newValue;
            }
        }
	}
	return 0;
}

auto SystemLayout::translate() -> void {
    memoryLayout.setText( trans->get("memory") );
    driveLayout.setText( trans->get("drives") );
    cpuLayout.setText("Cpu");
	regionLayout.setText( trans->get("region") );
    
	chipsetLayout.setText( trans->get("Chipset") );
    featureLayout.setText( trans->get("feature") );
    expansionLayout.setText( trans->get("expansion_port") );
	
	regionLayout.pal.setText( trans->get("PAL") );
    regionLayout.ntsc.setText( trans->get("NTSC") );      

    for(auto block : driveLayout.driveCountFrame.driveCounter) {

        auto ident = block->mediaGroup->name + "_drives";
        block->name.setText(trans->get(ident,{}, true));
        
        if (block->mediaGroup->isDisk() && dynamic_cast<LIBC64::Interface*>(emulator) )
            block->name.setTooltip(trans->get("cpu_warning_disk_info"));
    }        
	
    for( auto line : featureLayout.lines ) {
        for( auto block : line->blocks ) {                               
            auto feature = block->feature;

            if (feature->isSwitch() )
                block->checkBox.setTooltip( trans->get( featureIdent( feature->name ) + "_info" ) );
			
			else if (feature->isRadio() ) {
				unsigned pos = 0;
				for(auto option : block->options) {
					option->setText( trans->get( feature->options[pos++] ) );
				}
				block->label.setTooltip( trans->get( featureIdent( feature->name ) + "_info" ) );
				
            } else
                block->label.setTooltip( trans->get( featureIdent( feature->name ) + "_info" ) );
            
            block->checkBox.setText( trans->get( featureIdent( feature->name ) ) );
            block->label.setText( trans->get( featureIdent( feature->name ), {}, feature->isRadio() ) );  
        }
	}
    
    for( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {                               
            block->box.setText( trans->get( block->expansion->name ) );
        }
    }
    
    std::vector<SliderLayout*> sliderLayouts;
    for(auto block : memoryLayout.blocks ) {    
        sliderLayouts.push_back( &block->sliderLayout );
    }
    
    SliderLayout::scale(sliderLayouts, "1024 mb");
        
    driveLayout.speed.name.setText( trans->get("Speed", {}, true) );
    driveLayout.wobble.name.setText( trans->get("Variation", {}, true) );
    
    sliderLayouts.clear();    
    sliderLayouts.push_back( &driveLayout.speed );
    sliderLayouts.push_back( &driveLayout.wobble );

    SliderLayout::scale(sliderLayouts, "300.0 RPM");
    
    accuracyLayout.setText( trans->get("accuracy and performance") ); 
    accuracyLayout.dangerLabel.setText( trans->get("cpu load") );  
    accuracyLayout.dangerLabel.setTooltip( trans->get("cpu load info") );  
    accuracyLayout.block.videoCycleAccuracy.setText( trans->get("video cycle accuracy") );
    accuracyLayout.block.videoCycleAccuracy.setTooltip( trans->get("video cycle accuracy info") );
    accuracyLayout.block.videoScanlineThread.setText( trans->get("video scanline thread") );
    accuracyLayout.block.videoScanlineThread.setTooltip( trans->get("video scanline thread info") );
    accuracyLayout.block.diskHighLoadThread.setText( trans->get("disk highload thread") );
    accuracyLayout.block.diskHighLoadThread.setTooltip( trans->get("disk highload thread info") );
    accuracyLayout.block.diskIdle.setText( trans->get("disk idle") );
    accuracyLayout.block.diskIdle.setTooltip( trans->get("disk idle info") );
    accuracyLayout.block.audioRealtimeThread.setText( trans->get("audio realtime thread") );
    accuracyLayout.block.audioRealtimeThread.setTooltip( trans->get("audio realtime thread info") );
}

auto SystemLayout::featureIdent( std::string ident ) -> std::string {
    
    return GUIKIT::String::toLowerCase( GUIKIT::String::replace(ident, " ", "_") );
}

auto SystemLayout::setEnabled(bool state) -> void {
    upperLayout.setEnabled( state );
	regionLayout.setEnabled( true );
    
    if (state)
        updateExpansionMemory();
}

auto SystemLayout::getSizeString( unsigned sizeInKb ) -> std::string {
    
    if (sizeInKb < 1024)
        return std::to_string( sizeInKb ) + " kb";
    
    float _size = (float)sizeInKb / 1024.0;
    
    return GUIKIT::String::convertDoubleToString( _size, 1 ) + " mb";
}

auto SystemLayout::updateExpansionMemory() -> void {
    
    Emulator::Interface::Expansion* expansionSelected = nullptr;
    
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {  
            if (block->box.checked()) {
                expansionSelected = block->expansion;
                break;
            }                
        }
    }
    
    for( auto& expansion : emulator->expansions ) {
        
        if (!expansion.memoryType)
            continue;
        
        for (auto block : memoryLayout.blocks) {
            
            if (block->memoryType == expansion.memoryType) {
                
                block->setEnabled( &expansion == expansionSelected );
                
                break;
            }
        }
    }    
}

auto SystemLayout::setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void {
    
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {   
            
            if (!newExpansion) {
                if (block->expansion->isEmpty()) {
                    if (!block->box.checked()) {
                        block->box.setChecked();
                        block->box.onActivate(); 
                    }
                    return;
                }
            }
            
            else if (block->expansion == newExpansion) {
                if (!block->box.checked()) {
                    block->box.setChecked();
                    block->box.onActivate(); 
                }
                return;
            }                
        }
    }
}

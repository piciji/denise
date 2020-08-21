
AccuracyLayout::Block::Block() {    
    append(videoCycleAccuracy, {0u, 0u}, 10);
    append(videoScanlineThread, {0u, 0u}, 10);
    append(diskHighLoadThread, {0u, 0u}, 10);
    append(diskIdle, {0u, 0u}, 10);
  //  append(audioRealtimeThread, {0u, 0u});   
    
    setAlignment(0.5);
}

AccuracyLayout::AccuracyLayout() {
    setPadding(10);
    append( dangerLabel, {0u, 0u}, 5 );
    append( block, {0u, 0u} );
    dangerLabel.setForegroundColor(0xff4500);
    setFont(GUIKIT::Font::system("bold"));    
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
    append( wobble, {~0u, 0u}, 5);
	append( tapeWobble, {~0u, 0u});
    
    speed.slider.setLength( 501 );
    wobble.slider.setLength( 51 );
}

SystemLayout::SystemLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    memoryLayout.build( emulator );
    driveLayout.build( emulator );
    modelLayout.build( tabWindow, emulator,
    {Emulator::Interface::Model::Purpose::Cpu, Emulator::Interface::Model::Purpose::GraphicChip, Emulator::Interface::Model::Purpose::SoundChip,
    Emulator::Interface::Model::Purpose::Cia, Emulator::Interface::Model::Purpose::Misc}, { 3, 3, 3 } );
    expansionLayout.build( emulator );

    setMargin(10);
    
    leftLayout.append(expansionLayout, {~0u, 0u}, 10);
    leftLayout.append(memoryLayout, {~0u, 0u});
    
    upperLayout.append(leftLayout, {~0u, 0u}, 10);
    rightLayout.append(driveLayout, {~0u, 0u});	

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);
    
    if (modelLayout.lines.size() > 0)
        append(modelLayout, {~0u, 0u}, 10);
        
    append(accuracyLayout, {~0u, 0u});
    
    modelLayout.setEvents();
    modelLayout.updateWidgets();
		
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
            
            ident = block->mediaGroup->name;
            
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
			
        } else if (block->mediaGroup->isTape()) {
			
			ident = block->mediaGroup->name;
			
			driveLayout.tapeWobble.onToggle = [this, ident]() {
                
                auto checked = driveLayout.tapeWobble.checked();              

                settings->set<bool>(this->tabWindow->ident(ident + "_wobble"), checked);
            };
									
			driveLayout.tapeWobble.setChecked( settings->get<bool>(this->tabWindow->ident(ident + "_wobble"), false ) );
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
    
    accuracyLayout.block.videoCycleAccuracy.setChecked( settings->get<bool>(this->tabWindow->ident("video_cycle_accuracy"), true) );
    
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

auto SystemLayout::translate() -> void {
    memoryLayout.setText( trans->get("memory") );
    driveLayout.setText( trans->get("drives") );
    
    modelLayout.translate();
    
    expansionLayout.setText( trans->get("expansion_port") );
	
    for(auto block : driveLayout.driveCountFrame.driveCounter) {

        auto ident = block->mediaGroup->name + "_drives";
        block->name.setText(trans->get(ident,{}, true));
        
        if (block->mediaGroup->isDisk() && dynamic_cast<LIBC64::Interface*>(emulator) )
            block->name.setTooltip(trans->get("cpu_warning_disk_info"));
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
	driveLayout.tapeWobble.setText( trans->get("Datasette Motor Variation") );
    
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

auto SystemLayout::setEnabled(bool state) -> void {
    upperLayout.setEnabled( state );
    
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

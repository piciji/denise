
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

SystemLayout::SystemLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    memorySliderReset.setInterval(500);
    
    memorySliderReset.onFinished = [this]() {
        if (activeEmulator)
            program->power(activeEmulator);
        
        memorySliderReset.setEnabled(false);
    };
    
    memoryLayout.build( emulator );
    modelLayout.build( tabWindow, emulator,
    {Emulator::Interface::Model::Purpose::Cpu, Emulator::Interface::Model::Purpose::GraphicChip, Emulator::Interface::Model::Purpose::SoundChip,
    Emulator::Interface::Model::Purpose::Cia, Emulator::Interface::Model::Purpose::Misc}, { 3, 3, 3 } );

    driveModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::DriveSettings}, { 2, 1, 1, 2 } );

    expansionLayout.build( emulator );

    setMargin(10);
    
    leftLayout.append(expansionLayout, {~0u, 0u}, 10);
    leftLayout.append(memoryLayout, {~0u, 0u});
    
    upperLayout.append(leftLayout, {~0u, 0u}, 10);
    rightLayout.append(driveModelLayout, {~0u, 0u});

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);
    
    if (modelLayout.lines.size() > 0)
        append(modelLayout, {~0u, 0u}, 10);
        
    append(accuracyLayout, {~0u, 0u});
    
    modelLayout.setEvents();
    driveModelLayout.setEvents();
		
    for( auto block : memoryLayout.blocks ) {
        auto memoryType = block->memoryType;
        
        block->sliderLayout.slider.onChange = [this, block, memoryType]() {
			unsigned pos = block->sliderLayout.slider.position();
			if (pos >= memoryType->memory.size())
				return;
			
            _settings->set<unsigned>( _underscore(memoryType->name) + "_mem", pos);
            block->sliderLayout.value.setText( getSizeString( memoryType->memory[pos].size ) );

			if (activeEmulator)                
                memorySliderReset.setEnabled();				            
        };
    }
               
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {            
            block->box.onActivate = [this, block]() {

                _settings->set<unsigned>( "expansion", block->expansion->id);
                updateExpansionMemory();

				if (activeEmulator)
					program->power(activeEmulator);
            };
        }
    }       
    
    accuracyLayout.block.videoCycleAccuracy.onToggle = [this]() {

        bool state = accuracyLayout.block.videoCycleAccuracy.checked();
        
        _settings->set<bool>("video_cycle_accuracy", state);

        program->fastForward(false);

        emulator->videoCycleAccuracy(state);
        
        if (this->emulator == activeEmulator)
            program->power(activeEmulator);
    };
    
    accuracyLayout.block.videoScanlineThread.onToggle = [this]() {

        bool state = accuracyLayout.block.videoScanlineThread.checked();
        
        _settings->set<bool>("video_scanline_thread", state);

        program->fastForward(false);

        emulator->videoScanlineThread(state);
    };    
    
    accuracyLayout.block.diskHighLoadThread.onToggle = [this]() {

        bool state = accuracyLayout.block.diskHighLoadThread.checked();
        
        _settings->set<bool>("disk_highload_thread", state);

        program->fastForward(false);

        emulator->diskHighLoadThread(state);
    };  

    accuracyLayout.block.diskIdle.onToggle = [this]() {

        bool state = accuracyLayout.block.diskIdle.checked();
        
        _settings->set<bool>("disk_idle", state);

        program->fastForward(false);

        emulator->diskIdle(state);
    };  
    
    accuracyLayout.block.audioRealtimeThread.onToggle = [this]() {

        bool state = accuracyLayout.block.audioRealtimeThread.checked();
        
        _settings->set<bool>("audio_realtime_thread", state);

        program->fastForward(false);

        emulator->audioRealtimeThread(state);
    }; 
    
    loadSettings();
}

auto SystemLayout::activateDrive( Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void {

    if (requestedCount > mediaGroup->media.size())
        requestedCount = mediaGroup->media.size();

    auto modelId = emulator->getModelIdOfEnabledDrives( mediaGroup );

    unsigned counter = emulator->getModelValue( modelId );

    if (counter >= requestedCount)
        return;

    driveModelLayout.updateWidget( modelId );
    tabWindow->mediaLayout->updateVisibility( mediaGroup, requestedCount );
}

auto SystemLayout::translate() -> void {
    memoryLayout.setText( trans->get("memory") );
    
    modelLayout.translate();
    driveModelLayout.translate( "drives" );
    
    expansionLayout.setText( trans->get("expansion_port") );
    
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

    driveModelLayout.alignSlider( "300.00 RPM" );
    
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
		
	std::vector<MemoryLayout::Block*> inUse;
    
	for( auto& expansion : emulator->expansions ) {

		if (!expansion.memoryType)
			continue;

		for (auto block : memoryLayout.blocks) {

			if (block->memoryType == expansion.memoryType) {
				
				if (&expansion == expansionSelected) {
					
					block->setEnabled( true );
					
					inUse.push_back( block );
					
				} else {
					
					if (!GUIKIT::Vector::find( inUse, block ))
						block->setEnabled( false );
				}								
				
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
						updateExpansionMemory();
					}
                    
                    return;
                }
            }
            
            else if (block->expansion == newExpansion) {
                if (!block->box.checked()) {
					block->box.setChecked();
					updateExpansionMemory();
				}
				
                return;
            }                
        }
    }
}

auto SystemLayout::loadSettings() -> void {

    for( auto block : memoryLayout.blocks ) {
        auto memoryType = block->memoryType;
        
        unsigned id = _settings->get<unsigned>(_underscore(memoryType->name) + "_mem", memoryType->defaultMemoryId);
        if (id >= memoryType->memory.size())
            id = memoryType->defaultMemoryId;
        
        block->sliderLayout.slider.setPosition(id);
        block->sliderLayout.value.setText(getSizeString(memoryType->memory[id].size)); 
    }
    
    auto expansionId = _settings->get<unsigned>( "expansion", 0);
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {
            if (block->expansion->id == expansionId)
                block->box.setChecked();
        }
    }
    
    accuracyLayout.block.videoCycleAccuracy.setChecked( _settings->get<bool>("video_cycle_accuracy", true) );
    
    accuracyLayout.block.videoScanlineThread.setChecked( _settings->get<bool>("video_scanline_thread", false) );
    
    accuracyLayout.block.diskHighLoadThread.setChecked( _settings->get<bool>("disk_highload_thread", false) );
    
    accuracyLayout.block.diskIdle.setChecked( _settings->get<bool>("disk_idle", false) );
    
    accuracyLayout.block.audioRealtimeThread.setChecked( _settings->get<bool>("audio_realtime_thread", false) );
    
    updateExpansionMemory();
    
    modelLayout.updateWidgets();

    driveModelLayout.updateWidgets();
}

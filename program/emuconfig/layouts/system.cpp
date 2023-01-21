
auto ExpansionLayout::build( Emulator::Interface* emulator ) -> void {
    unsigned blocksPerLine = 4;
    auto& expansions = emulator->expansions;

    if (expansions.size() <= 1)
        return;

    Line* line = nullptr;
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
    if (memoryTypes.empty()) return;

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
    std::vector<unsigned> dim;
    
    memorySliderReset.setInterval(500);
    
    memorySliderReset.onFinished = [this]() {
        if (activeEmulator) {
            emuThread->lock();
            program->power(activeEmulator);
            emuThread->unlock();
        }
        
        memorySliderReset.setEnabled(false);
    };

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = { 3, 3, 3 };
    else
        dim = { 1, 1, };

    memoryLayout.build( emulator );
    modelLayout.build( tabWindow, emulator,
    {Emulator::Interface::Model::Purpose::Cpu, Emulator::Interface::Model::Purpose::GraphicChip, Emulator::Interface::Model::Purpose::SoundChip,
    Emulator::Interface::Model::Purpose::Cia, Emulator::Interface::Model::Purpose::SubModels, Emulator::Interface::Model::Purpose::Misc}, dim );

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = { 2, 1, 1, 1, 3, 2, 2, 1, 2 };
    else
        dim = { 1, 1, 1, 1, 1 };

    memoryNewModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::Memory}, { 1, 1 } );
    driveModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::DriveSettings}, dim );
    performanceModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::Performance}, { 3 } );

    expansionLayout.build( emulator );

    setMargin(10);

    if (!expansionLayout.lines.empty())
        leftLayout.append(expansionLayout, {~0u, 0u}, 10);

    if (memoryNewModelLayout.hasElements())
        leftLayout.append(memoryNewModelLayout, {~0u, 0u}, 10);
    else if (!memoryLayout.blocks.empty())
        leftLayout.append(memoryLayout, {~0u, 0u});
    
    upperLayout.append(leftLayout, {~0u, 0u}, 10);

    if (driveModelLayout.hasElements())
        rightLayout.append(driveModelLayout, {~0u, 0u}, 10);

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);
    
    if (modelLayout.hasElements())
        append(modelLayout, {~0u, 0u}, 10);

    if (performanceModelLayout.hasElements())
        append(performanceModelLayout, {~0u, 0u});

    modelLayout.setEvents();
    memoryNewModelLayout.setEvents();
    driveModelLayout.setEvents();
    performanceModelLayout.setEvents();
		
    for( auto block : memoryLayout.blocks ) {
        auto memoryType = block->memoryType;
        
        block->sliderLayout.slider.onChange = [this, block, memoryType](unsigned position) {
			if (position >= memoryType->memory.size())
				return;
			
            _settings->set<unsigned>( _underscore(memoryType->name) + "_mem", position);
            block->sliderLayout.value.setText( getSizeString( memoryType->memory[position].size ) );

			if (activeEmulator)                
                memorySliderReset.setEnabled();				            
        };
    }
               
    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {            
            block->box.onActivate = [this, block]() {

                _settings->set<unsigned>( "expansion", block->expansion->id);
                updateExpansionMemory();

				if (activeEmulator) {
                    emuThread->lock();
                    program->power(activeEmulator);
                    emuThread->unlock();
                }
            };
        }
    }       

    loadSettings();
}

auto SystemLayout::translate() -> void {
    memoryLayout.setText( trans->get("memory") );
    
    modelLayout.translate();
    driveModelLayout.translate( "drives" );
    performanceModelLayout.translate( "accuracy and performance" );
    memoryNewModelLayout.translate( "memory" );
    
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

    updateExpansionMemory();
    
    modelLayout.updateWidgets();

    driveModelLayout.updateWidgets();

    performanceModelLayout.updateWidgets();

    memoryNewModelLayout.updateWidgets();
}

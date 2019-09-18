
FeatureLayout::Line::Block::Block(bool switched) {
	
	if (switched) {
		append(checkBox, {0u, 0u} );	
	} else {
		append(label, {0u, 0u}, 5 );
		append(lineEdit, {42u, 0u} );
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

auto FeatureLayout::build( Emulator::Interface* emulator ) -> void {
	auto& features = emulator->features;
	    
    Line* line;
    unsigned i = 0;
    unsigned lineCount = (features.size() / 4);
    lineCount += ((features.size() % 4) != 0) ? 1 : 0;
    
    for( auto& feature : features ) {
        
        if ((i++ % 4) == 0) {
            line = new Line();            
            lines.push_back( line );
            append( *line, {~0u, 0u}, ( lines.size() < lineCount ) ? 10 : 0 );
        }
        
        auto block = new Line::Block( feature.isSwitch() );
        line->blocks.push_back( block );
        block->typeId = feature.id;
        line->append(*block, {0u, 0u}, ((i % 4) == 0) ? 0 : 15);

        if (feature.performanceHit)
            block->append(block->dangerLabel, {0u, 0u} );
        
        block->checkBox.setText( feature.name );
        block->label.setText( feature.name );  
                
        block->dangerLabel.setForegroundColor(0xff4500);
    }
}

MemoryLayout::Block::Block(bool disable) {
    append(name, {60, 0u});
    append(value, {50, 0u});
    append(slider, {~0u, 0u});
    setAlignment(0.5);
    
    if (disable)
        slider.setEnabled(false);
}

MemoryLayout::MemoryLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

auto MemoryLayout::build( Emulator::Interface* emulator ) -> void {
    auto& memoryTypes = emulator->memoryTypes;
    for(auto& memoryType : memoryTypes ) {
                
        auto block = new Block( memoryType.memory.size() == 1 );
        blocks.push_back( block );
        block->typeId = memoryType.id;
        append(*block, {~0u, 0u}, &memoryType != &memoryTypes.back() ? 7 : 0);
        block->slider.setLength( memoryType.memory.size() );
        block->name.setText( memoryType.name + ":" );
    }   
}

DriveLayout::DriveCount::DriveCount() {
    append(name, {0u, 0u}, 5);
    append(combo, {0u, 0u});
    setAlignment(0.5);
}

DriveLayout::DriveLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

auto DriveLayout::build( Emulator::Interface* emulator ) -> void {
    for(auto& driveGroup : emulator->driveGroups) {
        if (driveGroup.isModuleSlot() || driveGroup.isMemory() )
            continue;
        
        auto driveCount = new DriveCount;
        driveCount->typeId = driveGroup.id;

        for(unsigned i = 0; i <= driveGroup.drives.size(); i++) {
            driveCount->combo.append( std::to_string(i) );
        }
        append(*driveCount, {0u,0u}, 15u);
        driveCounter.push_back(driveCount);
        
        if (driveGroup.isDiskDrive() && dynamic_cast<LIBC64::Interface*>(emulator) )
            driveCount->name.setForegroundColor(0xff4500);
    }
}

CpuLayout::Turbo::Turbo() {
	append(title, {0u, 0u}, 7u);
	append(slider, {~0u, 0u}, 7u);	
	append(label, {35u, 0u});	
	setAlignment( 0.5 );
	slider.setLength(201);
}

CpuLayout::CpuLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));	
}

auto CpuLayout::build( Emulator::Interface* emulator ) -> void {
    auto& cpus = emulator->cpus;
    for(auto& cpu : cpus) {
        auto radio = new GUIKIT::RadioBox;
        selector.radios.push_back( radio );
        radio->setText( cpu.name );
        selector.append(*radio, {0u, 0u}, &cpu != &cpus.back() ? 15 : 0);
    }
    GUIKIT::RadioBox::setGroup( selector.radios );	
    
    append(selector, {~0u, 0u}, emulator->turboSupported() ? 5u : 0u);
    
    if ( emulator->turboSupported() )  
        append(turbo, {~0u, 0u});
}

ChipsetLayout::ChipsetLayout() {
	setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    append(selector, {~0u, 0u}, 5u);
}

auto ChipsetLayout::build( Emulator::Interface* emulator ) -> void {
	
	auto& chipsets = emulator->chipsets;
	
    for(auto& chipset : chipsets ) {
        auto radio = new GUIKIT::RadioBox;
        selector.radios.push_back( radio );
        radio->setText( chipset.name );
        selector.append(*radio, {0u, 0u}, &chipset != &chipsets.back() ? 15 : 0);
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

    setMargin(10);

    leftLayout.append(memoryLayout, {~0u, 0u}, 10);
    
    upperLayout.append(leftLayout, {~0u, 0u}, 10);
    rightLayout.append(driveLayout, {~0u, 0u}, 10);
	
	bottomLayout.append(cpuLayout, {~0u, 0u}, 10);
	bottomLayout.append(chipsetLayout, {0u, 0u});
	
    rightLayout.append(bottomLayout, {~0u, 0u});

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);
    
    if (featureLayout.lines.size() > 0)
        append(featureLayout, {~0u, 0u});

    unsigned i = 0;
    for ( auto radio : cpuLayout.selector.radios ) {
        radio->onActivate = [this, i, radio]() {
            settings->set<unsigned>( this->tabWindow->ident("cpu"), i );
			cpuLayout.turbo.slider.setPosition(0);
			cpuLayout.turbo.slider.onChange();
        };
        i++;
    }
	
    cpuLayout.selector.radios[0]->setChecked();
    for(auto& cpu : emulator->cpus) {
        if (cpu.id == settings->get<unsigned>( tabWindow->ident("cpu"), 0)) {
            cpuLayout.selector.radios[cpu.id]->setChecked();
        }
    }
	
	cpuLayout.turbo.slider.onChange = [this]() {
        unsigned pos = cpuLayout.turbo.slider.position();
		settings->set<unsigned>( this->tabWindow->ident("cpu_turbo"), pos);
		cpuLayout.turbo.label.setText( std::to_string( pos ) + " %");
	};
	
	cpuLayout.turbo.label.setText( settings->get<std::string>( tabWindow->ident("cpu_turbo"), "0") + " %" );
	cpuLayout.turbo.slider.setPosition( settings->get<unsigned>( tabWindow->ident("cpu_turbo"), 0) );

    for( auto block : memoryLayout.blocks ) {
        auto& memoryType = emulator->memoryTypes[ block->typeId ];
        
        block->slider.onChange = [this, block, memoryType]() {
            unsigned id = block->slider.position();
            if (id >= memoryType.memory.size() ) return;
            settings->set<unsigned>( this->tabWindow->ident(memoryType.name + "_mem"), id);
            block->value.setText( memoryType.memory[id].name );
        };

        unsigned id = settings->get<unsigned>(tabWindow->ident(memoryType.name + "_mem"), memoryType.defaultMemoryId);
        if (id >= memoryType.memory.size()) id = memoryType.defaultMemoryId;
        block->slider.setPosition(id);
        block->value.setText(memoryType.memory[id].name);        
    }
    
    for(auto block : driveLayout.driveCounter) {
        
        auto& driveGroup = emulator->driveGroups[ block->typeId ];
        auto ident = driveGroup.name + "_count";
        
        block->combo.onChange = [this, ident, block]() {
            settings->set<unsigned>( this->tabWindow->ident(ident), block->combo.selection());
            auto& driveGroup = emulator->driveGroups[ block->typeId ];
            this->tabWindow->drivesLayout->updateVisibility( &driveGroup, block->combo.selection() );
            settings->remove( this->tabWindow->ident("access_floppy") );
        };
        
        unsigned counter = settings->get<unsigned>( tabWindow->ident(ident), driveGroup.defaultUsage());
        if (counter >= block->combo.rows())
            counter = driveGroup.defaultUsage();
        
        block->combo.setSelection( counter );
    }
    
	for( auto line : featureLayout.lines ) {
        
        for( auto block : line->blocks ) {
            
            auto& feature = emulator->features[ block->typeId ];

            if (feature.isSwitch() ) {	

                block->checkBox.onToggle = [this, block, feature]( ) {

                    settings->set<bool>( this->tabWindow->ident( feature.name ), block->checkBox.checked( ) );

                    emulator->setFeature( feature.id, block->checkBox.checked( ) );
                };

            } else {

                block->lineEdit.onChange = [this, block]() {

                    auto& feature = emulator->features[ block->typeId ];

                    int val;
                    auto str = block->lineEdit.text();

                    if ( feature.isHex() ) {                    
                        val = GUIKIT::String::convertHexToInt( str, feature.defaultValue );
                    } else
                        val = block->lineEdit.value();

                    auto range = feature.range;

                    if (val < range[0])
                        val = range[0];

                    if (val > range[1])
                        val = range[1];

                    settings->set<int>( this->tabWindow->ident( feature.name ), val );

                    emulator->setFeature( feature.id, val );
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
}

auto SystemLayout::activateDrive( Emulator::Interface::DriveGroup& driveGroup, unsigned requestedCount ) -> void {

    if (requestedCount > driveGroup.drives.size())
        requestedCount = driveGroup.drives.size();
    
    for (auto block : driveLayout.driveCounter) {

        auto& group = emulator->driveGroups[ block->typeId ];
        
        if (&driveGroup != &group)
            continue;
        
        auto ident = driveGroup.name + "_count";
        
        unsigned counter = settings->get<unsigned>( tabWindow->ident(ident), driveGroup.defaultUsage());
        
        if (counter >= requestedCount)
            break;
        
        block->combo.setSelection( requestedCount );
        settings->set<unsigned>( this->tabWindow->ident(ident), requestedCount);
        settings->remove( this->tabWindow->ident("access_floppy") );
        
        this->tabWindow->drivesLayout->updateVisibility( &driveGroup, requestedCount );
    }
}

auto SystemLayout::toggleFeature(unsigned id) -> bool {
	for(auto line : featureLayout.lines) {
        for( auto block : line->blocks ) {            
            if (block->typeId == id) {
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
            
            auto& feature = emulator->features[ block->typeId ];
            
            if (!feature.runtimeChangeable)
                continue;
            
            updateFeatureWidget( block );
        }
    }
}

auto SystemLayout::updateFeatureWidget( FeatureLayout::Line::Block* block ) -> void {	
	auto& feature = emulator->features[ block->typeId ];
	
	if (feature.isSwitch() ) {
		block->checkBox.setChecked( settings->get<bool>( tabWindow->ident( feature.name ), feature.defaultValue ) );
		return;
	}
	
	auto _val = settings->get<int>( tabWindow->ident( feature.name ), feature.defaultValue, feature.range );

	if ( feature.isHex() )                 
		block->lineEdit.setText( GUIKIT::String::convertIntToHex( _val ) );
	else            
		block->lineEdit.setValue( _val );	
}

auto SystemLayout::updateFeature(unsigned id, int step) -> int {
    for(auto line : featureLayout.lines) {
        for( auto block : line->blocks ) {                        
            if (block->typeId == id) {
                auto& feature = emulator->features[ block->typeId ];
                auto newValue = settings->get<int>( tabWindow->ident( feature.name ), feature.defaultValue, feature.range );
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
    
    for( auto& radio : cpuLayout.selector.radios ) {
        if (emulator->turboSupported())
            radio->setTooltip( trans->get( radio->text() + "_speed") );
    }
    
	cpuLayout.turbo.title.setText("Turbo:");
	chipsetLayout.setText("Chipset");
    featureLayout.setText( trans->get("feature") );

    for(auto block : driveLayout.driveCounter) {
        auto& driveGroup = emulator->driveGroups[ block->typeId ];
        auto ident = driveGroup.name + "_drives";
        block->name.setText(trans->get(ident,{}, true));
        
        if (driveGroup.isDiskDrive() && dynamic_cast<LIBC64::Interface*>(emulator) )
            block->name.setTooltip(trans->get("cpu_warning_disk_info"));
    }        
	
    for( auto line : featureLayout.lines ) {
        for( auto block : line->blocks ) {                               
            auto& feature = emulator->features[ block->typeId ];

            if (feature.isSwitch() )
                block->checkBox.setTooltip( trans->get( featureIdent( feature.name ) + "_info" ) );
            else
                block->label.setTooltip( trans->get( featureIdent( feature.name ) + "_info" ) );

            block->dangerLabel.setTooltip( trans->get("cpu_warning_info") );  
            block->dangerLabel.setText( " [" + trans->get("cpu_warning") + "]" );  
            
            block->checkBox.setText( trans->get( featureIdent( feature.name ) ) );
            block->label.setText( trans->get( featureIdent( feature.name ) ) );  
        }
	}
}

auto SystemLayout::featureIdent( std::string ident ) -> std::string {
    
    return GUIKIT::String::toLowerCase( GUIKIT::String::replace(ident, " ", "_") );
}

auto SystemLayout::setEnabled(bool state) -> void {
    upperLayout.setEnabled( state );
        
	// some features are changeable during emulation        
    for( auto line : featureLayout.lines ) {
        
        for( auto block : line->blocks ) {
            
            auto& feature = emulator->features[ block->typeId ];
            
            block->setEnabled( state ? true : feature.runtimeChangeable );
        }
    }    
}

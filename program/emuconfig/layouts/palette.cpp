
PaletteColorLayout::PaletteColorLayout(unsigned editWidth, unsigned canvasHeight) {   

    append( color, {~0u, 0u}, 5 );
    append( canvas, {(unsigned)((float)canvasHeight * 1.5), canvasHeight}, 10 );
    append( hex, {0u, 0u}, 1 );
    append( edit, {editWidth, 0u} );
    
    hex.setText( "0x" );
    hex.setForegroundColor( 0x333333 );
    color.setFont( GUIKIT::Font::system("bold") );
    canvas.setBorderColor( 1, 0x333333 );
    edit.setMaxLength(6);
    setAlignment( 0.5 );
}

PaletteControlLayout::PaletteControlLayout() {    
    append( title, {200u, 0u});
    append( spacer, {~0u, 0u} );
    append( ownPalette, {0u, 0u}, 10 );    
    append( create, {0u, 0u}, 10 );    
    append( remove, {0u, 0u} );
    setAlignment( 0.5 );
}

PaletteSaveLayout::PaletteSaveLayout() {
    append( spacer, {~0u, 0u} );
    append( title, {0u, 0u}, 10 );
    append( allChanges, {0u, 0u}, 30 );
    append( onExit, {0u, 0u} );
    
    onExit.setChecked( true );
    setAlignment( 0.5 );
}

PaletteDetailLayout::PaletteDetailLayout::Right::Right() : 
    r(""), g(""), b("")
{ }

PaletteDetailLayout::PaletteDetailLayout() {
        
    right.append( right.r, {~0u, 30u}, 10 );
    right.append( right.g, {~0u, 30u}, 10 );
    right.append( right.b, {~0u, 30u} );
    
    left.append( left.canvas, {100u, ~0u} );
    
    append(left, {0u, ~0u}, 10);
    append(right, {~0u, 0u});
    
    left.canvas.setBorderColor(1, 0x333333);
    
    setEnabled( false );
}

PaletteLayout::PaletteLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;     

    setMargin(10);
    updateList();
    PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
    GUIKIT::HorizontalLayout* colorLine = nullptr;
    
    detailLayout.right.r.slider.setLength(256);
    detailLayout.right.g.slider.setLength(256);
    detailLayout.right.b.slider.setLength(256);
    
    paletteLayout.append(controlLayout, {~0u, 0u}, 10);
    
    controlLayout.title.onChange = [this]() {
        
        auto& palette = this->getSelectedPalette();
        
        if (!palette.editable)
            return;
        
        palette.name = controlLayout.title.text();
        
        if (palette.name == "")
            palette.name = "???";
        
        listView.setText( listView.selection(), 0, palette.name );
    };
    
    GUIKIT::LineEdit test1;
    test1.setText( "bbbbbb" );
    auto editWidth = test1.minimumSize().width;
    
    GUIKIT::LineEdit test2;
    test2.setText("F");    
    auto canvasHeight = test2.minimumSize().height - 2;
    
    for(unsigned i = 0; i < paletteManager->getSize(); i++ ) {

        if ((i % 3) == 0) {
            if (colorLine) {
                colorLines.push_back(colorLine);
                paletteLayout.append(*colorLine, {~0u, 0u}, 10);
            }

            colorLine = new GUIKIT::HorizontalLayout;
        } 
        
        PaletteColorLayout* colorLayout = new PaletteColorLayout( editWidth, canvasHeight );
        
        colorLayout->pos = i;        
        
        colorLayout->edit.onChange = [this, colorLayout]() {

            auto& palette = this->getSelectedPalette();

            if (!palette.editable)
                return;
            
            colorPos = colorLayout->pos;
            
            std::string rgb = colorLayout->edit.text();                        
            
            palette.paletteColors[colorPos].rgb = GUIKIT::String::convertHexToInt( rgb, 0 );            
            palette.paletteColors[colorPos].updateChannels();

            program->setPalette( this->emulator );    
            
            if (!detailLayout.enabled())
                detailLayout.setEnabled();
            
            this->updateDetailLayout();
        };
		
		colorLayout->edit.onFocus = [this, colorLayout]() {
			auto& palette = this->getSelectedPalette();

			if (!palette.editable)
				return;

			colorPos = colorLayout->pos;
			markSelectedColor(colorLayout);			

			if (!detailLayout.enabled())
				detailLayout.setEnabled();

			this->updateDetailLayout();
		};
        
        colorLayout->canvas.onMouseRelease = [this, colorLayout](GUIKIT::Mouse::Button button) {

            auto& palette = this->getSelectedPalette();

            if (!palette.editable)
                return;

            colorPos = colorLayout->pos;
			markSelectedColor(colorLayout);

            if (!detailLayout.enabled())
                detailLayout.setEnabled();

            this->updateDetailLayout();
        };
        
        if ((i % 3) != 0)
            colorLayout->color.setAlign( GUIKIT::Label::Align::Right );
        else if (i == (paletteManager->getSize()-1))
            colorLayout->color.setAlign( GUIKIT::Label::Align::Right );
        
        colorLine->append( *colorLayout, {~0u, 0u} );
        
        colorLayouts.push_back( colorLayout );
    }  

    colorLines.push_back(colorLine);
    paletteLayout.append(*colorLine,{~0u, 0u}, 10);        
    paletteLayout.append(detailLayout, {~0u, 0u}, 10); 
    
    setPalette( getSelectedPalette() );
    
    main.append( listView, { GUIKIT::Font::scale( 180 ), paletteLayout.minimumSize().height - 10}, 10 );
    main.append( paletteLayout, {~0u, 0u} );  
    append(main, {~0u, 0u}, 10);    
    
    paletteLayout.append(saveLayout,{~0u, 0u} );
    
    listView.onChange = [this]() {
        
        auto& palette = getSelectedPalette();
        
        _settings->set<unsigned>( "palette", palette.id );
        
        program->setPalette( this->emulator ); 				
        
        this->setPalette( palette );                           
    };
    
    detailLayout.right.r.slider.onChange = [this]() {
        
		auto position = detailLayout.right.r.slider.position();
		
		detailLayout.right.r.value.setText( std::to_string(position) );
		
        updateSliderChange(position, 16);                
    };

    detailLayout.right.g.slider.onChange = [this]() {

		auto position = detailLayout.right.g.slider.position();
		
		detailLayout.right.g.value.setText( std::to_string(position) );

        updateSliderChange(position, 8);
    };

    detailLayout.right.b.slider.onChange = [this]() {

		auto position = detailLayout.right.b.slider.position();
		
		detailLayout.right.b.value.setText( std::to_string(position) );

        updateSliderChange( position, 0);
    };
    
    controlLayout.create.onActivate = [this]() {
        
        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
        
        auto& palette = paletteManager->add( getSelectedPalette() );
        
        _settings->set<unsigned>( "palette", palette.id );
        
        updateList();
        
        setPalette( palette );
    };
    
    controlLayout.remove.onActivate = [this]() {
        
        if ( !mes->question( trans->get("palette_remove_question") ) )
            return;
        
        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
        
        Emulator::Interface::Palette& palette = getSelectedPalette();
        
        if (!palette.editable)
            return;
        
        paletteManager->remove( palette );
        
        auto& _palette = emulator->palettes[0];
        
        _settings->set<unsigned>( "palette", _palette.id );
        
        updateList();
        
        setPalette( _palette );
        
        program->setPalette( this->emulator ); 
    };
    
    saveLayout.allChanges.onActivate = [this]() {
        
        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
        
        if ( !paletteManager->save() )
            mes->warning( trans->get("file_creation_error", {{"%path%", paletteManager->path()}} ));
        else
            status->addMessage( trans->get("file_creation_success", {{"%path%", paletteManager->path()}} ) );
    };
    
    saveLayout.onExit.onToggle = [this]() {
        
        _settings->set<bool>( "save_palettes_on_exit", saveLayout.onExit.checked(), false );
    };
}

auto PaletteLayout::updateDetailLayout() -> void {

    auto& palette = this->getSelectedPalette();

    if (!palette.editable)
        return;
    
    auto& pC = palette.paletteColors[colorPos];
    
    detailLayout.right.r.slider.setPosition( pC.r );
    detailLayout.right.g.slider.setPosition( pC.g );
    detailLayout.right.b.slider.setPosition( pC.b );
    
    detailLayout.right.r.value.setText( std::to_string( pC.r ) );
    detailLayout.right.g.value.setText( std::to_string( pC.g ) );
    detailLayout.right.b.value.setText( std::to_string( pC.b ) );
    
    detailLayout.left.canvas.setBackgroundColor( pC.rgb );
}

auto PaletteLayout::setPalette(Emulator::Interface::Palette& palette) -> void {
    
    auto& paletteColors = palette.paletteColors;
    
    controlLayout.title.setText( palette.name );
    
    controlLayout.title.setEnabled( palette.editable );
    
    controlLayout.remove.setEnabled( palette.editable );
    
    detailLayout.setEnabled( false );
	
	detailLayout.left.canvas.setBackgroundColor(0xffffff);
    
    for( auto colorLayout : colorLayouts ) {
        
        colorLayout->edit.setText( GUIKIT::String::prependZero( GUIKIT::String::convertIntToHex( paletteColors[colorLayout->pos].rgb, false ), 6) );                
        
        colorLayout->edit.setEnabled( palette.editable );
        
        colorLayout->canvas.setBackgroundColor( paletteColors[colorLayout->pos].rgb );
		
		colorLayout->color.setFont( GUIKIT::Font::system() );
    }
}

auto PaletteLayout::getSelectedPalette() -> Emulator::Interface::Palette& {
    
    unsigned size = emulator->palettes.size();
    
    if (listView.selection() >= size)
        return emulator->palettes[0];
    
    return emulator->palettes[ listView.selection() ];
}

auto PaletteLayout::updateList() -> void {
    
    auto usedPaletteId = _settings->get<unsigned>( "palette", 0 );
    
    listView.reset();
    
    unsigned i = 0;
    unsigned selected = 0;
    
    for ( auto& palette : emulator->palettes ) {
        
        listView.append( {trans->get( palette.name )} );
        
        if (palette.id == usedPaletteId)
            selected = i;            
        
        i++;
    }
    
    listView.setSelection( selected );
    listView.setSelected();
}

auto PaletteLayout::updateSliderChange( uint8_t colorChannel, uint8_t bits ) -> void {

    auto& palette = getSelectedPalette();

    if (!palette.editable)
        return;

    unsigned rgb = palette.paletteColors[colorPos].rgb;

    rgb &= ~(0xff << bits);
    rgb |= (colorChannel & 0xff) << bits;

    palette.paletteColors[colorPos].rgb = rgb;
    palette.paletteColors[colorPos].updateChannels();

    program->setPalette(this->emulator);

    detailLayout.left.canvas.setBackgroundColor(rgb);

    colorLayouts[colorPos]->canvas.setBackgroundColor(rgb);

    colorLayouts[colorPos]->edit.setText( GUIKIT::String::prependZero( GUIKIT::String::convertIntToHex(rgb, false), 6 ) ); 
}

auto PaletteLayout::translate() -> void {
    
    PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
    
    for(auto colorLayout : colorLayouts) {
        
        colorLayout->color.setText( trans->get( paletteManager->getIdent( colorLayout->pos ) ) );
    }
    
    detailLayout.right.r.name.setText( trans->get("red", {}, true ));
    detailLayout.right.g.name.setText( trans->get("green", {}, true ));
    detailLayout.right.b.name.setText( trans->get("blue", {}, true ));
    
    controlLayout.ownPalette.setText( trans->get("own_palette", {}, true) );
    controlLayout.create.setText( trans->get("create") );
    controlLayout.remove.setText( trans->get("remove") );    
    
    saveLayout.title.setText( trans->get("save", {}, true) );
    saveLayout.allChanges.setText( trans->get("all_changes") );
    saveLayout.onExit.setText( trans->get("save_on_exit") );

    SliderLayout::scale({&detailLayout.right.r, &detailLayout.right.g, &detailLayout.right.b}, "255");
}

auto PaletteLayout::markSelectedColor( PaletteColorLayout* selectColorLayout ) -> void {
	
	for( auto colorLayout : colorLayouts ) {				
		
		if (selectColorLayout == colorLayout)
			colorLayout->color.setFont( GUIKIT::Font::system("bold") );
		else
			colorLayout->color.setFont( GUIKIT::Font::system() );
	}
}

auto PaletteLayout::loadSettings() -> void {

    auto usedPaletteId = _settings->get<unsigned>( "palette", 0 );
    
    unsigned selected = 0;
    unsigned i = 0;
    
    for ( auto& palette : emulator->palettes ) {
        
        if (palette.id == usedPaletteId)
            selected = i;            
        
        i++;
    }
    
    listView.setSelection( selected );
    listView.setSelected();
        
    auto& palette = getSelectedPalette();

    this->setPalette(palette); 
}


#include "../../program.h"
#include "../config.h"
#include "../../view/message.h"
#include "../../audio/manager.h"
#include "../../../emulation/libc64/interface.h"
#include "model.h"

namespace EmuConfigView {   
    
#define mes this->tabWindow->message    
    
ModelLayout::Line::Block::Block(Emulator::Interface::Model* model) {
    
	this->model = model;
    
	if (model->isSwitch()) {
		append(checkBox, {0u, 0u} );
		
	} else if (model->isRadio()) {		
		append(label, {0u, 0u}, 5 );
		
		for(auto& option : model->options) {
			auto radio = new GUIKIT::RadioBox;
			options.push_back( radio );
			append( *radio, {0u, 0u}, &model->options.back() == &option ? 0 : 5 );	
		}
		GUIKIT::RadioBox::setGroup( options );
		
	} else if (model->isCombo()) {		
		append(label, {0u, 0u}, 5 );
		
		int i = 0;
		for(auto& option : model->options)			
			combo.append( option, i++ );		
		
		append( combo, {0u, 0u} );	

    } else if (model->isSlider()) {
        append(label, {0u, 0u}, 5 );
        append(slider, {400u, 0u});
        slider.updateValueWidth( std::to_string( model->range[0] < 0 ? model->range[0] : model->range[1] ) );        
        
        slider.slider.setLength( model->steps + 1 );
        
	} else {
        GUIKIT::LineEdit tester;
        tester.setText( model->isHex() ? "0xAA" : std::to_string(model->range[0]) );
		append(label, {0u, 0u}, 5 );
		append(lineEdit, {tester.minimumSize().width, 0u} );
	}
        
	setAlignment(0.5);
}

ModelLayout::Line::Line() {
    setAlignment(0.5);
}

ModelLayout::ModelLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));    
}

auto ModelLayout::build( TabWindow* tabWindow, Emulator::Interface* emulator, std::vector<Emulator::Interface::Model::Purpose> purposes, std::vector<unsigned> dim ) -> void {
    
    this->tabWindow = tabWindow;
    this->emulator = emulator;
    this->purposes = purposes;    
        
	auto& models = emulator->models;
	    
    Line* line = nullptr;
    unsigned linePos = 0;
	unsigned blockCount = 0;
    unsigned blockPos = 0;
  
    for( auto& model : models ) {
        
        if (!GUIKIT::Vector::find(purposes, model.purpose))
            continue;
        
        if (!line || (blockCount == blockPos) ) {
            
            if (dim.size() > linePos) {
                blockCount = dim[ linePos++ ];
                blockPos = 0;
                line = new Line();
                lines.push_back( line );
                append(*line, {~0u, 0u}, 5);     
                
                if (custom && lines.size() == 4 )
                    appendControlLayout();
            }
        }
        
        auto block = new Line::Block(&model);
        line->blocks.push_back(block);
        line->append(*block,{0u, 0u}, 15);
        
        blockPos++;
    }    
	
	update( *line, 0 );
}

auto ModelLayout::setEvents( ) -> void {
    
    for( auto line : lines ) {
        
        for( auto block : line->blocks ) {

            auto model = block->model;
            
            if (!GUIKIT::Vector::find(purposes, model->purpose))
                continue;
            
            if (model->isSwitch() ) {	

                block->checkBox.onToggle = [this, block, model]( ) {

                    settings->set<bool>( this->tabWindow->ident( model->name ), block->checkBox.checked( ) );

                    emulator->setModel( model->id, block->checkBox.checked( ) );
                                        
                    applyCustomStuff( model );
                };

			} else if (model->isRadio() ) {	
				unsigned val = 0;
				for( auto option : block->options ) {
					
					option->onActivate = [this, block, model, val]() {

						if (model->isGraphicChip()) {
							if (this->emulator == activeEmulator) {
								if (!mes->question(trans->get("setting change need reset"))) {
									unsigned oldValue = settings->get<int>( this->tabWindow->ident( model->name ), model->defaultValue, model->range );
									
									if (oldValue < block->options.size())
										block->options[oldValue]->setChecked( );		
									
									return;
								}
							}  
						}
						
						settings->set<unsigned>(this->tabWindow->ident(model->name), val);
						
						emulator->setModel( model->id, val );
						
                        applyCustomStuff( model );                                           
					};
					val++;
				}

			} else if (model->isCombo() ) {	
									
				block->combo.onChange = [this, block, model]() {

					if (model->isGraphicChip()) {
						if (this->emulator == activeEmulator) {
							if (!mes->question(trans->get("setting change need reset"))) {
								unsigned oldValue = settings->get<int>( this->tabWindow->ident( model->name ), model->defaultValue, model->range );

								if (oldValue < block->combo.rows())
									block->combo.setSelection(oldValue);

								return;
							}
						}  
					}

					int val = block->combo.userData();
					
					settings->set<unsigned>(this->tabWindow->ident(model->name), val);

					emulator->setModel( model->id, val );
                    
                    applyCustomStuff( model );
				};
					
            } else if (model->isSlider() ) {	
                
                block->slider.slider.onChange = [this, block, model]() {
                    
                    unsigned pos = block->slider.slider.position();
                    
                    int _min = model->range[0];
                    
                    int _max = model->range[1];
                    
                    int range = _max - _min;
                    
                    int stepSize = range / model->steps;
                    
                    int val = pos * stepSize + _min;
                    
                    settings->set<int>( this->tabWindow->ident( model->name ), val );
                    
                    block->slider.value.setText( std::to_string(val) );
                    
                    emulator->setModel( model->id, val );  
                    
                    applyCustomStuff( model );
                };
                
            } else {

                block->lineEdit.onChange = [this, block, model]() {
                                        
                    int val;
                    auto str = block->lineEdit.text();

                    if ( model->isHex() ) {                    
                        val = GUIKIT::String::convertHexToInt( str, model->defaultValue );
                    } else
                        val = block->lineEdit.value();

                    auto range = model->range;

                    if (val < range[0])
                        val = range[0];

                    if (val > range[1])
                        val = range[1];

                    settings->set<int>( this->tabWindow->ident( model->name ), val );

                    emulator->setModel( model->id, val );
                    
                    applyCustomStuff( model );
                };			
            }            
        }
        
	}
}

auto ModelLayout::updateWidgets( ) -> void {
    
    for (auto line : lines) {
        for (auto block : line->blocks)                        
            updateWidget( block );        
    }
    
    if (custom) {
        hideExtraAudioChips();
        hideBias();
    }
}

auto ModelLayout::updateWidget( unsigned id ) -> void {
    
    for(auto line : lines) {
        for( auto block : line->blocks ) {            
            if (block->model->id == id) {
                updateWidget( block );
                return;
            }
        }
    }
}

auto ModelLayout::updateWidget( Line::Block* block ) -> void {	
	auto model = block->model;
    
    if (!GUIKIT::Vector::find(purposes, model->purpose))
        return;
	
	if (model->isSwitch() ) {
		block->checkBox.setChecked( settings->get<bool>( tabWindow->ident( model->name ), model->defaultValue ) );
		return;
	}
    
    if (model->isSlider() ) {	
        
        auto val = settings->get<int>( tabWindow->ident( model->name ), model->defaultValue, model->range );

        int _min = model->range[0];

        int _max = model->range[1];

        int range = _max - _min;

        int stepSize = range / model->steps;

        unsigned pos = (val - _min) / stepSize;
        
        block->slider.slider.setPosition( pos );
        
        block->slider.value.setText( std::to_string(val) );
        
        return;
    }
		
	if (model->isRadio() ) {
		auto usedVal = settings->get<int>( tabWindow->ident( model->name ), model->defaultValue, model->range );
		
		unsigned val = 0;
		for(auto option : block->options) {
			if ( val++ == usedVal) {
				option->setChecked();
				break;
			}
		}
		
		return;
	}
	
	if (model->isCombo() ) {
		auto usedVal = settings->get<int>( tabWindow->ident( model->name ), model->defaultValue, model->range );
		block->combo.setSelection( usedVal );		
		return;
	}
	
	auto _val = settings->get<int>( tabWindow->ident( model->name ), model->defaultValue, model->range );

	if ( model->isHex() )                 
		block->lineEdit.setText( GUIKIT::String::convertIntToHex( _val ) );
	else            
		block->lineEdit.setValue( _val );	
}

auto ModelLayout::toggleCheckbox(unsigned id) -> bool {
    
	for(auto line : lines) {
        for( auto block : line->blocks ) {            
            if (block->model->id == id) {
                
                if (!block->model->isSwitch())
                    continue;
                
                block->checkBox.toggle();
                
                return block->checkBox.checked();
            }
        }        
	}
	return false;
}

auto ModelLayout::nextOption(unsigned id) -> unsigned {
    
    for(auto line : lines) {
        for( auto block : line->blocks ) {            
            if (block->model->id == id) {
                unsigned val = 0;
                
                if (block->model->isRadio()) {                    
                    
                    for (auto option : block->options) {
                        val++;
                        
                        if (option->checked()) {
                            
                            if (val == block->options.size())
                                val = 0;
                            
                            block->options[val]->activate();
                            return val;
                        }                  
                    }
                }                
                else if (block->model->isCombo() ) {
                    val = block->combo.selection() + 1;
                    
                    if (val == block->combo.rows())
                        val = 0;
                    
                    block->combo.setSelection( val );	
                    block->combo.onChange();
                    return val;
                }
            }
        }        
	}
    return 0;
}

auto ModelLayout::stepRange(unsigned id, int step) -> int {
    
    for(auto line : lines) {
        for( auto block : line->blocks ) {
            auto model = block->model;
            
            if (model->id == id) {
                auto newValue = settings->get<int>( tabWindow->ident( model->name ), model->defaultValue, model->range );                                                
                newValue += step;

                int _min = model->range[0];
                int _max = model->range[1];
                int range = _max - _min;

                if (newValue > _max)
                    newValue = _max;
                
                else if (newValue < _min)
                    newValue = _min;
                
                if (model->isRange()) {
                    block->lineEdit.setValue( newValue );            
                    block->lineEdit.onChange();
                    return newValue;
                    
                } else if (model->isSlider()) {

                    int stepSize = range / model->steps;

                    unsigned pos = (newValue + _max) / stepSize;

                    block->slider.slider.setPosition(pos);
                    
                    block->slider.slider.onChange();
                    
                    return newValue;
                }
            }
        }
	}
	return 0;
}

auto ModelLayout::translate( ) -> void {
    
    setText( trans->get("model") );    
    
    controlLayout.label.setText( trans->get("all", {}, true) );
    controlLayout.firstAll.setText( "8580" );
    controlLayout.secondAll.setText( "6581" ); 
    
    for (auto line : lines) {
        for (auto block : line->blocks) {
            auto model = block->model;
            
            std::string tooltip;
            std::string name = getIdent( model, tooltip );

            if (model->isSwitch())
                block->checkBox.setTooltip(trans->get(tooltip));

            else if (model->isRadio()) {
                unsigned pos = 0;
                for (auto option : block->options) {
                    option->setText(trans->get(model->options[pos++]));
                }
                block->label.setTooltip(trans->get(tooltip));

            } else if (model->isCombo()) {

                unsigned pos = 0;
                for (auto option : model->options) {
                    block->combo.setText(pos++, trans->get(option));
                }

                block->label.setTooltip(trans->get(tooltip));

            } else
                block->label.setTooltip(trans->get(tooltip));

            block->checkBox.setText(trans->get( name ));
            block->label.setText(trans->get( name, {},
            model->isRadio() || model->isCombo() || model->isSlider() ));
        }
    }
}

auto ModelLayout::applyCustomStuff(Emulator::Interface::Model* model) -> void {
    
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        
        switch(model->id) {
            case LIBC64::Interface::ModelIdSidFilterType:
                hideBias();
                break;
            case LIBC64::Interface::ModelIdSidMulti:
                hideExtraAudioChips();
            case LIBC64::Interface::ModelIdSid1Left:
            case LIBC64::Interface::ModelIdSid1Right:
            case LIBC64::Interface::ModelIdSid2Left:
            case LIBC64::Interface::ModelIdSid2Right:
            case LIBC64::Interface::ModelIdSid3Left:
            case LIBC64::Interface::ModelIdSid3Right:
            case LIBC64::Interface::ModelIdSid4Left:
            case LIBC64::Interface::ModelIdSid4Right:
            case LIBC64::Interface::ModelIdSid5Left:
            case LIBC64::Interface::ModelIdSid5Right:
            case LIBC64::Interface::ModelIdSid6Left:
            case LIBC64::Interface::ModelIdSid6Right:
            case LIBC64::Interface::ModelIdSid7Left:
            case LIBC64::Interface::ModelIdSid7Right:
            case LIBC64::Interface::ModelIdSid8Left:
            case LIBC64::Interface::ModelIdSid8Right:
                if (activeEmulator)
                    audioManager->power();
                break;
                
            case LIBC64::Interface::ModelIdVicIIModel:
                tabWindow->videoLayout->updatePresets();

                if (activeEmulator)
                    program->power(activeEmulator);
                break;
                
            case LIBC64::Interface::ModelIdSid: {
                auto cfgView = EmuConfigView::TabWindow::getView(this->emulator);

                if (this == &cfgView->systemLayout->modelLayout)
                    cfgView->audioLayout->settingsLayout.updateWidget(LIBC64::Interface::ModelIdSid);
                else
                    cfgView->systemLayout->modelLayout.updateWidget(LIBC64::Interface::ModelIdSid);
                
                } break;
                
            case LIBC64::Interface::ModelIdSidSampleFetch:
                audioManager->setResampler(); 
                break;
        }
    }
}

auto ModelLayout::hideBias() -> void {
    
    int filter = emulator->getModel( LIBC64::Interface::ModelIdSidFilterType );

    bool showBias6581 = filter == 0 || filter == 1;
    bool showBias8580 = filter == 0;
    
    if (lines[1]->enabled() != showBias6581 )
        lines[1]->setEnabled( showBias6581 );

    if (lines[2]->enabled() != showBias8580 )
        lines[2]->setEnabled( showBias8580 );
}

auto ModelLayout::hideExtraAudioChips() -> void {
    
    static int activeSidsNow = -1;
    
    int activeSids = emulator->getModel( LIBC64::Interface::ModelIdSidMulti );
    
    if (controlLayout.firstAll.checked())
        controlLayout.firstAll.setChecked(false);
    if (controlLayout.secondAll.checked())
        controlLayout.secondAll.setChecked(false);
    
    
    if (activeSidsNow == activeSids)
        return;
    
    activeSidsNow = activeSids;
    
    for (unsigned i = 0; i < 8; i++) {
        
        if (i <= activeSids)
            lines[i + 4]->setEnabled(true);
        else
            lines[i + 4]->setEnabled(false);        
    }
    
    if (activeSids == 0) {
        controlLayout.setEnabled( false );
        lines[4]->blocks[1]->setEnabled(false);
        lines[4]->blocks[2]->setEnabled(false);
        lines[4]->blocks[3]->setEnabled(false);
    } else
        controlLayout.setEnabled( true );
}

auto ModelLayout::appendControlLayout() -> void {
    
    update( *lines[lines.size() - 1], 10 );    
    
    GUIKIT::Label test;
    test.setText("SID 8:");
    
    controlLayout.append(controlLayout.label, {test.minimumSize().width, 0u}, 5);
    controlLayout.append(controlLayout.firstAll, {0u, 0u}, 5);
    controlLayout.append(controlLayout.secondAll, {0u, 0u}, 20);
    
    append(controlLayout, {0u, 0u}, 5);
    
    controlLayout.firstAll.onToggle = [this]() {
        
        if (!controlLayout.firstAll.checked())
            return;
        
        Line::Block* block;
        block = getBlock( LIBC64::Interface::ModelIdSid );
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid2);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid3);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid4);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid5);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid6);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid7);
        if (block) block->options[0]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid8);
        if (block) block->options[0]->activate();
        
        controlLayout.secondAll.setChecked( false );
    };

    controlLayout.secondAll.onToggle = [this]() {

        if (!controlLayout.secondAll.checked())
            return;
        
        Line::Block* block;
        block = getBlock(LIBC64::Interface::ModelIdSid);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid2);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid3);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid4);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid5);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid6);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid7);
        if (block) block->options[1]->activate();
        block = getBlock(LIBC64::Interface::ModelIdSid8);
        if (block) block->options[1]->activate();
        
        controlLayout.firstAll.setChecked( false );
    };
}

auto ModelLayout::getBlock( unsigned modelId ) -> Line::Block* {
    
    for (auto line : lines) {
        for(auto block : line->blocks ) {
            
            if (block->model->id == modelId)
                return block;
        }
    }
    return nullptr;
}

auto ModelLayout::getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string {
    
    std::string name = model->name;
        
    switch(model->id) {
        case LIBC64::Interface::ModelIdSid:
            if (!custom)
                name = "SID";
        case LIBC64::Interface::ModelIdSid2:
        case LIBC64::Interface::ModelIdSid3:
        case LIBC64::Interface::ModelIdSid4:
        case LIBC64::Interface::ModelIdSid5:
        case LIBC64::Interface::ModelIdSid6:
        case LIBC64::Interface::ModelIdSid7:
        case LIBC64::Interface::ModelIdSid8:            
            tooltip = "SID tooltip";
            break;
            
        case LIBC64::Interface::ModelIdSid1Adr:
        case LIBC64::Interface::ModelIdSid2Adr:
        case LIBC64::Interface::ModelIdSid3Adr:
        case LIBC64::Interface::ModelIdSid4Adr:
        case LIBC64::Interface::ModelIdSid5Adr:
        case LIBC64::Interface::ModelIdSid6Adr:
        case LIBC64::Interface::ModelIdSid7Adr:
        case LIBC64::Interface::ModelIdSid8Adr:
            name = "Address";
            tooltip = name + " tooltip";
            break;
            
        case LIBC64::Interface::ModelIdSid1Left:
        case LIBC64::Interface::ModelIdSid2Left:
        case LIBC64::Interface::ModelIdSid3Left:
        case LIBC64::Interface::ModelIdSid4Left:
        case LIBC64::Interface::ModelIdSid5Left:
        case LIBC64::Interface::ModelIdSid6Left:
        case LIBC64::Interface::ModelIdSid7Left:
        case LIBC64::Interface::ModelIdSid8Left:
            name = "Left Channel";
            tooltip = name + " tooltip";
            break;
            
        case LIBC64::Interface::ModelIdSid1Right:
        case LIBC64::Interface::ModelIdSid2Right:
        case LIBC64::Interface::ModelIdSid3Right:
        case LIBC64::Interface::ModelIdSid4Right:
        case LIBC64::Interface::ModelIdSid5Right:
        case LIBC64::Interface::ModelIdSid6Right:
        case LIBC64::Interface::ModelIdSid7Right:
        case LIBC64::Interface::ModelIdSid8Right:
            name = "Right Channel";
            tooltip = name + " tooltip";
            break;
            
        default:
            tooltip = name + " tooltip";
            break;
    }        
    
    return name;
}

}

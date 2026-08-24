
#include "../../program.h"
#include "../config.h"
#include "../../audio/manager.h"
#include "../../thread/emuThread.h"
#include "model.h"
#include "../../config/slider.h"
#include "../../../data/icons.h"

namespace EmuConfigView {   
    
#define mes this->tabWindow->message

GUIKIT::Image* ModelLayout::backImg = nullptr;

ModelLayout::Line::Block::Block(Emulator::Interface::Model* model, ModelLayout* layout) {
    
	this->model = model;
    
	if (model->isSwitch()) {
        checkBox = new GUIKIT::CheckBox;
		append(*checkBox, {0u, 0u} );
		
	} else if (model->isRadio()) {
        label = new GUIKIT::Label;
		append(*label, {0u, 0u}, 7 );
		
		for(auto& option : model->options) {
			auto radio = new GUIKIT::RadioBox;
			options.push_back( radio );
			append( *radio, {0u, 0u}, &model->options.back() == &option ? 0 : 5 );	
		}
		GUIKIT::RadioBox::setGroup( options );
		
	} else if (model->isCombo()) {
        label = new GUIKIT::Label;
		append(*label, {0u, 0u}, 5 );
		
		int i = 0;
        combo = new GUIKIT::ComboButton(model->isAudioSettings());

	    std::vector<GUIKIT::ComboButton::Entry> rows;

		for(auto& option : model->options)
			rows.push_back({option, i++, "" });

	    combo->appendMulti( rows );
		
		append( *combo, {0u, 0u} );

    } else if (model->isSlider()) {
        auto& sOptions = model->options;
        sliderLayout = new ::SliderLayout("", false, !sOptions.size());
        if (!backImg) {
            backImg = new GUIKIT::Image;
            backImg->loadPng((uint8_t*)Icons::back, sizeof(Icons::back));
        }
        sliderLayout->defaultButton.setImage(backImg);
        append(*sliderLayout, {~0u, 0u});

        if (!sOptions.empty()) {
            GUIKIT::Label tester;
            int w = 0;
            std::string longest;
            for(auto& sOption : sOptions) {
                tester.setText(sOption);
                if (tester.minimumSize().width > w) {
                    w = tester.minimumSize().width;
                    longest = sOption;
                }
            }
            sliderLayout->updateValueWidth(longest);
            sliderLayout->slider.setLength( sOptions.size() );
        } else {
            std::string longest = std::to_string( model->range[0] < 0 ? model->range[0] : model->range[1] );
            if (model->scaler != 1.0)
                longest += ".0";
            longest += layout->getUnit(model);

            sliderLayout->updateValueWidth( longest );
            sliderLayout->slider.setLength( model->steps + 1 );
        }
	} else {
        GUIKIT::LineEdit tester;
        tester.setText( model->isHex() ? "0xAA" : std::to_string(model->range[0]) );
        label = new GUIKIT::Label;
        lineEdit = new GUIKIT::LineEdit;
		append(*label, {0u, 0u}, 5 );
		append(*lineEdit, {tester.minimumSize().width, 0u} );
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

auto ModelLayout::build( TabWindow* tabWindow, Emulator::Interface* emulator, std::vector<Emulator::Interface::Model::Purpose> purposes, std::vector<unsigned> dim, unsigned lineSpace ) -> void {
    
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
                lineWillAppend( lines.size() );
                line = new Line();
                lines.push_back( line );
                append(*line, {~0u, 0u}, lineSpace);
            }
        }

        if (!line)
            return;

        auto block = new Line::Block(&model, this);
        line->blocks.push_back(block);

        blockWillAppend(line, block);

        blockPos++;

        if (model.isSlider())
            line->append(*block,{~0u, 0u}, blockCount != blockPos ? 15 : 0);
        else
            line->append(*block,{0u, 0u}, blockCount != blockPos ? 15 : 0);
    }

    if (line)
	    update( *line, 0 );
}

auto ModelLayout::setEvents( ) -> void {
    
    for( auto line : lines ) {
        
        for( auto block : line->blocks ) {

            auto model = block->model;
            
            if (!GUIKIT::Vector::find(purposes, model->purpose))
                continue;
            
            if (model->isSwitch() ) {	

                block->checkBox->onToggle = [this, block, model]( bool checked ) {
                    tabWindow->settings->set<bool>( _underscore(model->name), checked );

                    bool locked = emuThread->lock(true);
                    emulator->setModelValue( model->id, checked );
                    updated( block, model );
                    if (locked) // nested (e.g. changing speeder)
                        emuThread->unlock();
                };

			} else if (model->isRadio() ) {
				unsigned val = 0;
				for( auto option : block->options ) {
					
					option->onActivate = [this, block, model, val]() {
						
						tabWindow->settings->set<unsigned>(_underscore(model->name), val);

                        bool locked = emuThread->lock(true);
						emulator->setModelValue( model->id, val );
					    updated( block, model );
                        if (locked)
                            emuThread->unlock();
					};
					val++;
				}

			} else if (model->isCombo() ) {	
									
				block->combo->onChange = [this, block, model]() {

					int val = block->combo->userData();
					
					tabWindow->settings->set<unsigned>( _underscore(model->name), val);

                    bool locked = emuThread->lock(true);
					emulator->setModelValue( model->id, val );
				    updated( block, model );
                    if (locked)
                        emuThread->unlock();
				};
					
            } else if (model->isSlider() ) {
                
                block->sliderLayout->slider.onChange = [this, block, model](unsigned position) {
                    int _min = model->range[0];
                    int _max = model->range[1];
                    int range = _max - _min;
                    auto& options = model->options;

                    int stepSize = 1;
                    int val = position;
                    std::string displayText;
                    std::string unit = getUnit(model);

                    if (!options.size()) {
                        stepSize = range / model->steps;
                        val = position * stepSize + _min;
                        displayText = std::to_string(val);
                        if (model->scaler != 1.0)
                            displayText = GUIKIT::String::formatFloatingPoint( (float)val / model->scaler, decimalPlaces(model->scaler));
                    } else {
                        if (position < options.size())
                            displayText = options[position];
                    }

                    tabWindow->settings->set<int>( _underscore(model->name), val );

                    block->sliderLayout->value.setText( displayText + unit );

                    bool locked = emuThread->lock(true);
                    emulator->setModelValue( model->id, val );
                    updated( block, model );
                    if (locked)
                        emuThread->unlock();
                };

                block->sliderLayout->defaultButton.onActivate = [this, block, model]() {
                    int defaultVal = model->defaultValue;
                    tabWindow->settings->set<int>( _underscore(model->name), defaultVal );
                    updateWidget(block);

                    bool locked = emuThread->lock(true);
                    emulator->setModelValue( model->id, defaultVal );
                    updated( block, model );
                    if (locked)
                        emuThread->unlock();
                };

            } else {

                block->lineEdit->onChange = [this, block, model]() {
                                        
                    int val;
                    auto str = block->lineEdit->text();

                    if ( model->isHex() ) {                    
                        val = GUIKIT::String::convertHexToInt( str, model->defaultValue );
                    } else
                        val = block->lineEdit->value();

                    auto range = model->range;

                    if (val < range[0])
                        val = range[0];

                    if (val > range[1])
                        val = range[1];

                    tabWindow->settings->set<int>( _underscore(model->name), val );

                    bool locked = emuThread->lock(true);
                    emulator->setModelValue( model->id, val );
                    updated( block, model );
                    if (locked)
                        emuThread->unlock();
                };			
            }            
        }
	}
}

auto ModelLayout::alignSlider( std::string maxText ) -> void {
    std::vector<SliderLayout*> sliderLayouts;

    for (auto line : lines) {
        for (auto block : line->blocks) {
            if (block->model->isSlider()) {
                sliderLayouts.push_back( (SliderLayout*)block->sliderLayout );
            }
        }
    }

    SliderLayout::scale(sliderLayouts, maxText);
}

auto ModelLayout::updateWidgets( ) -> void {
    for (auto line : lines) {
        for (auto block : line->blocks) {
            updateWidget(block);
        }
    }

    updateVisibillity();
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
		block->checkBox->setChecked( tabWindow->settings->get<bool>( _underscore(model->name), model->defaultValue ) );

	} else if (model->isSlider() ) {
        auto val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );

        int _min = model->range[0];
        int _max = model->range[1];
        int range = _max - _min;
        auto& options = model->options;

        int stepSize = 1;
        unsigned pos = val;
        std::string displayText;
        std::string unit = getUnit(model);

        if (options.empty()) {
            stepSize = range / model->steps;
            if (stepSize == 0)
                pos = 0;
            else
                pos = (val - _min) / stepSize;
            displayText = std::to_string(val);
            if (model->scaler != 1.0)
                displayText = GUIKIT::String::formatFloatingPoint( (float) val / model->scaler, decimalPlaces(model->scaler));

        } else {
            if (pos < options.size())
                displayText = options[pos];
        }

        block->sliderLayout->slider.setPosition( pos );

        block->sliderLayout->value.setText( displayText + unit );

    } else if (model->isRadio() ) {
		auto usedVal = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
		
		unsigned val = 0;
		for(auto option : block->options) {
			if ( val++ == usedVal) {
				option->setChecked();
				break;
			}
		}
	} else if (model->isCombo() ) {
		auto usedVal = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
		block->combo->setSelection( usedVal );

	} else {
	    auto _val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );

	    if ( model->isHex() )
	        block->lineEdit->setText( GUIKIT::String::convertIntToHex( _val ) );
	    else
	        block->lineEdit->setValue( _val );
	}

    widgetUpdated(block, model);
}

inline auto ModelLayout::decimalPlaces(float scaler) -> unsigned {
    int places = 0;
    float val = 1.0;
    while (val < scaler) {
        val *= 10.0f;
        places++;
    }
    return places;
}

auto ModelLayout::toggleCheckbox(unsigned id) -> bool {
    
	for(auto line : lines) {
        for( auto block : line->blocks ) {            
            if (block->model->id == id) {
                
                if (!block->model->isSwitch())
                    continue;
                
                block->checkBox->toggle();
                
                return block->checkBox->checked();
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
                    val = block->combo->selection() + 1;
                    
                    if (val == block->combo->rowCount())
                        val = 0;
                    
                    block->combo->setSelection( val );
                    block->combo->onChange();
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
                auto newValue = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );                                                
                newValue += step;

                int _min = model->range[0];
                int _max = model->range[1];
                int range = _max - _min;

                if (newValue > _max)
                    newValue = _max;
                
                else if (newValue < _min)
                    newValue = _min;
                
                if (model->isRange()) {
                    block->lineEdit->setValue( newValue );
                    block->lineEdit->onChange();
                    return newValue;
                    
                } else if (model->isSlider()) {
                    unsigned pos = newValue;

                    if (!model->options.size()) {
                        int stepSize = range / model->steps;
                        pos = (newValue - _min) / stepSize;
                    }

                    block->sliderLayout->slider.setPosition(pos);
                    
                    block->sliderLayout->slider.onChange(pos);
                    
                    return newValue;
                }
            }
        }
	}
	return 0;
}

auto ModelLayout::translate( std::string theme ) -> void {
    
    setText( trans->getA( theme ) );
    
    for (auto line : lines) {
        for (auto block : line->blocks) {
            auto model = block->model;
            
            std::string tooltip;
            std::string name = getIdent( model, tooltip );

            if (model->isSwitch()) {
                block->checkBox->setText(trans->getA( name ));
                block->checkBox->setTooltip(trans->getA(tooltip));

            } else if (model->isRadio()) {
                unsigned pos = 0;
                for (auto option : block->options) {
                    option->setText(trans->getA(model->options[pos++]));
                }

            } else if (model->isCombo()) {
                unsigned pos = 0;
                for (auto option : model->options) {
                    block->combo->setText(pos++, trans->getA(option));
                }

            } else if (model->isSlider()) {
                block->sliderLayout->name.setText(trans->getA( name, true ));
                block->sliderLayout->name.setTooltip(trans->getA(tooltip));
                if (block->sliderLayout->withButton) {
                    block->sliderLayout->defaultButton.setTooltip( trans->getA(trans->getA("default") ) );
                }
            }

            if (block->label) {
                block->label->setText(trans->getA(name, model->isRadio() || model->isCombo()));
                block->label->setTooltip(trans->getA(tooltip));
            }
        }
    }
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
    tooltip = model->name + " tooltip";
    return model->name;
}

}

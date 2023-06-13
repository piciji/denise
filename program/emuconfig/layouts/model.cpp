
// todo handle custom stuff better

#include "../../program.h"
#include "../config.h"
#include "../../view/message.h"
#include "../../audio/manager.h"
#include "../../../emulation/libc64/interface.h"
#include "../../media/media.h"
#include "../../thread/emuThread.h"
#include "model.h"

namespace EmuConfigView {   
    
#define mes this->tabWindow->message    
    
ModelLayout::Line::Block::Block(Emulator::Interface::Model* model, ModelLayout* layout) {
    
	this->model = model;
    
	if (model->isSwitch()) {
        checkBox = new GUIKIT::CheckBox;
		append(*checkBox, {0u, 0u} );
		
	} else if (model->isRadio()) {
        label = new GUIKIT::Label;
		append(*label, {layout->getAlignedWidth(model), 0u}, 5 );
		
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
        combo = new GUIKIT::ComboButton(model->isCombo() && model->isAudioSettings());
		for(auto& option : model->options)			
			combo->append( option, i++ );
		
		append( *combo, {0u, 0u} );

    } else if (model->isSlider()) {
        sliderLayout = new ::SliderLayout;
        append(*sliderLayout, {~0u, 0u});
        auto& sOptions = model->options;
        if (sOptions.size()) {
            GUIKIT::Label tester;
            int w = 0;
            std::string longest = "";
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
            sliderLayout->updateValueWidth( std::to_string( model->range[0] < 0 ? model->range[0] : model->range[1] ) );
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

auto ModelLayout::build( TabWindow* tabWindow, Emulator::Interface* emulator, std::vector<Emulator::Interface::Model::Purpose> purposes, std::vector<unsigned> dim ) -> void {
    
    this->tabWindow = tabWindow;
    this->emulator = emulator;
    this->purposes = purposes;    
        
	auto& models = emulator->models;
	    
    Line* line = nullptr;
    unsigned linePos = 0;
	unsigned blockCount = 0;
    unsigned blockPos = 0;

    bool useMultiAudioChipSelector = dynamic_cast<LIBC64::Interface*>(this->emulator) && GUIKIT::Vector::find(purposes, Emulator::Interface::Model::AudioSettings);
  
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
                
                if ( useMultiAudioChipSelector && (lines.size() == 4) )
                    appendAudioSelectorLayout();
            }
        }
        
        auto block = new Line::Block(&model, this);
        line->blocks.push_back(block);
        if (model.isSlider())
            line->append(*block,{~0u, 0u}, 15);
        else
            line->append(*block,{0u, 0u}, 15);
        
        blockPos++;
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

                    bool locked = emuThread->lock();
                    emulator->setModelValue( model->id, checked );
                    applyCustomStuff( block, model );
                    if (locked) // nested (e.g. changing speeder)
                        emuThread->unlock();
                };

			} else if (model->isRadio() ) {	
				unsigned val = 0;
				for( auto option : block->options ) {
					
					option->onActivate = [this, block, model, val]() {
						
						tabWindow->settings->set<unsigned>(_underscore(model->name), val);

                        bool locked = emuThread->lock();
						emulator->setModelValue( model->id, val );
                        applyCustomStuff( block, model );
                        if (locked)
                            emuThread->unlock();
					};
					val++;
				}

			} else if (model->isCombo() ) {	
									
				block->combo->onChange = [this, block, model]() {

					int val = block->combo->userData();
					
					tabWindow->settings->set<unsigned>( _underscore(model->name), val);

                    bool locked = emuThread->lock();
					emulator->setModelValue( model->id, val );
                    applyCustomStuff( block, model );
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
                    std::string displayText = "";
                    std::string unit = "";

                    if (!options.size()) {
                        stepSize = range / model->steps;
                        val = position * stepSize + _min;
                        displayText = std::to_string(val);
                        if (model->scaler != 1.0)
                            displayText = GUIKIT::String::formatFloatingPoint( (float)val / model->scaler, 2);
                    } else {
                        if (position < options.size())
                            displayText = options[position];
                    }

                    tabWindow->settings->set<int>( _underscore(model->name), val );

                    if (model->isDriveSettings())
                        unit = getUnit(model->id);

                    block->sliderLayout->value.setText( displayText + unit );

                    bool locked = emuThread->lock();
                    emulator->setModelValue( model->id, val );
                    applyCustomStuff( block, model );
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

                    bool locked = emuThread->lock();
                    emulator->setModelValue( model->id, val );
                    applyCustomStuff( block, model );
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

            if (tabWindow->mediaLayout && block->model->isDriveSettings() &&
            (emulator->getModelIdOfEnabledDrives(emulator->getDiskMediaGroup()) == block->model->id) ) {
                tabWindow->mediaLayout->updateVisibility( emulator->getDiskMediaGroup(), block->combo->selection() );
            }
        }
    }
    
    if (dynamic_cast<LIBC64::Interface*>(this->emulator) && GUIKIT::Vector::find( purposes, Emulator::Interface::Model::AudioSettings )) {
        updateExtraAudioChipsVisibillity();
        updateBiasVisibillity();
    } else if (dynamic_cast<LIBC64::Interface*>(this->emulator) && GUIKIT::Vector::find( purposes, Emulator::Interface::Model::DriveSettings )) {
        updateBurstVisibillity();
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
		block->checkBox->setChecked( tabWindow->settings->get<bool>( _underscore(model->name), model->defaultValue ) );
		return;
	}
    
    if (model->isSlider() ) {	
        
        auto val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );

        int _min = model->range[0];
        int _max = model->range[1];
        int range = _max - _min;
        auto& options = model->options;

        int stepSize = 1;
        unsigned pos = val;
        std::string displayText = "";
        std::string unit = "";

        if (!options.size()) {
            stepSize = range / model->steps;
            pos = (val - _min) / stepSize;
            displayText = std::to_string(val);
            if (model->scaler != 1.0)
                displayText = GUIKIT::String::formatFloatingPoint( (float) val / model->scaler, 2);

        } else {
            if (pos < options.size())
                displayText = options[pos];
        }

        block->sliderLayout->slider.setPosition( pos );

        if (model->isDriveSettings())
            unit = getUnit(model->id);

        block->sliderLayout->value.setText( displayText + unit );
        
        return;
    }
		
	if (model->isRadio() ) {
		auto usedVal = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
		
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
		auto usedVal = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
		block->combo->setSelection( usedVal );
		return;
	}
	
	auto _val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );

	if ( model->isHex() )                 
		block->lineEdit->setText( GUIKIT::String::convertIntToHex( _val ) );
	else            
		block->lineEdit->setValue( _val );
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
                    
                    if (val == block->combo->rows())
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
                    // todo apply model scaler

                    unsigned pos = newValue;

                    if (!model->options.size()) {
                        int stepSize = range / model->steps;
                        pos = (newValue + _max) / stepSize;
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

auto ModelLayout::setVisibility( Emulator::Interface::Model* model ) -> void {
    for (auto line : lines) {
        for (auto block : line->blocks)
            block->setEnabled( model && (block->model->id == model->id) );
    }
}

auto ModelLayout::translate( std::string theme ) -> void {
    
    setText( trans->getA( theme ) );

    if (GUIKIT::Vector::find(purposes, Emulator::Interface::Model::AudioSettings)) {
        controlLayout.label.setText(trans->getA("all"));
        controlLayout.firstAll.setText("8580");
        controlLayout.secondAll.setText("6581");
    }
    
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
            }

            if (block->label) {
                block->label->setText(trans->getA(name, model->isRadio() || model->isCombo()));
                block->label->setTooltip(trans->getA(tooltip));
            }
        }
    }
}

auto ModelLayout::applyCustomStuff( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        
        switch(model->id) {
            case LIBC64::Interface::ModelIdSidFilterType:
                updateBiasVisibillity();
                break;
            case LIBC64::Interface::ModelIdSidMulti:
                updateExtraAudioChipsVisibillity();
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
                if (tabWindow->videoLayout)
                    tabWindow->videoLayout->updatePresets();
                else if (videoDriver)
                    VideoManager::getInstance( emulator )->reloadSettings();

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;
                
            case LIBC64::Interface::ModelIdSid: {
                auto emuView = EmuConfigView::TabWindow::getView(this->emulator);

                if (!emuView->systemLayout)
                    break;

                if (this == &emuView->systemLayout->modelLayout) {
                    if (emuView->audioLayout)
                        emuView->audioLayout->settingsLayout.updateWidget(LIBC64::Interface::ModelIdSid);
                } else {
                    emuView->systemLayout->modelLayout.updateWidget(LIBC64::Interface::ModelIdSid);
                }
                
                } break;
                
            case LIBC64::Interface::ModelIdSidSampleFetch:
                audioManager->setResampler();
                break;

            case LIBC64::Interface::ModelIdDiskDrivesConnected:
                if(tabWindow->mediaLayout)
                    tabWindow->mediaLayout->updateVisibility( emulator->getDiskMediaGroup(), block->combo->selection() );
                // fall through
            case LIBC64::Interface::ModelIdTapeDrivesConnected:
            case LIBC64::Interface::ModelIdDriveRam20To3F:
            case LIBC64::Interface::ModelIdDriveRam40To5F:
            case LIBC64::Interface::ModelIdDriveRam60To7F:
            case LIBC64::Interface::ModelIdDriveRam80To9F:
            case LIBC64::Interface::ModelIdDriveRamA0ToBF:
            case LIBC64::Interface::ModelIdReuRam:
            case LIBC64::Interface::ModelIdGeoRam:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);

                break;

            case LIBC64::Interface::ModelIdDiskDriveModel:
                updateBurstVisibillity();
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;

            case  LIBC64::Interface::ModelIdDriveFastLoader:
                hintDriveSettings();
                break;

            case LIBC64::Interface::ModelIdCycleAccurateVideo:
                program->fastForward(false);

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;

            case LIBC64::Interface::ModelIdDiskThread:
            case LIBC64::Interface::ModelIdDiskOnDemand:
                program->fastForward(false);
                break;
        }
    } else {
        switch(model->id) {
            case LIBAMI::Interface::ModelIdDiskDrivesConnected:
                if(tabWindow->mediaLayout)
                    tabWindow->mediaLayout->updateVisibility( emulator->getDiskMediaGroup(), block->combo->selection() );

                if (this->emulator == activeEmulator)
                    program->reset(activeEmulator);
                break;
            case LIBAMI::Interface::ModelIdRegion:
                if (tabWindow->videoLayout)
                    tabWindow->videoLayout->updatePresets();
                else if (videoDriver)
                    VideoManager::getInstance( emulator )->reloadSettings();
                // fallthrough
            case LIBAMI::Interface::ModelIdChipMem:
            case LIBAMI::Interface::ModelIdSlowMem:
            case LIBAMI::Interface::ModelIdFastMem:
            case LIBAMI::Interface::ModelIdSystem:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;
            case LIBAMI::Interface::ModelIdSampleFetch:
                audioManager->setResampler();
                break;
        }
    }
}

auto ModelLayout::updateBurstVisibillity() -> void {
    auto blockBurstMode = getBlock( LIBC64::Interface::ModelIdCiaBurstMode );
    auto blockDriveModel = getBlock( LIBC64::Interface::ModelIdDiskDriveModel );
    auto selection = blockDriveModel->combo->selection();

    blockBurstMode->checkBox->setEnabled( selection == 3 || selection == 4 );
}

auto ModelLayout::hintDriveSettings() -> void {
    program->powerOff();

    auto blockFastloader = getBlock( LIBC64::Interface::ModelIdDriveFastLoader );
    auto blockParallel = getBlock( LIBC64::Interface::ModelIdDriveParallelCable );
    auto blockBurst = getBlock( LIBC64::Interface::ModelIdCiaBurstMode );
    auto blockDriveModel = getBlock( LIBC64::Interface::ModelIdDiskDriveModel );
    auto blockRam20 = getBlock( LIBC64::Interface::ModelIdDriveRam20To3F );
    auto blockRam40 = getBlock( LIBC64::Interface::ModelIdDriveRam40To5F );
    auto blockRam60 = getBlock( LIBC64::Interface::ModelIdDriveRam60To7F );
    auto blockRam80 = getBlock( LIBC64::Interface::ModelIdDriveRam80To9F );
    auto blockRamA0 = getBlock( LIBC64::Interface::ModelIdDriveRamA0ToBF );
    auto selection = blockFastloader->combo->selection();

    if (blockBurst->checkBox->checked())
        blockBurst->checkBox->toggle();
    if (!blockParallel->checkBox->checked())
        blockParallel->checkBox->toggle();
    if (blockRam20->checkBox->checked())
        blockRam20->checkBox->toggle();
    if (blockRam40->checkBox->checked())
        blockRam40->checkBox->toggle();
    if (blockRam60->checkBox->checked())
        blockRam60->checkBox->toggle();
    if (blockRam80->checkBox->checked())
        blockRam80->checkBox->toggle();
    if (blockRamA0->checkBox->checked())
        blockRamA0->checkBox->toggle();

    if (selection == 0) {
        blockParallel->checkBox->toggle();
    } else if (selection == 1) { // SpeedDOS
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 2) { // DolphinDOS v2
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 3) { // DolphinDOS v2 Ultimate
        blockRam40->checkBox->toggle();
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 4) { // DolphinDOS v3 1541
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 5) { // DolphinDOS v3 157x
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    } else if (selection == 6) { // ProfDOS v1 1541
        blockRamA0->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 7) { // ProfDOS R4 1541
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 8) { // ProfDOS R5 1570
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(3);
    } else if (selection == 9) { // ProfDOS R6 1571
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    } else if (selection == 10) { // PrologicDOS Classic 1541
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 11) { // PrologicDOS 1541
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 12) { // Turbo Trans
        blockRamA0->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 13) { // Pro Speed 1571
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    }

    blockDriveModel->combo->onChange();

    program->power(emulator);
}

auto ModelLayout::updateBiasVisibillity() -> void {
    
    int filter = emulator->getModelValue( LIBC64::Interface::ModelIdSidFilterType );

    bool showBias6581 = filter == 0 || filter == 1;
    bool showBias8580 = filter == 0;
    
    if (lines[1]->enabled() != showBias6581 )
        lines[1]->setEnabled( showBias6581 );

    if (lines[2]->enabled() != showBias8580 )
        lines[2]->setEnabled( showBias8580 );
}

auto ModelLayout::updateExtraAudioChipsVisibillity() -> void {
    
    static int activeSidsNow = -1;
    
    int activeSids = emulator->getModelValue( LIBC64::Interface::ModelIdSidMulti );
    
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

auto ModelLayout::getAlignedWidth(Emulator::Interface::Model* model) -> unsigned {
    static unsigned sidWidth = 0u;
    unsigned result = 0u; // minimum size

    if (!dynamic_cast<LIBC64::Interface*>( emulator ))
        return result;

    if (!model || ( ((model->id == LIBC64::Interface::ModelIdSid) && GUIKIT::Vector::find(purposes, Emulator::Interface::Model::AudioSettings) )
                || model->id == LIBC64::Interface::ModelIdSid2
                || model->id == LIBC64::Interface::ModelIdSid3 || model->id == LIBC64::Interface::ModelIdSid4
                || model->id == LIBC64::Interface::ModelIdSid5 || model->id == LIBC64::Interface::ModelIdSid6
                || model->id == LIBC64::Interface::ModelIdSid7 || model->id == LIBC64::Interface::ModelIdSid8)) {

        if (sidWidth == 0) {
            GUIKIT::Label test;
            test.setText("SID 10:");
            sidWidth = test.minimumSize().width + (model ? 1 : 2);
        }
        result = sidWidth;
    }

    return result;
}

auto ModelLayout::appendAudioSelectorLayout() -> void {
    
    update( *lines[lines.size() - 1], 10 );    

    controlLayout.append(controlLayout.label, {getAlignedWidth(), 0u}, 5);
    controlLayout.append(controlLayout.firstAll, {0u, 0u}, GUIKIT::Application::isCocoa() ? 7 : 5);
    controlLayout.append(controlLayout.secondAll, {0u, 0u}, 20);
    controlLayout.setAlignment(0.5);
    append(controlLayout, {0u, 0u}, 5);
    
    controlLayout.firstAll.onToggle = [this](bool checked) {
        
        if (!checked)
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

    controlLayout.secondAll.onToggle = [this](bool checked) {

        if (!checked)
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

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        switch (model->id) {
            case LIBC64::Interface::ModelIdSid:
                if (!GUIKIT::Vector::find(purposes, Emulator::Interface::Model::AudioSettings))
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

            case LIBC64::Interface::ModelIdDriveRam20To3F:
            case LIBC64::Interface::ModelIdDriveRam40To5F:
            case LIBC64::Interface::ModelIdDriveRam60To7F:
            case LIBC64::Interface::ModelIdDriveRam80To9F:
            case LIBC64::Interface::ModelIdDriveRamA0ToBF:
            case LIBC64::Interface::ModelIdDriveFastLoader:
                tooltip = "";
                break;

            default:
                tooltip = name + " tooltip";
                break;
        }
    } else if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        tooltip = name + " tooltip";
    }
    
    return name;
}

auto ModelLayout::getUnit(unsigned id) -> std::string {
    if (dynamic_cast<LIBC64::Interface*>(this->emulator)) {
        if (id == LIBC64::Interface::ModelIdDiskDriveStepperSeekTime)
            return " ms";
    } else {
        if (id == LIBAMI::Interface::ModelIdDiskDriveStepperSeekTime || id == LIBAMI::Interface::ModelIdDiskDriveStepperAccessTime)
            return " ms";
    }

    return " RPM";
}

}

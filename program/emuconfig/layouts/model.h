
#pragma once

#include "../../config/slider.h"

namespace EmuConfigView {
    
struct TabWindow;
    
struct ModelLayout : GUIKIT::FramedVerticalLayout {
    
    struct Line : GUIKIT::HorizontalLayout {
        
        struct Block : GUIKIT::HorizontalLayout {
            Emulator::Interface::Model* model;
            GUIKIT::CheckBox checkBox;
			GUIKIT::ComboButton combo;
            SliderLayout slider;
			std::vector<GUIKIT::RadioBox*> options;
            GUIKIT::Label label;
            GUIKIT::LineEdit lineEdit;

            Block(Emulator::Interface::Model* model);
        };
        std::vector<Block*> blocks;       
        
        Line();
    };
    
    std::vector<Line*> lines;
    
    Emulator::Interface* emulator;
    
    TabWindow* tabWindow;
    
    std::vector<Emulator::Interface::Model::Purpose> purposes;
        
    auto build( TabWindow* tabWindow, Emulator::Interface* emulator, std::vector<Emulator::Interface::Model::Purpose> purposes, std::vector<unsigned> dim ) -> void;
    
    auto setEvents( ) -> void;

    auto updateWidget( Line::Block* block ) -> void;
    
    auto updateWidget( unsigned id ) -> void;
    
    auto updateWidgets( ) -> void;
    
    auto toggleCheckbox(unsigned id) -> bool;
    
    auto stepRange( unsigned id, int step ) -> int;
    
    auto nextOption(unsigned id) -> unsigned;
    
    auto translate( bool addCounter = false ) -> void;
    
    auto getIdent( Emulator::Interface::Model* model, bool custom, std::string& tooltip ) -> std::string;
    
    ModelLayout();
};

}

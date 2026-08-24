
#pragma once

struct SliderLayout;

namespace EmuConfigView {
    
struct TabWindow;
    
struct ModelLayout : GUIKIT::FramedVerticalLayout {
    static GUIKIT::Image* backImg;

    struct Line : GUIKIT::HorizontalLayout {
        
        struct Block : GUIKIT::HorizontalLayout {
            Emulator::Interface::Model* model;
            GUIKIT::CheckBox* checkBox = nullptr;
			GUIKIT::ComboButton* combo = nullptr;
            SliderLayout* sliderLayout = nullptr;
			std::vector<GUIKIT::RadioBox*> options;
            GUIKIT::Label* label = nullptr;
            GUIKIT::LineEdit* lineEdit = nullptr;
            GUIKIT::ImageView* imageView = nullptr;

            Block(Emulator::Interface::Model* model, ModelLayout* layout);
        };
        std::vector<Block*> blocks;       
        
        Line();
    };
    
    std::vector<Line*> lines;
       
    Emulator::Interface* emulator;
    
    TabWindow* tabWindow;

    std::vector<Emulator::Interface::Model::Purpose> purposes;

    virtual auto blockWillAppend( Line* line, Line::Block* block ) -> void {}

    virtual auto lineWillAppend( unsigned pos ) -> void {}

    virtual auto updateVisibillity( ) -> void {}

    virtual auto widgetUpdated( Line::Block* block, Emulator::Interface::Model* model ) -> void {}

    virtual auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {}

    virtual auto getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string;

    virtual auto getUnit(Emulator::Interface::Model* model) -> std::string { return ""; }
        
    auto build( TabWindow* tabWindow, Emulator::Interface* emulator, std::vector<Emulator::Interface::Model::Purpose> purposes, std::vector<unsigned> dim, unsigned lineSpace = 5 ) -> void;
    
    auto setEvents( ) -> void;

    auto hasElements() -> bool { return !lines.empty(); }

    auto updateWidget( Line::Block* block ) -> void;
    
    auto updateWidget( unsigned id ) -> void;
    
    auto updateWidgets( ) -> void;
    
    auto toggleCheckbox(unsigned id) -> bool;
    
    auto stepRange( unsigned id, int step ) -> int;
    
    auto nextOption(unsigned id) -> unsigned;
    
    auto translate( std::string theme = "model" ) -> void;

    auto getBlock( unsigned modelId ) -> Line::Block*;

    auto alignSlider( std::string maxText ) -> void;

    auto decimalPlaces(float scaler) -> unsigned;
    
    ModelLayout();
};

}

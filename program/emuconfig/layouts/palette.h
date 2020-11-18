
struct PaletteColorLayout : GUIKIT::HorizontalLayout {    
    GUIKIT::Label color;
    GUIKIT::SquareCanvas canvas;
    GUIKIT::Label hex;
    GUIKIT::LineEdit edit;
    unsigned pos;
    
    PaletteColorLayout(unsigned editWidth, unsigned canvasHeight);
};

struct PaletteControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::LineEdit title;
    GUIKIT::Widget spacer;
    GUIKIT::Label ownPalette;
    GUIKIT::Button create;    
    GUIKIT::Button remove;
    
    PaletteControlLayout();
};

struct PaletteSaveLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Widget spacer;
    GUIKIT::Label title;
    GUIKIT::Button allChanges;
    GUIKIT::CheckBox onExit;
    
    PaletteSaveLayout();
};

struct PaletteDetailLayout : GUIKIT::HorizontalLayout {
    
    struct Left : GUIKIT::VerticalLayout {
        GUIKIT::SquareCanvas canvas;
    } left;
    
    struct Right : GUIKIT::VerticalLayout {
        SliderLayout r;
        SliderLayout g;
        SliderLayout b;
        
        Right();
    } right;
    
    PaletteDetailLayout();
};

struct PaletteLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    unsigned colorPos = 0;
    
    GUIKIT::HorizontalLayout main;
    GUIKIT::ListView listView;
    
    GUIKIT::VerticalLayout paletteLayout;        
    
    std::vector<GUIKIT::HorizontalLayout*> colorLines;    
    std::vector<PaletteColorLayout*> colorLayouts;
    
    PaletteControlLayout controlLayout; 
    PaletteSaveLayout saveLayout;
    PaletteDetailLayout detailLayout;
    
    auto translate() -> void;
    
    auto updateList() -> void;
    
    auto setPalette(Emulator::Interface::Palette& palette) -> void;
    
    auto getSelectedPalette() -> Emulator::Interface::Palette&;
    
    auto updateDetailLayout() -> void;
   
    auto updateSliderChange( uint8_t colorChannel, uint8_t bits ) -> void;
    
    auto markSelectedColor( PaletteColorLayout* selectColorLayout ) -> void;
    
    auto loadSettings() -> void;
    
    PaletteLayout(TabWindow* tabWindow);
};

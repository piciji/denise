
#pragma once

#include "../../../guikit/api.h"
#include "../../program.h"
#include "../../config/slider.h"
#include "../../config/sliderAlt.h"
#include "model.h"

namespace EmuConfigView {

struct PaletteColorLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label color;
    GUIKIT::SquareCanvas canvas;
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
    GUIKIT::Label allChanges;
    GUIKIT::Button save;
    
    PaletteControlLayout();
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
    
    auto translate() -> void;
    
    auto updateList() -> void;
    
    auto setPalette(Emulator::Interface::Palette& palette) -> void;
    
    auto getSelectedPalette() -> Emulator::Interface::Palette&;
   
    auto updateChange( uint32_t rgb ) -> void;
    
    auto markSelectedColor( PaletteColorLayout* selectColorLayout ) -> void;
    
    auto loadSettings() -> void;
    
    PaletteLayout(TabWindow* tabWindow);
};

}

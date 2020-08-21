
struct AudioLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    ModelLayout settingsLayout;
    
    AudioLayout(TabWindow* tabWindow);
    
    auto translate() -> void;
};
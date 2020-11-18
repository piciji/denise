
AudioLayout::AudioLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);        
    
    settingsLayout.custom = true;
    
    settingsLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::SoundChip, Emulator::Interface::Model::Purpose::AudioSettings, Emulator::Interface::Model::Purpose::AudioResampler },
    {4, 1, 1, 3, 4, 4, 4, 4, 4, 4, 4, 4} );   
        
    append(settingsLayout, {~0u, 0u});
    
    settingsLayout.setEvents();
//    settingsLayout.updateWidgets();
    
    loadSettings();
}

auto AudioLayout::translate() -> void {
    
    settingsLayout.translate( );
    
}

auto AudioLayout::loadSettings() -> void {
    settingsLayout.updateWidgets();  
}
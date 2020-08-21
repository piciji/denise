
AudioLayout::AudioLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    
    settingsLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::SoundChip, Emulator::Interface::Model::Purpose::AudioSettings, Emulator::Interface::Model::Purpose::AudioResampler },
    {4, 1, 1, 1} );
    
    append(settingsLayout, {~0u, 0u}, 10);
    
    settingsLayout.setEvents();
    settingsLayout.updateWidgets();
}

auto AudioLayout::translate() -> void {
    
    settingsLayout.translate();
    

}
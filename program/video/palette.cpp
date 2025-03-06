
#include "palette.h"
#include "../program.h"
#include "../cmd/cmd.h"

std::vector<PaletteManager*> paletteManagers;

auto PaletteManager::getInstance( Emulator::Interface* emulator ) -> PaletteManager* {
	
	for (auto paletteManager : paletteManagers) {
		if (paletteManager->emulator == emulator)
			return paletteManager;
	}
    
	return nullptr;
}

PaletteManager::PaletteManager(Emulator::Interface* emulator) {
    
    palSettings = new GUIKIT::Settings;
    this->emulator = emulator;
}

PaletteManager::~PaletteManager() {
    
    if( !cmd->debug)            
        save();
}

auto PaletteManager::getCurrentPalette() -> Emulator::Interface::Palette* {
    
    auto usedPaletteId = program->getSettings(emulator)->get<unsigned>( "palette", 0 );
    Emulator::Interface::Palette* palette = find( usedPaletteId );
    
    if (!palette)
        palette = &emulator->palettes[0];
    
    return palette;
}

auto PaletteManager::rebuildSettings() -> void {
    
    if (palSettings)
        delete palSettings;
    
    palSettings = new GUIKIT::Settings;
        
    for( auto& palette : emulator->palettes ) {
        
        if (!palette.editable)
            continue;
        
        std::string ident = "pal_" + std::to_string(palette.id) + "_name";
        
        palSettings->set<std::string>( ident, palette.name );
        
        for( auto& paletteColor : palette.paletteColors ) {   
            
            ident = "pal_" + std::to_string(palette.id) + "_" + paletteColor.name;
            
            palSettings->set<unsigned>( ident, paletteColor.rgb & 0xffffff );
        }                
    }
}

auto PaletteManager::add(Emulator::Interface::Palette& copy ) -> Emulator::Interface::Palette& {
    
    unsigned paletteId = 50;
    
    for( auto& palette : emulator->palettes ) {
        
        if (!palette.editable)
            continue;
        
        if (palette.id >= paletteId)
            paletteId = palette.id + 1;
    }
    
    emulator->palettes.push_back(copy);
    Emulator::Interface::Palette& palette = emulator->palettes.back();
    palette.id = paletteId;
    palette.name += " copy";
    palette.editable = true;
    
    return palette;
}

auto PaletteManager::remove( Emulator::Interface::Palette& palette ) -> void {
    
    unsigned i = 0;
    unsigned selected = 0;
    
    for( auto& _palette : emulator->palettes ) {
        
        if (&_palette == &palette)
            selected = i;
        
        i++;
    }
    
    if (!emulator->palettes[selected].editable )
        return;
    
    GUIKIT::Vector::eraseVectorPos( emulator->palettes, selected );
}

auto PaletteManager::save() -> bool {
    
    rebuildSettings();
    
    return palSettings->save( path(true) );
}

auto PaletteManager::removeEditablePalettes() -> void {
    auto iter = std::remove_if(emulator->palettes.begin(), emulator->palettes.end(),
        [](const Emulator::Interface::Palette& palette) {
            return palette.editable;
        });

    emulator->palettes.erase(iter, emulator->palettes.end());
}

auto PaletteManager::load() -> void {
    removeEditablePalettes();

    palSettings->load( path(false) );
    
    auto& list = palSettings->getList();
    
    for( auto setting : list ) {
        
        auto parts = GUIKIT::String::split( setting->getIdent(), '_' );
        
        if (parts.size() != 3)
            continue;
        
        if (parts[0] != "pal")
            continue;
        
        try {
            unsigned paletteId = std::stoul( parts[1] );
            
            Emulator::Interface::Palette* palette = find( paletteId );
            
            if (!palette) {
                emulator->palettes.push_back( {paletteId} );   
                palette = &emulator->palettes.back();
                palette->editable = true;
            }
            
            if (parts[2] == "name") {
                palette->name = setting->value;
                continue;
            }
            
            unsigned rgb = setting->uValue;
            rgb &= 0xffffff;
     
            palette->paletteColors.push_back( {parts[2], rgb} );
            palette->paletteColors.back().updateChannels();
                   
        } catch(...) {
            
        }
    }
    
    sanitize();
}

auto PaletteManager::sanitize() -> void {    
    for( auto& palette : emulator->palettes ) {
        if (!reorder( &palette ))
            palette.id = ~0;
    }

    auto iter = std::remove_if(emulator->palettes.begin(), emulator->palettes.end(),
        [](const Emulator::Interface::Palette& palette) {
            return palette.id == ~0;
        });

    emulator->palettes.erase(iter, emulator->palettes.end());
}

auto PaletteManager::reorder(Emulator::Interface::Palette* palette) -> bool {
    
    if (!palette->editable)
        return true;
    
    if (palette->paletteColors.size() != getSize())
        return false;
    
    unsigned* paletteColors = new unsigned[getSize()];
    
    for(auto color : palette->paletteColors) {
        
        GUIKIT::String::toLowerCase( color.name );
        
        bool found = false;
        
        for (unsigned i = 0; i < getSize(); i++) {            
            
            if ( color.name == getIdent(i)) {
                paletteColors[i] = color.rgb;
                found = true;
                break;
            }
        }
        
        if (!found)
            return false;
    }
    
    for (unsigned i = 0; i < getSize(); i++) {
        palette->paletteColors[i].rgb = paletteColors[i];
        palette->paletteColors[i].name = getIdent(i);
        palette->paletteColors[i].updateChannels();
    }

    delete[] paletteColors;
    
    return true;   
}

auto PaletteManager::find( unsigned id ) -> Emulator::Interface::Palette* {
    
    for( auto& palette : emulator->palettes ) {
        
        if (palette.id == id)
            return &palette;
    }
    
    return nullptr;
}

auto PaletteManager::getSize() -> unsigned {
    // expect that first palette is always defined correctly
    return emulator->palettes[0].paletteColors.size();
}

auto PaletteManager::getIdent(unsigned i) -> std::string {
    // expect that first palette is always defined correctly
    auto ident = emulator->palettes[0].paletteColors[i].name;
    
    GUIKIT::String::toLowerCase( ident );
    
    return ident;
}

auto PaletteManager::path(bool createFolder) -> std::string {
    return program->generatedFolder("", createFolder) + emulator->ident + ".pal";
}
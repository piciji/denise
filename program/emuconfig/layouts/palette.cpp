
#include "palette.h"
#include "../config.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"
#include "../../input/manager.h"
#include "../../view/status.h"
#include "../../audio/manager.h"
#include "../../video/palette.h"

#define mes this->tabWindow->message
#define _settings this->tabWindow->settings

namespace EmuConfigView {

PaletteColorLayout::PaletteColorLayout(unsigned editWidth, unsigned canvasHeight) {

    append( color, {~0u, 0u}, 5 );
    append( canvas, {(unsigned)((float)canvasHeight * 1.5), canvasHeight}, 20 );
    append( edit, {editWidth, 0u} );

    color.setFont( GUIKIT::Font::system("bold") );
    canvas.setBorderColor( 1, 0x666666 );
    edit.setMaxLength(6);
    setAlignment( 0.5 );
}

PaletteControlLayout::PaletteControlLayout() {    
    append( title, {200u, 0u});
    append( spacer, {~0u, 0u} );
    append( ownPalette, {0u, 0u}, 10 );    
    append( create, {0u, 0u}, 10 );    
    append( remove, {0u, 0u}, 10 );
    append( allChanges, { 0u, 0u }, 5);
    append( save, { 0u, 0u }, 5);
    setAlignment( 0.5 );
}

PaletteLayout::PaletteLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    
    PaletteManager* paletteManager = PaletteManager::getInstance( emulator );
    GUIKIT::HorizontalLayout* colorLine = nullptr;
    
    paletteLayout.append(controlLayout, {~0u, 0u}, 10);
    
    controlLayout.title.onChange = [this]() {
        
        auto& palette = this->getSelectedPalette();
        
        if (!palette.editable)
            return;
        
        palette.name = controlLayout.title.text();
        
        if (palette.name == "")
            palette.name = "???";
        
        listView.setText( listView.selection(), 0, palette.name );
    };
    
    GUIKIT::LineEdit test1;
    test1.setText( "bbbbbb" );
    auto editWidth = test1.minimumSize().width;
    
    GUIKIT::LineEdit test2;
    test2.setText("F");    
    auto canvasHeight = test2.minimumSize().height - 2;
    
    for(unsigned i = 0; i < paletteManager->getSize(); i++ ) {

        if ((i % 3) == 0) {
            if (colorLine) {
                colorLines.push_back(colorLine);
                paletteLayout.append(*colorLine, {~0u, 0u}, 10);
            }

            colorLine = new GUIKIT::HorizontalLayout;
        } 
        
        PaletteColorLayout* colorLayout = new PaletteColorLayout( editWidth, canvasHeight );
        
        colorLayout->pos = i;        
        
        colorLayout->edit.onChange = [this, colorLayout]() {

            auto& palette = this->getSelectedPalette();

            if (!palette.editable)
                return;
            
            colorPos = colorLayout->pos;
            
            std::string rgb = colorLayout->edit.text();                        
            
            palette.paletteColors[colorPos].rgb = GUIKIT::String::convertHexToInt( rgb, 0 );
            colorLayouts[colorPos]->canvas.setBackgroundColor( palette.paletteColors[colorPos].rgb );
            palette.paletteColors[colorPos].updateChannels();

            emuThread->lock();
            program->setPalette( this->emulator );
            emuThread->unlock();
        };
		
        colorLayout->edit.onFocus = [this, colorLayout]() {
            auto& palette = this->getSelectedPalette();

            if (!palette.editable)
                return;

            colorPos = colorLayout->pos;
            markSelectedColor(colorLayout);
        };

        colorLayout->canvas.onMouseRelease = [this, colorLayout](GUIKIT::Mouse::Button button) {

            auto& palette = this->getSelectedPalette();

            if (!palette.editable)
                return;

            colorPos = colorLayout->pos;
            markSelectedColor(colorLayout);

            GUIKIT::ColorChooser colorChooser;
            colorChooser.onChoose = [this](unsigned color) {
                updateChange(color );
            };

            unsigned defaultColor = palette.paletteColors[colorPos].rgb;

            colorChooser.setWindow( *this->tabWindow );
            colorChooser.setDefault( defaultColor );
            auto result = colorChooser.choose();
            if (GUIKIT::Application::isQuit)
                return;

            updateChange(result.value_or( defaultColor ));
        };

        if ((i % 3) != 0)
            colorLayout->color.setAlign( GUIKIT::Label::Align::Right );
        else if (i == (paletteManager->getSize()-1))
            colorLayout->color.setAlign( GUIKIT::Label::Align::Right );

        colorLine->append( *colorLayout, {~0u, 0u} );

        colorLayouts.push_back( colorLayout );
    }

    colorLines.push_back(colorLine);
    paletteLayout.append(*colorLine,{~0u, 0u}, 10);

    main.append( listView, { GUIKIT::Font::scale( 180 ), paletteLayout.minimumSize().height - 10}, 10 );
    main.append( paletteLayout, {~0u, 0u} );
    append(main, {~0u, 0u}, 10);

    listView.onChange = [this]() {

        auto& palette = getSelectedPalette();

        _settings->set<unsigned>( "palette", palette.id );

        emuThread->lock();
        program->setPalette( this->emulator );
        emuThread->unlock();

        this->setPalette( palette );
    };

    controlLayout.create.onActivate = [this]() {

        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );

        emuThread->lock();
        auto& palette = paletteManager->add( getSelectedPalette() );
        emuThread->unlock();

        _settings->set<unsigned>( "palette", palette.id );

        updateList();

        setPalette( palette );
    };

    controlLayout.remove.onActivate = [this]() {

        if ( !mes->question( trans->get("palette_remove_question") ) )
            return;

        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );

        Emulator::Interface::Palette& palette = getSelectedPalette();

        if (!palette.editable)
            return;

        emuThread->lock();
        paletteManager->remove( palette );

        auto& _palette = emulator->palettes[0];

        _settings->set<unsigned>( "palette", _palette.id );

        updateList();

        setPalette( _palette );

        program->setPalette( this->emulator );
        emuThread->unlock();
    };

    controlLayout.save.onActivate = [this]() {

        PaletteManager* paletteManager = PaletteManager::getInstance( emulator );

        if ( !paletteManager->save() )
            statusHandler->setMessage(trans->get("file_creation_error", { {"%path%", paletteManager->path()}} ), true);
        else {
            emuThread->lock();
            statusHandler->setMessage(trans->get("file_creation_success", {{"%path%", paletteManager->path()}} ));
            emuThread->unlock();
        }
    };

    loadSettings();
}

auto PaletteLayout::setPalette(Emulator::Interface::Palette& palette) -> void {

    auto& paletteColors = palette.paletteColors;

    controlLayout.title.setText( palette.name );

    controlLayout.title.setEnabled( palette.editable );

    controlLayout.remove.setEnabled( palette.editable );


    for( auto colorLayout : colorLayouts ) {

        colorLayout->edit.setText( GUIKIT::String::prependZero( GUIKIT::String::convertIntToHex( paletteColors[colorLayout->pos].rgb, false ), 6) );

        colorLayout->edit.setEnabled( palette.editable );

        colorLayout->canvas.setBackgroundColor( paletteColors[colorLayout->pos].rgb );

        colorLayout->color.setFont( GUIKIT::Font::system() );
    }
}

auto PaletteLayout::getSelectedPalette() -> Emulator::Interface::Palette& {

    unsigned size = emulator->palettes.size();

    if (listView.selection() >= size)
        return emulator->palettes[0];

    return emulator->palettes[ listView.selection() ];
}

auto PaletteLayout::updateList() -> void {

    auto usedPaletteId = _settings->get<unsigned>( "palette", 0 );

    listView.reset();

    unsigned i = 0;
    unsigned selected = 0;

    for ( auto& palette : emulator->palettes ) {

        listView.append( {trans->get( palette.name )} );

        if (palette.id == usedPaletteId)
            selected = i;

        i++;
    }

    listView.setSelection( selected );
}

auto PaletteLayout::updateChange( uint32_t rgb ) -> void {

    auto& palette = getSelectedPalette();

    if (!palette.editable)
        return;

    palette.paletteColors[colorPos].rgb = rgb;
    palette.paletteColors[colorPos].updateChannels();

    program->setPalette(this->emulator);

    colorLayouts[colorPos]->canvas.setBackgroundColor(rgb);

    colorLayouts[colorPos]->edit.setText( GUIKIT::String::prependZero( GUIKIT::String::convertIntToHex(rgb, false), 6 ) );
}

auto PaletteLayout::translate() -> void {

    PaletteManager* paletteManager = PaletteManager::getInstance( emulator );

    for(auto colorLayout : colorLayouts) {

        colorLayout->color.setText( trans->get( paletteManager->getIdent( colorLayout->pos ) ) );
    }


    controlLayout.ownPalette.setText( trans->get("own_palette", {}, true) );
    controlLayout.create.setText( trans->get("create") );
    controlLayout.remove.setText( trans->get("remove") );
    controlLayout.allChanges.setText(trans->get("all_changes"));
    controlLayout.save.setText(trans->get("save"));
}

auto PaletteLayout::markSelectedColor( PaletteColorLayout* selectColorLayout ) -> void {

    for( auto colorLayout : colorLayouts ) {

        if (selectColorLayout == colorLayout)
            colorLayout->color.setFont( GUIKIT::Font::system("bold") );
        else
            colorLayout->color.setFont( GUIKIT::Font::system() );
    }
}

auto PaletteLayout::loadSettings() -> void {
    updateList();
    setPalette(getSelectedPalette());
}

}

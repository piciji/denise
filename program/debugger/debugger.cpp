
#include "debugger.h"

#include "../../emulation/libami/interface.h"
#include "../program.h"
#include "../thread/emuThread.h"
#include "../../data/icons.h"

#include "cpuDebugger.h"
#include "../../emulation/interface.h"

GUIKIT::Timer* Debugger::timerVisibility = nullptr;

Debugger::~Debugger() {
    timerVisibility->setEnabled( false );
    setVisible(false);
}

Debugger::Debugger( Emulator::Interface* emulator, Mode mode )
: emulator( emulator ), mode( mode ) {
    this->settings = program->getSettings( emulator );
}

Debugger::Control::Control(Debugger* debugger) {
    stepOver.setEnabled( false );
    stepInto.setEnabled( false );
    stepOut.setEnabled( false );
    line.setEnabled( false );
    frame.setEnabled( false );

    searchEdit.setMaxLength( 8 );
    searchEdit.setFont(GUIKIT::Font::system(11));
    position.setFont( GUIKIT::Font::system( 11 ) );

    append( spacer, {0u, 0u}, 10 );
    append( resume, {0u, 0u}, 10 );
    append( stepOver, {0u, 0u}, 10 );
    append( stepInto, {0u, 0u}, 10 );
    append( stepOut, {0u, 0u}, 10 );
    append( line, {0u, 0u}, 10 );
    append( frame, {0u, 0u}, 30 );
    append( searchEdit, {120u, 0u}, 10 );
    append( search, {0u, 0u}, 20 );
    append( position, {~0u, 0u} );

    if (auto _control = debugger->buildControl())
        append(*_control, {0u, 0u}, 20);

    append( showTips, {0u, 0u} );

    setAlignment( 0.5 );
}

auto Debugger::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();

    GUIKIT::Geometry defaultGeometry = {50, 50, GUIKIT::Font::scale(1024), GUIKIT::Font::scale(570)};

    GUIKIT::Geometry geometry = {settings->get<int>(saveIdent() + "_x", defaultGeometry.x)
        ,settings->get<int>(saveIdent() + "_y", defaultGeometry.y)
        ,settings->get<unsigned>(saveIdent() + "_width", defaultGeometry.width)
        ,settings->get<unsigned>(saveIdent() + "_height", defaultGeometry.height)
    };

    setGeometry( geometry );

    if (isOffscreen())
        setGeometry( defaultGeometry );

    addImg.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    breakEnableImg.loadPng((uint8_t*)Icons::recordHi, sizeof(Icons::recordHi));
    breakDisableImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    // don't share list view images at different scaling sizes, otherwise it scales again and again
    breakEnableSmallImg.loadPng((uint8_t*)Icons::recordHi, sizeof(Icons::recordHi));
    breakDisableSmallImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));

    searchImg.loadPng((uint8_t*)Icons::search, sizeof(Icons::search));
    trashImg.loadPng((uint8_t*)Icons::trash, sizeof(Icons::trash));

    pauseImg.loadPng((uint8_t*)Icons::pause, sizeof(Icons::pause));
    resumeImg.loadPng((uint8_t*)Icons::resume, sizeof(Icons::resume));
    stepIntoImg.loadPng((uint8_t*)Icons::stepInto, sizeof(Icons::stepInto));
    stepOverImg.loadPng((uint8_t*)Icons::stepOver, sizeof(Icons::stepOver));
    stepOutImg.loadPng((uint8_t*)Icons::stepOut, sizeof(Icons::stepOut));

    lineImg.loadPng((uint8_t*)Icons::line, sizeof(Icons::line));
    frameImg.loadPng((uint8_t*)Icons::frame, sizeof(Icons::frame));

    memoryImg.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    exceptionImg.loadPng((uint8_t*)Icons::exception, sizeof(Icons::exception));
    clearImg.loadPng((uint8_t*)Icons::clear, sizeof(Icons::clear));

    offImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    onImg.loadPng((uint8_t*)Icons::ledGreenRound, sizeof(Icons::ledGreenRound));

    editImg.loadPng((uint8_t*)Icons::edit, sizeof(Icons::edit));
    checkedImg.loadPng((uint8_t*)Icons::checked, sizeof(Icons::checked));

    control = new Control(this);

    control->resume.setImage( &pauseImg );
    control->stepOver.setImage( &stepOverImg );
    control->stepInto.setImage( &stepIntoImg );
    control->stepOut.setImage( &stepOutImg );
    control->search.setImage( &searchImg );
    control->line.setImage( &lineImg );
    control->frame.setImage( &frameImg );

    layout.setMargin( 10 );

    layout.append( *buildTheme(), {~0u, ~0u}, 10 );
    layout.append( *control, {~0u, 0u} );

    append( layout );

    onClose = [this]() {
        emuThread->lock();
        closeTheme();
        setVisible(false);

        if (!program->hasActiveDebugger()) {
            program->isPause &= ~2;
            timerVisibility->setEnabled( false );
        }
        emuThread->unlock();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>(saveIdent() + "_x", geometry.x);
        settings->set<int>(saveIdent() + "_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>(saveIdent() + "_width", geometry.width);
        settings->set<unsigned>(saveIdent() + "_height", geometry.height);
    };

    control->stepInto.onActivate = [this]() {
        stepInto( emulator );
    };

    control->stepOut.onActivate = [this]() {
        stepOut( emulator );
    };

    control->stepOver.onActivate = [this]() {
        stepOver( emulator );
    };

    control->line.onActivate = [this]() {
        stepLine( emulator );
    };

    control->frame.onActivate = [this]() {
        stepFrame( emulator );
    };

    control->resume.onActivate = [this]() {
        resume( emulator );
    };

    control->search.onClick = [this]() {
        if (emulator != activeEmulator)
            return;

        std::string addressText = control->searchEdit.text();
        if (addressText.empty())
            return;
        GUIKIT::String::remove( addressText, {"$", "0x"} );

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        searchTheme(address);
    };

    control->searchEdit.onReturn = [this]() {
        control->search.onClick();
    };

    control->showTips.onToggle = [this](bool checked) {
        settings->set<bool>("debugger_tips", checked);
        emuThread->lock();
        for (auto debugger : debuggers) {
            if (debugger->emulator == emulator) {
                if (debugger != this)
                    debugger->control->showTips.setChecked( checked );
                debugger->translate();
            }
        }
        emuThread->unlock();
    };

    if (!timerVisibility) {
        timerVisibility = new GUIKIT::Timer();
        timerVisibility->setInterval( 20 );
        timerVisibility->onFinished = [this]() {
            if (timerVisibility->enabled()) {
                for (auto& debugger : program->getActiveDebuggers())
                    debugger->updateToolboxVisibility();

                timerVisibility->setEnabled(false);
            }
        };
    }

    control->showTips.setChecked( settings->get<bool>("debugger_tips", true) );
    
    setTitle( titleIdent() );

    translate();
}

auto Debugger::translate() -> void {
    bool showTips = control->showTips.checked();
    control->searchEdit.setPlaceholder( trans->getA( "address" ) );
    control->showTips.setText( trans->getA("popup hints") );
    control->stepInto.setTooltip( showTips ? trans->getA("step into") : "" );
    control->stepOver.setTooltip( showTips ? trans->getA("step over") : "" );
    control->stepOut.setTooltip( showTips ? trans->getA("step out") : "" );
    control->line.setTooltip( showTips ? trans->getA("step end of line") : "" );
    control->frame.setTooltip( showTips ? trans->getA("step end of frame") : "" );

    translateTheme();
}

auto Debugger::updateToolboxVisibility() -> void {
    if (isPaused()) {
        if (control->resume.image() == &pauseImg) {
            control->resume.setImage( &resumeImg );
            control->stepOver.setEnabled( );
            control->stepInto.setEnabled( );
            control->stepOut.setEnabled( );
            control->line.setEnabled( );
            control->frame.setEnabled( );
        }
    } else {
        if (control->resume.image() == &resumeImg) {
            control->resume.setImage( &pauseImg );
            control->stepOver.setEnabled( false );
            control->stepInto.setEnabled( false );
            control->stepOut.setEnabled( false );
            control->line.setEnabled( false );
            control->frame.setEnabled( false );
        }
    }
}

auto Debugger::Callback(Emulator::Interface::DebuggerSnapshot* snapshot) -> void {
    if (snapshot->callbackAction != DebuggerAction::AutoUpdate)
        program->isPause |= 2;

    for (auto debugger : program->getActiveDebuggers()) {
        debugger->snapshot = snapshot;
        debugger->prepareTheme();
    }

    if (emuThread->enabled)
        emuThread->events |= EmuThread::EVT_DEBUGGER;
    else
        Callback();
}

auto Debugger::Callback() -> void {
    Emulator::Interface::DebuggerSnapshot* snapshot = nullptr;
    timerVisibility->setEnabled(false);
    bool _threaded = emuThread->enabled;

    for (auto debugger : program->getActiveDebuggers()) {
        if (debugger->snapshot) {
            if (!snapshot) {
                snapshot = debugger->snapshot;
                if (_threaded)
                    snapshot->mutex.lock();
            }
            debugger->updateTheme();
        }

        debugger->updateToolboxVisibility();
    }

    if (snapshot) {
        if (_threaded)
            snapshot->mutex.unlock();
    }
}

auto Debugger::stepOut(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    if (emulator->debuggerStepOut()) {
        program->isPause &= ~2;
        timerVisibility->setEnabled();
    }
    emuThread->unlock();
}

auto Debugger::stepInto(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timerVisibility->setEnabled();
    emulator->debuggerStepInto();
    emuThread->unlock();
}

auto Debugger::stepOver(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timerVisibility->setEnabled();
    emulator->debuggerStepOver();
    emuThread->unlock();
}

auto Debugger::stepLine(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timerVisibility->setEnabled();
    emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::Line, 0 );
    emuThread->unlock();
}

auto Debugger::stepFrame(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timerVisibility->setEnabled();

    emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::Frame, 0 );
    emuThread->unlock();
}

auto Debugger::resume(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    program->isPause ^= 2;

    emuThread->lock();
    if (isPaused())
        emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::AutoUpdate, 0 );
    timerVisibility->setEnabled();
    emuThread->unlock();
}

auto Debugger::reset() -> void {
    for (auto debugger : program->getActiveDebuggers()) {
        debugger->initTheme();
        debugger->updateToolboxVisibility();
    }
}

auto Debugger::makeVisible() -> void {
    bool result = program->hasActiveDebugger();
    setVisible();
    if (emulator != activeEmulator)
        return;

    if (!result)
        program->isPause &= ~2;

    updateToolboxVisibility();

    emuThread->lock();
    initTheme();
    if (isPaused())
        emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::AutoUpdate, 0 );
    emuThread->unlock();
}

auto Debugger::isPaused() -> bool {
    return (program->isPause & 2) == 2;
}

auto Debugger::isC64() -> bool {
    return dynamic_cast<LIBC64::Interface*>(emulator);
}

auto Debugger::isAmiga() -> bool {
    return dynamic_cast<LIBAMI::Interface*>(emulator);
}

auto Debugger::updateReg(GUIKIT::LineEdit& reg, unsigned val) -> void {
    if ((unsigned)reg.getStore() != val) {
        reg.setStore( static_cast<int>(val) );
        reg.setText( GUIKIT::String::convertToHex( val ) );
    }
}

auto Debugger::updateReg(GUIKIT::LineEdit& widget, const std::string& text, unsigned ident) -> void {
    if ((unsigned)widget.getStore() != ident) {
        widget.setStore( static_cast<int>(ident) );
        widget.setText( text );
    }
}

auto Debugger::updateReg(GUIKIT::CheckBox& reg, bool state) -> void {
    if (reg.checked() != state) {
        reg.setChecked( state );
    }
}

auto Debugger::getWidth(unsigned length, bool editField, bool bigger) -> unsigned {
    static unsigned _w[8][2][2] = {0};
    GUIKIT::Widget* widget;

    unsigned& _width = _w[length - 1][bigger ? 1 : 0][editField ? 1 : 0];

    if (_width == 0) {
        if (editField)
            widget = new GUIKIT::LineEdit;
        else
            widget = new GUIKIT::Label;

        if (bigger)
            widget->setFont( GUIKIT::Font::system( 11, "", true ) );
        else
            widget->setFont( GUIKIT::Font::system( "", true ) );

        std::string _str;
        while (length--)
            _str += "0";

        widget->setText( _str );
        _width = widget->minimumSize().width;
        delete widget;
    }

    return _width;
}

auto Debugger::updateControl(uint16_t v, uint8_t h) -> void {
    control->position.setText("V: " + GUIKIT::String::convertToHex( v, 3 ) + " H: " + GUIKIT::String::convertToHex( h, 2 ) );
}

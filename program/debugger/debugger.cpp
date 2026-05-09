
#include "debugger.h"

#include "../../emulation/libami/interface.h"
#include "../program.h"
#include "../thread/emuThread.h"
#include "../../data/icons.h"
#include "../../emulation/interface.h"
#include "memDebugger.h"
#include "cpuDebugger.h"
#include "copperDebugger.h"
#include "conditionViewDebugger.h"
#include "watcherHelper.h"
#include "../helper/settingsHelper.h"
#include "../view/view.h"
#include <bitset>

GUIKIT::Timer* Debugger::timerVisibility = nullptr;

Debugger::~Debugger() {
    timerVisibility->setEnabled( false );
    setVisible(false);

    if (themeLayout) {
        layout.remove(*themeLayout);
        delete themeLayout;
    }

    layout.remove(*control);
    delete control;
}

Debugger::Debugger( Emulator::Interface* emulator )
: emulator( emulator ) {
    this->settings = Program::getSettings( emulator );
}

Debugger::Control::Control(Debugger* debugger) {
    stepOver.setEnabled( false );
    stepInto.setEnabled( false );
    stepOut.setEnabled( false );
    line.setEnabled( false );
    frame.setEnabled( false );

    lineEdit.setMaxLength( 3 );
    position.setFont( GUIKIT::Font::monospace() );

    append( spacer, {0u, 0u}, 10 );
    append( resume, {0u, 0u}, 10 );
    append( stepOver, {0u, 0u}, 10 );
    append( stepInto, {0u, 0u}, 10 );
    append( stepOut, {0u, 0u}, 10 );
    append( frame, {0u, 0u}, 10 );
    append( line, {0u, 0u}, 10 );
    append( lineEdit, {50u, 0u}, 5 );
    append( toLine, {0u, 0u}, 10 );
    append( position, {100u, 0u}, 0 );

    if (auto _control = debugger->buildControl())
        append(*_control, {~0u, 0u}, 10);
    else
        append( spacer2, {~0u, 0u} );

    append( settings, {0u, 0u} );

    setAlignment( 0.5 );
}

auto Debugger::build() -> void {
    GUIKIT::Geometry defaultGeometry = {50, 50, GUIKIT::Font::scale(1050), GUIKIT::Font::scale(550)};

    if (GUIKIT::Application::isGtk()) {
        defaultGeometry.height += 40;
    }

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
    breakDisableImg.loadPng((uint8_t*)Icons::circleGray, sizeof(Icons::circleGray));
    breakCondEnableImg.loadPng((uint8_t*)Icons::circleBlue, sizeof(Icons::circleBlue));
    // don't share list view images at different scaling sizes, otherwise it scales again and again
    breakEnableSmallImg.loadPng((uint8_t*)Icons::recordHi, sizeof(Icons::recordHi));
    breakDisableSmallImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    breakCondEnableSmallImg.loadPng((uint8_t*)Icons::circleBlue, sizeof(Icons::circleBlue));

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
    memoryBorderImg.loadPng((uint8_t*)Icons::memoryBorder, sizeof(Icons::memoryBorder));
    processorImg.loadPng((uint8_t*)Icons::processor, sizeof(Icons::processor));
    exceptionImg.loadPng((uint8_t*)Icons::exception, sizeof(Icons::exception));
    clearImg.loadPng((uint8_t*)Icons::clear, sizeof(Icons::clear));

    offImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    onImg.loadPng((uint8_t*)Icons::ledGreenRound, sizeof(Icons::ledGreenRound));

    editImg.loadPng((uint8_t*)Icons::edit, sizeof(Icons::edit));
    checkedImg.loadPng((uint8_t*)Icons::checked, sizeof(Icons::checked));
    forwardImg.loadPng((uint8_t*)Icons::forward, sizeof(Icons::forward));
    systemImg.loadPng((uint8_t*)Icons::system, sizeof(Icons::system));
    nextImg.loadPng((uint8_t*)Icons::next, sizeof(Icons::next));
    arrowLeftImg.loadPng((uint8_t*)Icons::arrowLeft, sizeof(Icons::arrowLeft));
    arrowRightImg.loadPng((uint8_t*)Icons::arrowRight, sizeof(Icons::arrowRight));

    control = new Control(this);

    control->resume.setImage( &pauseImg );
    control->stepOver.setImage( &stepOverImg );
    control->stepInto.setImage( &stepIntoImg );
    control->stepOut.setImage( &stepOutImg );
    control->line.setImage( &lineImg );
    control->frame.setImage( &frameImg );
    control->toLine.setImage( &forwardImg );
    control->settings.setImage( &systemImg );

    layout.setMargin( 10 );

    themeLayout = buildTheme();
    layout.append( *themeLayout, {~0u, ~0u}, 10 );
    layout.append( *control, {~0u, 0u} );

    append( layout );

    onClose = [this]() {
        emuThread->lock();
        closeTheme();
        setVisible(false);

        if (!program->hasActiveDebugger()) {
            emuThread->unlockDebugger();
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
        stepInto( emulator, getCpuTheme() );
    };

    control->stepOut.onActivate = [this]() {
        stepOut( emulator, getCpuTheme() );
    };

    control->stepOver.onActivate = [this]() {
        stepOver( emulator, getCpuTheme() );
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

    control->toLine.onClick = [this]() {
        if (emulator != activeEmulator)
            return;

        std::string lineTxt = control->lineEdit.text();
        if (lineTxt.empty())
            return;

        int line = GUIKIT::String::convertToNumber( lineTxt, -1 );
        if (line == -1)
            return;

        stepLine( emulator, line );
    };

    control->lineEdit.onReturn = [this]() {
        control->toLine.onClick();
    };

    showTipsItem.onToggle = [this]() {
        bool checked = showTipsItem.checked();
        settings->set<bool>("debugger_tips", checked);
        emuThread->lock();
        for (auto debugger : debuggers) {
            if (debugger->emulator == emulator) {
                if (debugger != this)
                    debugger->showTipsItem.setChecked( checked );
                debugger->translate();
            }
        }
        emuThread->unlock();
    };

    appendDebuggerItems();

    settingsMenu.append( *new GUIKIT::MenuSeparator );

    settingsMenu.append( showTipsItem );

    control->settings.onMenu = [this]() {
        return &settingsMenu;
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

    showTipsItem.setChecked( settings->get<bool>("debugger_tips", true) );
    
    setTitle( titleIdent() );

    translate();
}

auto Debugger::appendDebuggerItems() -> void {
    std::vector<DebuggerTheme> themes;

    if (isC64()) {
        themes = {
            DebuggerTheme::CPU, DebuggerTheme::SCPU, DebuggerTheme::Memory, DebuggerTheme::MemorySCPU,
            DebuggerTheme::CIA, DebuggerTheme::Video, DebuggerTheme::DMA, DebuggerTheme::SID,
            DebuggerTheme::Unspecified,
            DebuggerTheme::Drive8CPU, DebuggerTheme::Drive8Memory, DebuggerTheme::Drive8VIA,
            DebuggerTheme::Unspecified,
            DebuggerTheme::Drive9CPU, DebuggerTheme::Drive9Memory, DebuggerTheme::Drive9VIA,
            DebuggerTheme::Unspecified,
            DebuggerTheme::Drive10CPU, DebuggerTheme::Drive10Memory, DebuggerTheme::Drive10VIA,
            DebuggerTheme::Unspecified,
            DebuggerTheme::Drive11CPU, DebuggerTheme::Drive11Memory, DebuggerTheme::Drive11VIA,
        };
    } else {
        themes = {
            DebuggerTheme::CPU, DebuggerTheme::Memory, DebuggerTheme::CIA, DebuggerTheme::Video,
            DebuggerTheme::DMA, DebuggerTheme::Copper, DebuggerTheme::Blitter,
            DebuggerTheme::Agnus, DebuggerTheme::Paula, DebuggerTheme::Serial,
        };
    }

    for (auto& theme : themes) {
        if (theme == getTheme())
            continue;

        if (theme == DebuggerTheme::Unspecified) {
            auto item = new GUIKIT::MenuSeparator;
            settingsMenu.append( *item );
            continue;
        }

        auto item = new GUIKIT::MenuItem;
        item->setText( View::getReadable( theme, emulator ) );
        item->onActivate = [this, theme]() {
            emuThread->lock();
            program->openDebugger(emulator, theme);
            emuThread->unlock();
        };
        settingsMenu.append( *item );
    }
}

auto Debugger::translate() -> void {
    bool showTips = showTipsItem.checked();
    control->lineEdit.setPlaceholder( trans->getA( "line" ) );
    showTipsItem.setText( trans->getA("popup hints") );
    control->stepInto.setTooltip( showTips ? trans->getA("step into") : "" );
    control->stepOver.setTooltip( showTips ? trans->getA("step over") : "" );
    control->stepOut.setTooltip( showTips ? trans->getA("step out") : "" );
    control->line.setTooltip( showTips ? trans->getA("step next line") : "" );
    control->toLine.setTooltip( showTips ? trans->getA("step selected line") : "" );
    control->frame.setTooltip( showTips ? trans->getA("step next frame") : "" );

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
    if (program->quitInProgress || !emuThread->enabled) {
        snapshot->mutex.unlock();
        return;
    }

    for (auto debugger : program->getActiveDebuggers()) {
        debugger->snapshot = snapshot;
        debugger->prepareTheme(false);
    }

    emuThread->events |= EmuThread::EVT_DEBUGGER;
    snapshot->mutex.unlock();

    if (snapshot->callbackAction != DebuggerAction::AutoUpdate) {
        emuThread->lockDebugger();
    }
}

auto Debugger::Callback() -> void {
    Emulator::Interface::DebuggerSnapshot* snapshot = nullptr;
    timerVisibility->setEnabled(false);

    for (auto debugger : program->getActiveDebuggers()) {
        if (debugger->snapshot) {
            if (!snapshot) {
                snapshot = debugger->snapshot;
                snapshot->mutex.lock();
            }
            debugger->updateTheme();
        }

        debugger->updateToolboxVisibility();
    }

    if (snapshot) {
        snapshot->mutex.unlock();
    }
}

auto Debugger::stepOut(Emulator::Interface* emulator, DebuggerTheme theme) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;

    if (emulator->debuggerStepOut(theme)) {
        emuThread->unlockDebugger();
        timerVisibility->setEnabled();
    }
}

auto Debugger::stepInto(Emulator::Interface* emulator, DebuggerTheme theme) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    timerVisibility->setEnabled();
    emulator->debuggerStepInto(theme);
    emuThread->unlockDebugger();
}

auto Debugger::stepOver(Emulator::Interface* emulator, DebuggerTheme theme) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;

    timerVisibility->setEnabled();
    emulator->debuggerStepOver(theme);
    emuThread->unlockDebugger();
}

auto Debugger::stepLine(Emulator::Interface* emulator, unsigned line) -> void {
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    timerVisibility->setEnabled();
    emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::Line, line );
    emuThread->unlockDebugger();
    emuThread->unlock();
}

auto Debugger::stepFrame(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;

    program->isPause &= ~2;
    timerVisibility->setEnabled();
    emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::Frame, 0 );
    emuThread->unlockDebugger();
}

auto Debugger::resume(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;

    if (!isPaused()) {
        emuThread->lock();
        emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::UIRequestedStop, 0 );
        emuThread->unlock();
    } else {
        emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::AutoUpdate, 0 );
        emuThread->unlockDebugger();
    }
    timerVisibility->setEnabled();
}

auto Debugger::haltCpu(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    timerVisibility->setEnabled();
    emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::HaltCPU, 0 );
    emuThread->unlockDebugger();
    emuThread->unlock();
}

auto Debugger::reset() -> void {
    for (auto debugger : program->getActiveDebuggers()) {
        debugger->initTheme();
        debugger->updateToolboxVisibility();
    }
}

auto Debugger::makeVisible() -> void {
    setVisible();
    setFocused();
    if (emulator != activeEmulator)
        return;

    updateToolboxVisibility();

    initTheme();
    if (isPaused()) {
        if (!snapshot) {
            for (auto debugger : program->getActiveDebuggers()) {
                if (debugger->snapshot) {
                    snapshot = debugger->snapshot;
                    break;
                }
            }
        }

        if (snapshot) {
            emulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::AutoUpdate, 1 );
            prepareTheme(true);
            updateTheme();
        }
    }
}

auto Debugger::isPaused() -> bool {
    return emuThread->debugging;
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

auto Debugger::updateRegDec(GUIKIT::LineEdit& reg, unsigned val) -> void {
    if ((unsigned)reg.getStore() != val) {
        reg.setStore( static_cast<int>(val) );
        reg.setText( std::to_string(val) );
    }
}

template <unsigned length>
auto Debugger::updateRegBin(GUIKIT::LineEdit& reg, unsigned val) -> void {
    if ((unsigned)reg.getStore() != val) {
        reg.setStore( static_cast<int>(val) );
        reg.setText( std::bitset<length>(val).to_string() );
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

auto Debugger::updateReg(GUIKIT::RadioBox& reg) -> void {
    if (!reg.checked()) {
        reg.setChecked();
    }
}

auto Debugger::hilight(GUIKIT::CheckBox& reg, bool state) -> void {
    if (reg.overrideForegroundColor() != state) {
        if (state) {
            reg.setForegroundColor( SUCCESS_COLOR );
            reg.setFont(GUIKIT::Font::system( "bold", true ));
        } else {
            reg.resetForegroundColor();
            reg.setFont(GUIKIT::Font::monospace());
        }
    }
}

auto Debugger::getWidth(unsigned length, bool editField) -> unsigned {
    static unsigned _w[8][2] = {0};
    GUIKIT::Widget* widget;

    unsigned& _width = _w[length - 1][editField ? 1 : 0];

    if (_width == 0) {
        if (editField)
            widget = new GUIKIT::LineEdit;
        else
            widget = new GUIKIT::Label;

        widget->setFont( GUIKIT::Font::monospace(  ) );

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
    control->position.setText("V:" + std::to_string( v ) + " H:" +  std::to_string( h ) );
}

auto Debugger::changeMemory(const std::string& addrStr, const std::string& valStr) -> void {
    if (valStr.empty() || addrStr.empty())
        return;

    int addr = GUIKIT::String::convertHexToInt( addrStr, -1 );
    if (addr == -1)
        return;

    auto valStrs = GUIKIT::String::split( valStr, ' ', true );

    if (valStrs.empty())
        return;

    std::vector<uint16_t> values;

    for (auto& _valStr : valStrs) {
        int val = GUIKIT::String::convertHexToInt( _valStr, -1 );
        if (val != -1) {
            values.push_back( val );
        }
    }

    if (values.empty())
        return;

    emuThread->lock();
    emulator->editMemory( getTheme(), addr, values);

    if (isPaused()) {
        if (snapshot)
            snapshot->codeMaybeModified = true;

        for (auto& debugger : program->getActiveDebuggers()) {
            if (debugger->isDriveMem() || debugger->getTheme() == DebuggerTheme::Memory || debugger->getTheme() == DebuggerTheme::MemorySCPU)
                (dynamic_cast<MemDebugger*>(debugger))->memChanged(false);

            if (debugger->isDriveCpu() || debugger->getTheme() == DebuggerTheme::CPU || debugger->getTheme() == DebuggerTheme::SCPU) {
                (dynamic_cast<CpuDebugger*>(debugger))->memChanged();
            }

            if (debugger->getTheme() == DebuggerTheme::Copper) {
                (dynamic_cast<CopperDebugger*>(debugger))->memChanged();
            }
        }
    }

    emuThread->unlock();
}

auto Debugger::updateInstructionBreakpointVisuals(GUIKIT::ListView& listView, unsigned row, DbgWatcher* watcher, bool preventColumResizing) -> void {
    if (watcher->enabled) {
        if (watcher->useHitCount || watcher->useExpression)
            listView.setImage( row, 0, breakCondEnableImg, preventColumResizing );
        else
            listView.setImage( row, 0, breakEnableImg, preventColumResizing );

        listView.setRowForegroundColor( DEBUG_COLOR, row );
    } else {
        listView.setImage( row, 0, breakDisableImg, preventColumResizing);
        if (!preventColumResizing)
            listView.resetRowForegroundColor( row );
    }
}

auto Debugger::removeInstructionBreakpointVisuals(GUIKIT::ListView& listView, unsigned row) -> void {
    listView.setImage( row, 0, nullImg);
    listView.resetRowForegroundColor( row );
}

auto Debugger::updateWatchpointCondition(DbgWatcher& watcher) -> bool {
    unsigned hitCount = watcher.useHitCount ? watcher.hitCount : 0;
    const auto& expression = watcher.useExpression ? watcher.expression : "";
    return emulator->setWatchpointCondition( getTheme(), watcher.action, watcher.addr, hitCount, watcher.hitCountCompare, expression, watcher.expressionCompare );
}

auto Debugger::openConditionView(DbgWatcher* watcher, GUIKIT::Position position) -> void {
    delete conditionViewDebugger;

    conditionViewDebugger = new ConditionViewDebugger(this);
    conditionViewDebugger->create(watcher, position);
    conditionViewDebugger->open();
}
auto Debugger::getCpuTheme() -> DebuggerTheme {
    if (isAmiga())
        return DebuggerTheme::CPU;

    if (getTheme() == DebuggerTheme::SCPU)
        return DebuggerTheme::SCPU;

    if (isDriveCpu())
        return getTheme();

    return DebuggerTheme::CPU;
}

template auto Debugger::updateRegBin<16>(GUIKIT::LineEdit& reg, unsigned val) -> void;
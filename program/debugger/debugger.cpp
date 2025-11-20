
#include "debugger.h"

#include "../../emulation/libami/interface.h"
#include "../program.h"
#include "../thread/emuThread.h"
#include "../../data/icons.h"

// #838589

Debugger::~Debugger() {
    timer.setEnabled( false );
    setVisible(false);
}

Debugger::Debugger( Emulator::Interface* emulator ) {
    this->emulator = emulator;
    this->settings = program->getSettings( emulator );
    build();
}

Debugger::CPU68K::Watcher::Adder::Adder() {
    address.setMaxLength( 8 );
    address.setFont(GUIKIT::Font::system(12));
    append(address, {~0u, 0u}, 10);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

Debugger::CPU68K::Watcher::Watcher() {
    list.setHeaderText( { "", "address", "", ""} );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );

    append(list, {~0u, ~0u}, 5);
    append(adder, {~0u, 0u}, 5);

    append(breakPoint, {0u, 0u}, 3);
    append(watchPoint, {0u, 0u}, 3);
    append(exceptionPoint, {0u, 0u});

    GUIKIT::RadioBox::setGroup( breakPoint, watchPoint, exceptionPoint );
}

Debugger::CPU68K::State::Flags::Flags() {
    int i = 16;

    append(spacer, {0u, 0u}, 10);
    for (auto& f : flag) {
        i--;
        char _f = LIBAMI::CpuSnapshot::flagIdent[i];
        if (_f == ' ')
            continue;
        std::string _t{_f};
        GUIKIT::String::toLowerCase( _t );
        f.setText( _t );
        f.setStore( 0 );
        f.setEnabled( false );
        append(f, {20u, 0u}, 5);
    }
}

Debugger::CPU68K::State::Registers::Registers() {
    static unsigned _w = 0;

    if (_w == 0) {
        GUIKIT::Label test;
        test.setFont( GUIKIT::Font::system( 11 ) );
        test.setText( "U" );
        _w = test.minimumSize().width;
    }

    left.setAlign( GUIKIT::Label::Align::Right );
    right.setAlign( GUIKIT::Label::Align::Right );
    leftVal.setEditable( false );
    rightVal.setEditable( false );
    leftVal.setAlign( GUIKIT::LineEdit::Align::Right );
    rightVal.setAlign( GUIKIT::LineEdit::Align::Right );
    leftVal.setText( "0" );
    leftVal.setStore( 0 );
    rightVal.setText( "0" );
    rightVal.setStore( 0 );

    append(left, {_w * 3, 0u}, 5);
    append(leftVal, {_w * 8, 0u}, 20);
    append(right, {_w * 3, 0u}, 5);
    append(rightVal, {_w * 8, 0u});

    setAlignment( 0.5 );
}

Debugger::CPU68K::State::State::Trace::Trace() {
    append(toggle, {0u, 0u}, 10);
    append(clear, {0u, 0u});
    setAlignment( 0.5 );
}

Debugger::CPU68K::State::State() {
    int i = 0;
    for (auto& reg : registers) {
        i++;
        append(reg, {0u, 0u}, (i == 8 || i == 11) ? 20 : 5);
    }

    append(flags, {0u, 0u}, 10);
    append(trace, {0u, 0u});
}

Debugger::CPU68K::InstructionLayout::InstructionLayout() {
    list.setHeaderText( { "", "address", "data", "instruction" } );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

Debugger::CPU68K::TraceLayout::TraceLayout() {
    list.setHeaderText( { "PC", "Flags","Instruction" } );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

Debugger::CPU68K::CPU68K() {
    switchLayout.setLayout( 0, instructionLayout, {~0u, ~0u} );
    switchLayout.setLayout( 1, traceLayout, {~0u, ~0u} );

    append(switchLayout, {~0u, ~0u}, 20);
    append(watcher, {180u, ~0u}, 10);
    append(state, {0u, 0u});
}

Debugger::Control::Control() {
    stepOver.setEnabled( false );
    stepInto.setEnabled( false );
    line.setEnabled( false );
    frame.setEnabled( false );

    searchEdit.setMaxLength( 8 );
    searchEdit.setFont(GUIKIT::Font::system(12));

    append( position, {0u, 0u}, 10 );
    append( resume, {0u, 0u}, 10 );
    append( stepOver, {0u, 0u}, 10 );
    append( stepInto, {0u, 0u}, 10 );
    append( line, {0u, 0u}, 10 );
    append( frame, {0u, 0u}, 30 );
    append( searchEdit, {120u, 0u}, 10 );
    append( search, {0u, 0u} );

    setAlignment( 0.5 );
}

auto Debugger::build() -> void {
    setWidgetFont( GUIKIT::Font::system( 11 ) );

    GUIKIT::Geometry defaultGeometry = {50, 50, GUIKIT::Font::scale(1024), GUIKIT::Font::scale(570)};

    GUIKIT::Geometry geometry = {settings->get<int>("debugger_x", defaultGeometry.x)
        ,settings->get<int>("debugger_y", defaultGeometry.y)
        ,settings->get<unsigned>("debugger_width", defaultGeometry.width)
        ,settings->get<unsigned>("debugger_height", defaultGeometry.height)
    };

    setGeometry( geometry );

    addImg.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    breakEnableImg.loadPng((uint8_t*)Icons::ledRedRound, sizeof(Icons::ledRedRound));
    breakDisableImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    searchImg.loadPng((uint8_t*)Icons::search, sizeof(Icons::search));
    trashImg.loadPng((uint8_t*)Icons::trash, sizeof(Icons::trash));

    pauseImg.loadPng((uint8_t*)Icons::pause, sizeof(Icons::pause));
    resumeImg.loadPng((uint8_t*)Icons::resume, sizeof(Icons::resume));
    stepIntoImg.loadPng((uint8_t*)Icons::stepInto, sizeof(Icons::stepInto));
    stepOverImg.loadPng((uint8_t*)Icons::stepOver, sizeof(Icons::stepOver));

    lineImg.loadPng((uint8_t*)Icons::line, sizeof(Icons::line));
    frameImg.loadPng((uint8_t*)Icons::frame, sizeof(Icons::frame));

    memoryImg.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    exceptionImg.loadPng((uint8_t*)Icons::exception, sizeof(Icons::exception));
    clearImg.loadPng((uint8_t*)Icons::clear, sizeof(Icons::clear));

    cpu68k.watcher.adder.add.setImage( &addImg );

    if (isOffscreen())
        setGeometry( defaultGeometry );

    control.resume.setImage( &pauseImg );
    control.stepOver.setImage( &stepOverImg );
    control.stepInto.setImage( &stepIntoImg );
    control.search.setImage( &searchImg );
    control.line.setImage( &lineImg );
    control.frame.setImage( &frameImg );

    cpu68k.state.trace.clear.setImage( &clearImg );

    layout.setMargin( 10 );
    layout.append( cpu68k, {~0u, ~0u}, 10 );
    layout.append( control, {~0u, 0u} );

    append( layout );

    cpu68k.instructionLayout.list.onClick = [this](unsigned row, unsigned column) {
        if (column == 0) {
            emuThread->lock();
            auto& inst = instructions[row];
            Watcher* watcher = findWatcherBy(inst.addr, DebuggerAction::Breakpoint);
            if (!watcher) {
                addToWatcherList( inst.addr, DebuggerAction::Breakpoint );
                watcher = findWatcherBy(inst.addr, DebuggerAction::Breakpoint);
                emulator->debuggerAdd(DebuggerAction::Breakpoint, inst.addr);
            } else {
                watcher->enabled ^= 1;
                emulator->debuggerEnable(DebuggerAction::Breakpoint, inst.addr, watcher->enabled);
            }
            updateWatcherList();
            update();
            emuThread->unlock();
            enableInstructionBreakpoint(row, watcher->enabled);
        }
    };

    onClose = [this]() {
        timer.setEnabled( false );
        emuThread->lock();
        emulator->debuggerDisableAll();
        setVisible(false);
        emuThread->unlock();
        program->isPause &= ~2;
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>("debugger_x", geometry.x);
        settings->set<int>("debugger_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>("debugger_width", geometry.width);
        settings->set<unsigned>("debugger_height", geometry.height);
    };

    cpu68k.watcher.list.onClick = [this](unsigned row, unsigned column) {
        if (row >= watchers.size())
            return;

        if (column != 0 && column != 3)
            return;

        if (row >= watchers.size())
            return;

        auto& watcher = watchers[row];
        std::optional<unsigned> instRow = std::nullopt;

        if (watcher.action == DebuggerAction::Breakpoint)
            instRow = findInstructionRowBy(watcher.addr);

        emuThread->lock();

        if (column == 0) {
            watcher.enabled ^= 1;
            enableWatcher(row, watcher.enabled);
            emulator->debuggerEnable( watcher.action, watcher.addr, watcher.enabled );
            if (instRow.has_value())
                enableInstructionBreakpoint(instRow.value(), watcher.enabled);
        } else {
            emulator->debuggerRemove(watcher.action, watcher.addr);
            if (instRow.has_value())
                removeInstructionBreakpoint(instRow.value());

            removeFromWatcherList(watcher.addr, watcher.action);
            updateWatcherList();
        }

        emuThread->unlock();
    };

    cpu68k.watcher.adder.add.onActivate = [this]() {
        cpu68k.watcher.adder.address.onReturn();
    };

    cpu68k.watcher.adder.address.onReturn = [this]() {
        std::string addressText = cpu68k.watcher.adder.address.text();
        if (addressText.empty())
            return;
        GUIKIT::String::remove( addressText, {"$", "0x"} );

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        DebuggerAction action = DebuggerAction::Breakpoint;
        if (cpu68k.watcher.watchPoint.checked())
            action = DebuggerAction::Watchpoint;
        else if (cpu68k.watcher.exceptionPoint.checked())
            action = DebuggerAction::ExceptionPoint;

        if (findWatcherBy( address, action ))
            return;

        emuThread->lock();
        addToWatcherList( static_cast<unsigned>(address), action );
        updateWatcherList();

        if (action == DebuggerAction::Breakpoint) {
            auto instRow = findInstructionRowBy(static_cast<unsigned>(address));
            if (instRow.has_value())
                enableInstructionBreakpoint(instRow.value(), true);
        }

        emulator->debuggerAdd(action, address);
        emuThread->unlock();
    };

    control.stepInto.onActivate = [this]() {
        emuThread->lock();
        program->isPause &= ~2;
        updateToolboxVisibility();
        emulator->debuggerStepInto();
        emuThread->unlock();
    };

    control.stepOver.onActivate = [this]() {
        emuThread->lock();
        program->isPause &= ~2;
        timer.setEnabled(  );
        updateToolboxVisibility();
        emulator->debuggerStepOver();
        emuThread->unlock();
    };

    control.line.onActivate = [this]() {
        emuThread->lock();
        program->isPause &= ~2;
        updateToolboxVisibility();
        emulator->debuggerAdd( DebuggerAction::Line, 0 );
        emuThread->unlock();
    };

    control.frame.onActivate = [this]() {
        emuThread->lock();
        program->isPause &= ~2;
        updateToolboxVisibility();
        emulator->debuggerAdd( DebuggerAction::Frame, 0 );
        emuThread->unlock();
    };

    control.resume.onActivate = [this]() {
        program->isPause ^= 2;
        timer.setEnabled( (program->isPause & 2) == 0 );
        update();
        updateToolboxVisibility();
    };

    control.search.onClick = [this]() {
        std::string addressText = control.searchEdit.text();
        if (addressText.empty())
            return;
        GUIKIT::String::remove( addressText, {"$", "0x"} );

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        auto instRow = findInstructionRowBy(static_cast<unsigned>(address));
        if (instRow.has_value())
            cpu68k.instructionLayout.list.setSelection( instRow.value() );
        else {
            emuThread->lock();
            cacheInstructions(address);
            emuThread->unlock();
            updateInstructionList();
        }
    };

    control.searchEdit.onReturn = [this]() {
        control.search.onClick();
    };

    cpu68k.state.trace.toggle.onToggle = [this]() {
        if (cpu68k.switchLayout.selection() == 0) {
            updateTraceList();
            cpu68k.switchLayout.setSelection( 1 );
        } else
            cpu68k.switchLayout.setSelection( 0 );
    };

    cpu68k.state.trace.clear.onActivate = [this]() {
        emuThread->lock();
        emulator->debuggerDisable( Emulator::Interface::DebuggerAction::History, 0 );
        emulator->debuggerEnable( Emulator::Interface::DebuggerAction::History, 0 );
        updateTraceList();
        emuThread->unlock();
    };

    setTitle( emulator->ident + " Debugger" );

    translate();
}

auto Debugger::translate() -> void {
    cpu68k.watcher.adder.address.setPlaceholder( trans->getA( "address/vector" ) );
    control.searchEdit.setPlaceholder( trans->getA( "address" ) );
    cpu68k.watcher.breakPoint.setText( trans->getA( "Instruktion" ) );
    cpu68k.watcher.watchPoint.setText( trans->getA( "Speicherzugriff" ) );
    cpu68k.watcher.exceptionPoint.setText( trans->getA( "Ausnahme" ) );

    int i = 0;
    for (auto& reg : cpu68k.state.registers) {
        if (i < 8) {
            reg.left.setText( "D" + std::to_string( i ) );
            reg.right.setText( "A" + std::to_string( i ) );
        } else if (i == 8) {
            reg.left.setText("PC");
            reg.right.setText("IRD");
        } else if (i == 9) {
            reg.left.setText("SSP");
            reg.right.setText("IRC");
        } else if (i == 10) {
            reg.left.setText("USP");
            reg.right.setText("IPL");
        }

        i++;
    }

    cpu68k.state.trace.toggle.setText( "Trace" );
}

auto Debugger::update() -> void {
    bool locked = emuThread->lock();
    unsigned addr;

    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        LIBAMI::Interface* amiEmu = dynamic_cast<LIBAMI::Interface*>(emulator);
        auto cpuSnapshot = amiEmu->getCpuSnapshot();
        addr = cpuSnapshot.pcOpEdge;
        update68k(cpuSnapshot);
        updateAgnus(amiEmu);
    } else {
        addr = 0;
    }

    std::optional<unsigned> instRow = std::nullopt;

    if (!last.maybeModified)
        instRow = findInstructionRowBy(addr);

    if (instRow.has_value()) {
        if (locked)
            emuThread->unlock();
        cpu68k.instructionLayout.list.setSelection( instRow.value() );
    } else {
        cacheInstructions(addr);
        if (locked)
            emuThread->unlock();
        updateInstructionList();
    }

    if (cpu68k.switchLayout.selection() == 1)
        updateTraceList();
}

auto Debugger::updateAgnus(LIBAMI::Interface* amiEmu) -> void {
    auto s = amiEmu->getAgnusSnapshot();
    setTitle( emulator->ident + " Debugger V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
}

auto Debugger::update68k(LIBAMI::CpuSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu68k.state.registers) {
        if (i < 8) {
            unsigned val = s.regsD[i];
            if ((unsigned)reg.leftVal.getStore() != val) {
                reg.leftVal.setStore( static_cast<int>(val) );
                reg.leftVal.setText(  hex( val ) );
            }
            val = s.regsA[i];
            if ((unsigned)reg.rightVal.getStore() != val) {
                reg.rightVal.setStore( static_cast<int>(val) );
                reg.rightVal.setText(  hex( val ) );
            }
        } else if (i == 8) {
            if ((unsigned)reg.leftVal.getStore() != s.pc) {
                reg.leftVal.setStore( static_cast<int>(s.pc) );
                reg.leftVal.setText(  hex( s.pc ) );
            }
            if ((uint16_t)reg.rightVal.getStore() != s.ird) {
                reg.rightVal.setStore( static_cast<int>(s.ird) );
                reg.rightVal.setText(  hex( s.ird ) );
            }
        } else if (i == 9) {
            if ((unsigned)reg.leftVal.getStore() != s.ssp) {
                reg.leftVal.setStore( static_cast<int>(s.ssp) );
                reg.leftVal.setText(  hex( s.ssp ) );
            }
            if ((uint16_t)reg.rightVal.getStore() != s.irc) {
                reg.rightVal.setStore( static_cast<int>(s.irc) );
                reg.rightVal.setText(  hex( s.irc ) );
            }
        } else if (i == 10) {
            if ((unsigned)reg.leftVal.getStore() != s.usp) {
                reg.leftVal.setStore( static_cast<int>(s.usp) );
                reg.leftVal.setText(  hex( s.usp ) );
            }
            if ((uint8_t)reg.rightVal.getStore() != s.ipl) {
                reg.rightVal.setStore( static_cast<int>(s.ipl) );
                reg.rightVal.setText(  hex( s.ipl ) );
            }
        }
        i++;
    }

    i = 16;
    for (auto& f : cpu68k.state.flags.flag) {
        i--;
        char _f = LIBAMI::CpuSnapshot::flagIdent[i];

        if (_f == ' ')
            continue;

        bool state = s.flags & (1 << i);
        if ((bool)f.getStore() != state) {
            f.setStore( static_cast<int>(state) );
            std::string _t{_f};

            if (state) {
                f.setEnabled(  );
                f.setFont( GUIKIT::Font::system( 11, "bold" ) );
            } else {
                f.setEnabled( false );
                f.setFont( GUIKIT::Font::system( 11 ) );
                GUIKIT::String::toLowerCase( _t );
            }

            f.setText( _t );
        }
    }
}

auto Debugger::cacheInstructions(unsigned addr) -> void {
    unsigned bytes = 0;
    unsigned _addr = addr;

    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];

        inst.addr = addr;
        inst.disassembled = emulator->disassemble( addr, bytes );
        inst.data = emulator->disassembleData( addr, bytes );

        addr += bytes;
    }

    emulator->debuggerAdd( Emulator::Interface::DebuggerAction::ModifiedCode, _addr, addr );
}

auto Debugger::updateInstructionList() -> void {
    auto& instructionList = cpu68k.instructionLayout.list;
    instructionList.lockRedraw();
    instructionList.reset();

    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];

        auto parts = GUIKIT::String::split( inst.data, '|' );
        instructionList.resetRowForegroundColor( i );
        instructionList.append({ "",parts[0], parts.size() < 2 ? "" : parts[1], inst.disassembled }, true);

         auto breakPoint = findWatcherBy( inst.addr, DebuggerAction::Breakpoint );
         if (!breakPoint)
             instructionList.setImage( i, 0, nullImg, true );
         else if (breakPoint->enabled) {
             instructionList.setImage( i, 0, breakEnableImg, true );
             instructionList.setRowForegroundColor( i, DEBUG_COLOR );
         } else
             instructionList.setImage( i, 0, breakDisableImg, true );
        
    }
    instructionList.autoSizeColumns();
    instructionList.setSelection( 0 );

    instructionList.unlockRedraw();
}

auto Debugger::updateTraceList() -> void {
    auto& traceList = cpu68k.traceLayout.list;
    traceList.lockRedraw();
    traceList.reset();

    for (int i = 0; i < 512; i++) {
        uint16_t flags;
        std::string str;
        std::string result = emulator->disassembleTrace( i, flags );
        if (result.empty())
            break;
        auto parts = GUIKIT::String::split( result, '|' );

        for (int f = 15; f >= 0; f--) {
            bool state = flags & (1 << f);
            char _f = LIBAMI::CpuSnapshot::flagIdent[f];

            if (_f == ' ')
                continue;

            std::string _t{_f};

            if (state)
                str.append( _t );
            else
                str.append( "-" );
        }

        traceList.append({ parts[0], str, parts[1] }, true);
    }

    traceList.autoSizeColumns();
    traceList.setSelection( 0 );

    traceList.unlockRedraw();
}

auto Debugger::addToWatcherList(unsigned addr, DebuggerAction action) -> void {
    watchers.push_back( {addr, action, true} );

    std::sort(watchers.begin(), watchers.end(), [](Watcher& a, Watcher& b) -> bool {
        if (a.action < b.action)
            return true;
        if (a.action > b.action)
            return false;

        return a.addr < b.addr;
    });
}

auto Debugger::removeFromWatcherList(unsigned addr, DebuggerAction action) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->addr == addr && it->action == action) {
            watchers.erase(it);
            break;
        }
        ++it;
    }
}

auto Debugger::findWatcherBy(unsigned addr, DebuggerAction action) -> Watcher* {
    for (auto& watcher : watchers) {
        if (watcher.addr == addr && watcher.action == action)
            return &watcher;
    }
    return nullptr;
}

auto Debugger::findWatcherRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned> {
    for (unsigned i = 0; i < watchers.size(); i++) {
        Watcher& watcher = watchers[i];
        if (watcher.addr == addr && watcher.action == action)
            return i;
    }
    return std::nullopt;
}

auto Debugger::findInstructionRowBy(unsigned addr) -> std::optional<unsigned> {
    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];
        if (inst.addr == addr)
            return i;
    }
    return std::nullopt;
}

auto Debugger::updateWatcherList() -> void {
    auto& addrList = cpu68k.watcher.list;
    addrList.lockRedraw();
    addrList.reset();
    char hex[7];

    for (auto& w : watchers) {
        snprintf(hex, 7, "%06x", w.addr);
        addrList.append( {"", std::string(hex), "", ""}, true );
        unsigned row = addrList.rowCount() - 1;

        addrList.setImage( row, 0, w.enabled ? breakEnableImg : breakDisableImg, true );
        if (w.action == DebuggerAction::Watchpoint)
            addrList.setImage( row, 2, memoryImg, true );
        else if (w.action == DebuggerAction::ExceptionPoint)
            addrList.setImage( row, 2, exceptionImg, true );

        addrList.setImage( row, 3, trashImg, true );
    }

    addrList.autoSizeColumns();
    addrList.unlockRedraw();
}

auto Debugger::enableInstructionBreakpoint(unsigned row, bool state) -> void {
    auto& instructionList = cpu68k.instructionLayout.list;

    if (state) {
        instructionList.setImage( row, 0, breakEnableImg);
        instructionList.setRowForegroundColor( row, DEBUG_COLOR );
    } else {
        instructionList.setImage( row, 0, breakDisableImg);
        instructionList.resetRowForegroundColor( row );
    }
}

auto Debugger::removeInstructionBreakpoint(unsigned row) -> void {
    auto& instructionList = cpu68k.instructionLayout.list;
    instructionList.setImage( row, 0, nullImg);
    instructionList.resetRowForegroundColor( row );
}

auto Debugger::enableWatcher(unsigned row, bool state) -> void {
    auto& addrList = cpu68k.watcher.list;
    addrList.setImage( row, 0, state ? breakEnableImg : breakDisableImg );
}

auto Debugger::updateToolboxVisibility() -> void {
    if (program->isPause & 2) {
        if (control.resume.image() == &pauseImg) {
            control.resume.setImage( &resumeImg );
            control.stepOver.setEnabled( );
            control.stepInto.setEnabled( );
            control.line.setEnabled( );
            control.frame.setEnabled( );
        }
    } else {
        if (control.resume.image() == &resumeImg) {
            control.resume.setImage( &pauseImg );
            control.stepOver.setEnabled( false );
            control.stepInto.setEnabled( false );
            control.line.setEnabled( false );
            control.frame.setEnabled( false );
        }
    }
}

auto Debugger::debugCallback(Emulator::Interface::DebuggerAction action, unsigned addr, bool maybeModified) -> void {
    last.action = action;
    last.addr = addr;
    last.maybeModified = maybeModified;

    if (emuThread->enabled)
        emuThread->events |= EmuThread::EVT_DEBUGGER;
    else
        debugCallback();
}

auto Debugger::debugCallback() -> void {
    timer.setEnabled( false );
    update();
    updateToolboxVisibility();

    auto& watcherList = cpu68k.watcher.list;
    if (last.action == DebuggerAction::Watchpoint || last.action == DebuggerAction::ExceptionPoint) {
        auto row = findWatcherRowBy(last.addr, last.action);
        if (row.has_value())
            watcherList.setSelection( row.value() );
        else if (watcherList.selected())
            watcherList.setSelected( false );
    } else if (watcherList.selected())
        watcherList.setSelected( false );
}

auto Debugger::makeVisible() -> void {
    Window::setVisible(  );
    reset();
}

auto Debugger::reset() -> void {
    timer.setEnabled( false );
    if (!visible())
        return;

    if (emulator == activeEmulator) {
        bool locked = emuThread->lock();
        emulator->debuggerEnable( Emulator::Interface::DebuggerAction::History, 0 );
        for (auto& watcher : watchers)
            emulator->debuggerEnable( watcher.action, watcher.addr, watcher.enabled );

        last.maybeModified = true;
        update();
        program->isPause &= ~2;
        updateToolboxVisibility();

        timer.setInterval( 50 );
        timer.setEnabled( );
        timer.onFinished = [this]() {
            if (timer.enabled()) {
                update();
                timer.setEnabled( );
            }
        };
        if (locked)
            emuThread->unlock();
    }
}

auto Debugger::hex( uint32_t val, int length ) -> std::string {
    char hex[9];
    if (length == -1)
        snprintf(hex, 9, "%x", val);
    else {
        std::string format = "%0" + std::to_string(length) + "x";
        snprintf(hex, 9, format.c_str(), val);
    }
    std::string result = static_cast<std::string>(hex);
    GUIKIT::String::toUpperCase( result );
    return result;
}

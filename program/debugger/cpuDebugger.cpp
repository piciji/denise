
#include "cpuDebugger.h"
#include "memDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"

CpuDebugger::CpuDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

CpuDebugger::~CpuDebugger() {
    if (c64RdyControl) {
        if (control)
            control->remove( *c64RdyControl );
        delete c64RdyControl;
    }
}

CpuDebugger::CPU::WatcherLayout::MemoryAccessLayout::MemoryAccessLayout() {
    append(watchPoint, {0u, 0u}, 10);
    append(writeCheck, {0u, 0u});
    setAlignment( 0.5 );
}

CpuDebugger::CPU::WatcherLayout::Adder::Adder() {
    address.setMaxLength( 8 );
    append(address, {~0u, 0u}, 10);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

CpuDebugger::CPU::WatcherLayout::ExcAdder::ExcAdder()
: exceptionCombo(true) {
    append(exceptionCombo, {~0u, 0u}, 10);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

CpuDebugger::CPU::WatcherLayout::WatcherLayout() {
    list.setHeaderText( { "", "", "", ""} );

    append(list, {~0u, ~0u}, 5);
    append(breakPoint, {0u, 0u}, 3);
    append(memoryAccessLayout, {0u, 0u}, 3);
    append(adder, {~0u, 0u}, 5);

    append(excAdder, {~0u, 0u});

    GUIKIT::RadioBox::setGroup( breakPoint, memoryAccessLayout.watchPoint );
}

CpuDebugger::CPU::State::Flags::Flags(Debugger* debugger) {
    int i;
    const char* ident;

    if (debugger->isAmiga()) {
        i = sizeof(LIBAMI::DebuggerSnapshot::flagIdent);
        ident = &LIBAMI::DebuggerSnapshot::flagIdent[0];
    } else if (debugger->getTheme() == DebuggerTheme::SCPU) {
        i = sizeof(LIBC64::DebuggerSnapshot::flagIdent65816);
        ident = &LIBC64::DebuggerSnapshot::flagIdent65816[0];
    } else {
        i = sizeof(LIBC64::DebuggerSnapshot::flagIdent);
        ident = &LIBC64::DebuggerSnapshot::flagIdent[0];
    }

    flag.resize( i );

    append(spacer, {0u, 0u}, 10);
    for (auto& f : flag) {
        f = new GUIKIT::Label;
        i--;
        char _f = ident[i];
        if (_f == ' ')
            continue;
        std::string _t{_f};
        GUIKIT::String::toLowerCase( _t );
        f->setText( _t );
        f->setStore( 0 );
        f->setFont( GUIKIT::Font::system( 11 ) );
        f->setEnabled( false );
        append(*f, {20u, 0u}, 5);
    }
    setAlignment(0.5);
}

CpuDebugger::CPU::State::Registers::Registers(Debugger* debugger) {
    left.setFont( GUIKIT::Font::monospace(  ) );
    leftVal.setFont( GUIKIT::Font::monospace(  ) );
    right.setFont( GUIKIT::Font::monospace() );
    rightVal.setFont( GUIKIT::Font::monospace() );

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

    append(left, {getWidth(4, false), 0u}, 5);
    append(leftVal, {getWidth(debugger->isAmiga() ? 8 : 4, true), 0u}, 10);
    append(right, {getWidth(4, false), 0u}, 5);
    append(rightVal, {getWidth(debugger->isAmiga() ? 8 : 4, true), 0u});

    setAlignment( 0.5 );
}

CpuDebugger::CPU::State::State::Trace::Trace() {
    append(toggle, {0u, 0u}, 10);
    append(clear, {0u, 0u});
    setAlignment( 0.5 );
}

CpuDebugger::CPU::State::Options::Address::Address(Debugger* debugger) {
    edit.setMaxLength( debugger->isAmiga() ? 6 : 4 );
    append(edit, {80u, 0u}, 5);
    append(view, {0u, 0u});
    setAlignment( 0.5 );
}

CpuDebugger::CPU::State::Options::Value::Value(Debugger* debugger) {
    append(edit, {80u, 0u}, 5);
    append(view, {0u, 0u});
    setAlignment( 0.5 );
}

CpuDebugger::CPU::State::Options::Options(Debugger* debugger)
: address( debugger ), value( debugger ) {
    append( address, {0u, 0u}, 10 );
    append( value, {0u, 0u}, 10 );
}

CpuDebugger::CPU::State::State(Debugger* debugger)
: flags(debugger), options( debugger ) {
    int i = 0;
    registers.resize( debugger->isAmiga() ? 11 : (debugger->isDriveCpu() ? 3 : 4 ));

    for (auto& reg : registers) {
        reg = new Registers(debugger);
        i++;

        if (debugger->isAmiga())
            append(*reg, {0u, 0u}, (i == 8 || i == 11) ? 20 : 5);
        else
            append(*reg, {0u, 0u}, (i == 4) ? 20 : 5);
    }

    append(flags, {0u, 0u}, 20);
    append(trace, {0u, 0u}, 20);
    append(options, {0u, 0u});
}

CpuDebugger::CPU::InstructionLayout::InstructionLayout() {
    list.setHeaderText( { "", "address", "data", "instruction" } );
    list.setFont( GUIKIT::Font::monospace() );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

CpuDebugger::CPU::TraceLayout::TraceLayout() {
    list.setHeaderText( { "address", "status","instruction" } );
    list.setFont( GUIKIT::Font::monospace() );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

CpuDebugger::CPU::CPU(Debugger* debugger)
: state(debugger) {
    switchLayout.setLayout( 0, instructionLayout, {~0u, ~0u} );
    switchLayout.setLayout( 1, traceLayout, {~0u, ~0u} );

    append(switchLayout, {~0u, ~0u}, 20);
    append(watcher, {200u, ~0u}, 10);
    append(state, {0u, 0u});
}

CpuDebugger::C64RdyControl::C64RdyControl() {
    append( rdyButton, {0u, 0u} );
    setAlignment( 0.5 );
}

auto CpuDebugger::buildTheme() -> GUIKIT::Layout* {
    cpu = new CPU(this);
    cpu->watcher.adder.add.setImage( &addImg );
    cpu->watcher.excAdder.add.setImage( &addImg );
    cpu->state.trace.clear.setImage( &clearImg );

    watcherHelper.debugger = this;
    watcherHelper.watcherList = &cpu->watcher.list;

    if (isAmiga()) {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBAMI::DebuggerSnapshot::exceptions)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else if (getTheme() == DebuggerTheme::SCPU) {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBC64::DebuggerSnapshot::exceptions65816)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBC64::DebuggerSnapshot::exceptions)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    }

    cpu->instructionLayout.list.onClick = [this](unsigned row, unsigned column) {
        auto& instructionList = cpu->instructionLayout.list;

        if (column == 0) {
            emuThread->lock();
            auto& inst = instructions[row];
            DbgWatcher* watcher = watcherHelper.findBy(inst.addr, DebuggerAction::Breakpoint);
            if (!watcher) {
                watcherHelper.addToList( inst.addr, DebuggerAction::Breakpoint );
                watcher = watcherHelper.findBy(inst.addr, DebuggerAction::Breakpoint);
                emulator->debuggerAdd(getTheme(), DebuggerAction::Breakpoint, inst.addr);
            } else {
                watcher->enabled ^= 1;
                if (watcher->enabled) {
                    emulator->debuggerAdd(getTheme(), DebuggerAction::Breakpoint, inst.addr);
                    updateWatchpointCondition( *watcher );
                } else
                    emulator->debuggerRemove(getTheme(), DebuggerAction::Breakpoint, inst.addr);
            }
            watcherHelper.updateList();
            emuThread->unlock();
            updateInstructionBreakpointVisuals(instructionList, row, watcher);
            instructionList.setSelection( currentInstRow.has_value() ? currentInstRow.value_or(0) : 0 );
        } else if (isPaused()) {
            auto& inst = instructions[row];
            cpu->state.options.address.edit.setText( GUIKIT::String::convertToHex( inst.addr ) );
        }
    };

    cpu->instructionLayout.list.onContext = [this](unsigned row, unsigned column, GUIKIT::Position position ) {
        if (column == 0) {
            auto& inst = instructions[row];
            DbgWatcher* watcher = watcherHelper.findBy(inst.addr, DebuggerAction::Breakpoint);
            if (watcher) {
                openConditionView(watcher, position);
            }

            cpu->instructionLayout.list.setSelection( currentInstRow.has_value() ? currentInstRow.value_or(0) : 0 );
        }
    };

    cpu->watcher.list.onContext = [this](unsigned row, unsigned column, GUIKIT::Position position ) {
        if (column == 0) {
            if (row >= watcherHelper.elements())
                return;

            auto& watcher = watcherHelper.getWatcher(row);
            openConditionView(&watcher, position);
        }
    };


    cpu->watcher.list.onClick = [this](unsigned row, unsigned column) {
        if (row >= watcherHelper.elements())
            return;

        if (column != 0 && column != 3)
            return;

        auto& watcher = watcherHelper.getWatcher(row);
        std::optional<unsigned> instRow = std::nullopt;

        if (watcher.action == DebuggerAction::Breakpoint)
            instRow = findInstructionRowBy(watcher.addr);

        emuThread->lock();

        auto& instructionList = cpu->instructionLayout.list;

        if (column == 0) {
            watcher.enabled ^= 1;
            watcherHelper.updateBreakpointVisuals(row, &watcher);
            if (watcher.enabled) {
                emulator->debuggerAdd(getTheme(), watcher.action, watcher.addr);
                updateWatchpointCondition( watcher );
            } else
                emulator->debuggerRemove(getTheme(), watcher.action, watcher.addr);

            if (instRow.has_value())
                updateInstructionBreakpointVisuals(instructionList, instRow.value_or(0), &watcher);
        } else if (column == 3) {
            emulator->debuggerRemove( getTheme(), watcher.action, watcher.addr);
            if (instRow.has_value())
                removeInstructionBreakpointVisuals(instructionList, instRow.value_or(0));

            watcherHelper.removeFromList(watcher.addr, watcher.action);
            watcherHelper.updateList();
        }

        emuThread->unlock();
    };

    cpu->watcher.excAdder.add.onActivate = [this]() {
        unsigned vector = (int)cpu->watcher.excAdder.exceptionCombo.userData();

        DebuggerAction action = DebuggerAction::ExceptionPoint;

        if (watcherHelper.findBy( vector, action ))
            return;

        emuThread->lock();
        watcherHelper.addToList( vector, action, cpu->watcher.excAdder.exceptionCombo.text() );
        watcherHelper.updateList();
        emulator->debuggerAdd(getTheme(), action, vector);
        emuThread->unlock();
    };

    cpu->watcher.adder.add.onActivate = [this]() {
        cpu->watcher.adder.address.onReturn();
    };

    cpu->watcher.adder.address.onReturn = [this]() {
        auto& instructionList = cpu->instructionLayout.list;

        std::string addressText = cpu->watcher.adder.address.text();
        if (addressText.empty())
            return;

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        DebuggerAction action = DebuggerAction::Breakpoint;
        if (cpu->watcher.memoryAccessLayout.watchPoint.checked()) {
            action = DebuggerAction::Watchpoint;
            if (cpu->watcher.memoryAccessLayout.writeCheck.checked()) {
                action = DebuggerAction::WatchpointWrite;
            }
        }

        if (watcherHelper.findBy( address, action ))
            return;

        emuThread->lock();
        watcherHelper.addToList( static_cast<unsigned>(address), action );
        watcherHelper.updateList();

        if (action == DebuggerAction::Breakpoint) {
            auto instRow = findInstructionRowBy(static_cast<unsigned>(address));
            if (instRow.has_value())
                updateInstructionBreakpointVisuals(instructionList, instRow.value_or(0), watcherHelper.findBy( address, action ));
        }

        emulator->debuggerAdd(getTheme(), action, address);
        emuThread->unlock();
    };

    cpu->state.trace.toggle.onToggle = [this]() {
        if (cpu->switchLayout.selection() == 0) {
            emuThread->lock();
            fetchTraces();
            updateTraceList();
            emuThread->unlock();
            cpu->switchLayout.setSelection( 1 );
        } else
            cpu->switchLayout.setSelection( 0 );
    };

    cpu->state.trace.clear.onActivate = [this]() {
        emuThread->lock();
        emulator->debuggerRemove( getTheme(), DebuggerAction::History, 0 );
        emulator->debuggerAdd( getTheme(), DebuggerAction::History, 0 );
        fetchTraces();
        updateTraceList();
        emuThread->unlock();
    };

    cpu->state.options.address.view.setImage( &searchImg );
    cpu->state.options.value.view.setImage( &editImg );

    cpu->state.options.address.edit.onReturn = [this]() {
        cpu->state.options.address.view.onClick();
    };

    cpu->state.options.value.edit.onReturn = [this]() {
        cpu->state.options.value.view.onClick();
    };

    cpu->state.options.address.view.onClick = [this]() {
        if (emulator != activeEmulator)
            return;

        std::string addressText = cpu->state.options.address.edit.text();
        if (addressText.empty())
            return;

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        searchAddress(address);
    };

    cpu->state.options.value.view.onClick = [this]() {
        auto valStr = cpu->state.options.value.edit.text();
        auto addrStr = cpu->state.options.address.edit.text();

        changeMemory( addrStr, valStr );
    };

    return cpu;
}

auto CpuDebugger::memChanged() -> void {
    prepareTheme(true);
    updateTheme();
}

auto CpuDebugger::updateCpuFlags(const char* flagIdent, unsigned flags) -> void {
    int i = cpu->state.flags.flag.size();
    for (auto& f : cpu->state.flags.flag) {
        i--;
        char _f = *(flagIdent + i);

        if (_f == ' ')
            continue;

        bool state = flags & (1 << i);
        if ((bool)f->getStore() != state) {
            f->setStore( static_cast<int>(state) );
            std::string _t{_f};

            if (state) {
                f->setEnabled(  );
                f->setFont( GUIKIT::Font::system( 11, "bold" ) );
            } else {
                f->setEnabled( false );
                f->setFont( GUIKIT::Font::system( 11 ) );
                GUIKIT::String::toLowerCase( _t );
            }

            f->setText( _t );
        }
    }
}

auto CpuDebugger::updateInstructionList() -> void {
    auto& instructionList = cpu->instructionLayout.list;
    instructionList.lockRedraw();
    instructionList.reset();

    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];

        auto parts = GUIKIT::String::split( inst.data, '|' );
        instructionList.resetRowForegroundColor( i );
        instructionList.append({ "",parts[0], parts.size() < 2 ? "" : parts[1], inst.disassembled }, true);

        auto breakPoint = watcherHelper.findBy( inst.addr, DebuggerAction::Breakpoint );
        if (!breakPoint) {
            instructionList.setImage( i, 0, nullImg, true );
        } else {
            updateInstructionBreakpointVisuals(instructionList, i, breakPoint, true);
        }
    }
    instructionList.autoSizeColumns();
    instructionList.setSelection( 0 );

    instructionList.unlockRedraw();
}

auto CpuDebugger::updateTraceList() -> void {
    auto& traceList = cpu->traceLayout.list;
    traceList.lockRedraw();
    traceList.reset();

    unsigned flagSize;
    const char* flagIdent;
    if (isAmiga()) {
        flagSize = sizeof(LIBAMI::DebuggerSnapshot::flagIdent);
        flagIdent = &LIBAMI::DebuggerSnapshot::flagIdent[0];
    } else if (getTheme() == DebuggerTheme::SCPU) {
        flagSize = sizeof(LIBC64::DebuggerSnapshot::flagIdent65816);
        flagIdent = &LIBC64::DebuggerSnapshot::flagIdent65816[0];
    } else {
        flagSize = sizeof(LIBC64::DebuggerSnapshot::flagIdent);
        flagIdent = &LIBC64::DebuggerSnapshot::flagIdent[0];
    }

    for (int i = 0; i < LIST_TRACES; i++) {
        std::string str;
        Trace& trace = traces[i];

        uint16_t flags = trace.flags;
        std::string result = trace.disassembled;
        if (result.empty())
            break;

        auto parts = GUIKIT::String::split( result, '|' );

        for (int f = flagSize - 1; f >= 0; f--) {
            bool state = flags & (1 << f);
            char _f = flagIdent[f];

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

auto CpuDebugger::findInstructionRowBy(unsigned addr) -> std::optional<unsigned> {
    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];
        if (inst.addr == addr)
            return i;
    }
    return std::nullopt;
}

auto CpuDebugger::updateBreakpointVisuals(DbgWatcher* watcher) -> void {
    std::optional<unsigned> instRow = findInstructionRowBy(watcher->addr);

    if (instRow.has_value())
        updateInstructionBreakpointVisuals(cpu->instructionLayout.list, instRow.value_or(0), watcher);

    instRow = watcherHelper.findRowBy( watcher->addr, watcher->action );

    if (instRow.has_value())
        watcherHelper.updateBreakpointVisuals(instRow.value_or(0), watcher);
}

auto CpuDebugger::updateWatcherSelection() -> void {
    auto& watcherList = cpu->watcher.list;
    auto& t = snapshot->callbackTheme;
    auto& act = snapshot->callbackAction;
    bool hiLight = false;

    if ((t == DebuggerTheme::Unspecified) || (t == getTheme())) {
        if (act == DebuggerAction::Watchpoint
        || act == DebuggerAction::WatchpointWrite
        || act == DebuggerAction::Breakpoint
        || act == DebuggerAction::ExceptionPoint) {
            auto row = watcherHelper.findRowBy(snapshot->callbackAddress, act);
            if (row.has_value()) {
                watcherList.setSelection( row.value_or(0) );
                hiLight = true;
            }
        }
    }

    if (!hiLight && watcherList.selected())
        watcherList.setSelected( false );
}

auto CpuDebugger::searchAddress(unsigned addr) -> void {
    auto instRow = findInstructionRowBy(static_cast<unsigned>(addr));
    if (instRow.has_value())
        cpu->instructionLayout.list.setSelection( instRow.value_or(0) );
    else {
        emuThread->lock();
        fetchInstructions(addr);
        emuThread->unlock();
        updateInstructionList();
    }
}

auto CpuDebugger::fetchInstructions(unsigned addr) -> void {
    unsigned bytes = 0;
    unsigned _addr = addr;

    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];

        inst.addr = addr;
        inst.disassembled = emulator->disassemble( getTheme(), addr, bytes );
        inst.data = emulator->disassembleData( getTheme(), addr, bytes );

        addr += bytes;
    }

    emulator->debuggerAdd( getTheme(), DebuggerAction::ModifiedCode, _addr, addr );
}

auto CpuDebugger::fetchTraces() -> void {
    for (int i = 0; i < LIST_TRACES; i++) {
        uint16_t flags;
        Trace& trace = traces[i];
        trace.disassembled = emulator->disassembleTrace( getTheme(), i, flags );
        trace.flags = flags;
    }
}

auto CpuDebugger::prepareTheme(bool external) -> void {
    if (!snapshot)
        return;

    unsigned addr;
    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        addr = snap.pcEdge;
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        if (!isDriveCpu() && !(snap.superCpu && getTheme() == DebuggerTheme::SCPU) && !(!snap.superCpu && getTheme() == DebuggerTheme::CPU) )
            return;

        if (isDriveCpu()) {
            addr = snap.drives[getDriveId()].pcEdge;
        } else
            addr = snap.pcEdge;
    }

    if (cpu->switchLayout.selection() == 1)
        fetchTraces();

    currentInstRow = std::nullopt;

    if (!snapshot->codeMaybeModified)
        currentInstRow = findInstructionRowBy(addr);

    if (!currentInstRow.has_value())
        fetchInstructions(addr);
    else {
        unsigned row = currentInstRow.value_or( 0 );
        unsigned bytes = 2;
        if ((row + 1) < LIST_INSTRUCTIONS) {
            unsigned nextAddr = instructions[row + 1].addr;
            if (nextAddr > addr)
                bytes = nextAddr - addr;
        }

        if (instructions[row].data != emulator->disassembleData( getTheme(), addr, bytes )) {
            currentInstRow = std::nullopt;
            fetchInstructions(addr);
        }
    }
}

auto CpuDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        update68k(snap);

    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);

        if (isDriveCpu())
            update6502( snap );
        else if (!snap.superCpu && getTheme() == DebuggerTheme::CPU)
            update6510( snap );
        else if (snap.superCpu && getTheme() == DebuggerTheme::SCPU)
            update65816( snap );
        else
            return;
    }

    if (cpu->switchLayout.selection() == 1)
        updateTraceList();

    if (currentInstRow.has_value()) {
        cpu->instructionLayout.list.setSelection( currentInstRow.value_or(0) );
    } else {
        updateInstructionList();
    }

    updateWatcherSelection();
}

auto CpuDebugger::initTheme() -> void {
    emulator->debuggerAdd( getTheme(), DebuggerAction::None, 0);
    emulator->debuggerAdd( getTheme(), DebuggerAction::History, 0 );

    for (auto& watcher : watcherHelper.watchers) {
        if (watcher.enabled) {
            emulator->debuggerAdd( getTheme(), watcher.action, watcher.addr );
            updateWatchpointCondition(watcher);
        } else
            emulator->debuggerRemove( getTheme(), watcher.action, watcher.addr );
    }

    // force reload of instruction cache
    emulator->debuggerAdd( getTheme(), DebuggerAction::ModifiedCode, 0, ~0 );
}

auto CpuDebugger::closeTheme() -> void {
    emulator->debuggerRemove( getTheme(), DebuggerAction::None);
    emulator->debuggerRemove( getTheme(), DebuggerAction::Breakpoint );
    emulator->debuggerRemove( getTheme(), DebuggerAction::Watchpoint );
    emulator->debuggerRemove( getTheme(), DebuggerAction::WatchpointWrite );
    emulator->debuggerRemove( getTheme(), DebuggerAction::ExceptionPoint );
    emulator->debuggerRemove( getTheme(), DebuggerAction::History );
    emulator->debuggerRemove( getTheme(), DebuggerAction::ModifiedCode );
}

auto CpuDebugger::update68k(LIBAMI::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i) {
            case 0: case 1: case 2: case 3:
            case 4: case 5: case 6: case 7:
                updateReg(reg->leftVal, s.regsD[i]);
                updateReg(reg->rightVal, s.regsA[i]);
                break;
            case 8:
                updateReg(reg->leftVal, s.pc);
                updateReg(reg->rightVal, s.ird);
                break;
            case 9:
                updateReg(reg->leftVal, s.ssp);
                updateReg(reg->rightVal, s.irc);
                break;
            case 10:
                updateReg(reg->leftVal, s.usp);
                updateReg(reg->rightVal, s.ipl);
                break;
        }
        i++;
    }

    updateControl( s.vPos, s.hPos );
    updateCpuFlags(&LIBAMI::DebuggerSnapshot::flagIdent[0], s.flags);
}

auto CpuDebugger::update6502(LIBC64::DebuggerSnapshot& s) -> void {
    int i = 0;
    auto& _s = s.drives[getDriveId()];

    for (auto& reg : cpu->state.registers) {
        switch (i++) {
            case 0:
                updateReg(reg->leftVal, _s.pc);
                updateReg(reg->rightVal, _s.regS);
                break;
            case 1:
                updateReg(reg->leftVal, _s.regX);
                updateReg(reg->rightVal, _s.regY);
                break;
            case 2:
                updateReg(reg->leftVal, _s.regA);
                break;
            case 3:
                break;
        }
    }

    updateControl( s.vPos, s.hPos );
    updateCpuFlags(&LIBC64::DebuggerSnapshot::flagIdent[0], _s.flags);
}

auto CpuDebugger::update6510(LIBC64::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i++) {
            case 0:
                updateReg(reg->leftVal, s.pc);
                updateReg(reg->rightVal, s.regS);
                break;
            case 1:
                updateReg(reg->leftVal, s.regX);
                updateReg(reg->rightVal, s.regY);
                break;
            case 2:
                updateReg(reg->leftVal, s.regA);
                updateReg(reg->rightVal, s.ioLines);
                break;
            case 3:
                updateReg(reg->leftVal, s.por);
                updateReg(reg->rightVal, s.ddr);
                break;
        }
    }

    updateControl( s.vPos, s.hPos );
    updateCpuFlags(&LIBC64::DebuggerSnapshot::flagIdent[0], s.flags);
}

auto CpuDebugger::update65816(LIBC64::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i++) {
            case 0:
                updateReg(reg->leftVal, s.pc);
                updateReg(reg->rightVal, s.regS);
                break;
            case 1:
                updateReg(reg->leftVal, s.regX);
                updateReg(reg->rightVal, s.regY);
                break;
            case 2:
                updateReg(reg->leftVal, s.regA);
                updateReg(reg->rightVal, s.modeE);
                break;
            case 3:
                updateReg(reg->leftVal, s.pbr);
                updateReg(reg->rightVal, s.dbr);
                break;
        }
    }

    updateControl( s.vPos, s.hPos );
    updateCpuFlags(&LIBC64::DebuggerSnapshot::flagIdent65816[0], s.flags);
}

auto CpuDebugger::translateTheme() -> void {
    bool showTips = showTipsItem.checked();

    cpu->watcher.adder.address.setPlaceholder( trans->getA( "address" ) );
    cpu->watcher.breakPoint.setText( trans->getA( "instruction" ) );
    cpu->watcher.memoryAccessLayout.watchPoint.setText( trans->getA( "memory access" ) );
    cpu->watcher.memoryAccessLayout.writeCheck.setText( trans->getA( "write" ) );

    int i = 0;
    if (isAmiga()) {
        for (auto& reg : cpu->state.registers) {
            if (i < 8) {
                reg->left.setText( "D" + std::to_string( i ) );
                reg->right.setText( "A" + std::to_string( i ) );
            } else if (i == 8) {
                reg->left.setText("PC");
                reg->right.setText("IRD");
            } else if (i == 9) {
                reg->left.setText("SSP");
                reg->right.setText("IRC");
            } else if (i == 10) {
                reg->left.setText("USP");
                reg->right.setText("IPL");
            }

            i++;
        }
    } else {
        for (auto& reg : cpu->state.registers) {
            if (i == 0) {
                reg->left.setText( "PC" );
                reg->right.setText( "S" );
            } else if (i == 1) {
                reg->left.setText( "X" );
                reg->right.setText( "Y" );
            } else if (i == 2) {
                reg->left.setText( "A" );
                if (!isDriveCpu())
                    reg->right.setText( getTheme() == DebuggerTheme::SCPU ? "M-E" : "I/O" );
            } else if (i == 3) {
                if (!isDriveCpu()) {
                    reg->left.setText( getTheme() == DebuggerTheme::SCPU ? "PBR" : "POR" );
                    reg->right.setText( getTheme() == DebuggerTheme::SCPU ? "DBR" : "DDR" );
                }
            }

            i++;
        }
    }

    cpu->instructionLayout.list.setHeaderText( {"", trans->getA( "address"), trans->getA( "data"), trans->getA( "instruction") } );
    cpu->traceLayout.list.setHeaderText( {trans->getA( "address"), trans->getA( "status"), trans->getA( "instruction") } );
    cpu->state.trace.toggle.setText( trans->getA( "trace") );
    cpu->state.trace.toggle.setTooltip( showTips ? trans->getA( "toggle trace") : "" );
    cpu->state.trace.clear.setTooltip( showTips ? trans->getA( "empty trace") : "" );

    cpu->state.options.address.edit.setPlaceholder( trans->getA( "address" )  );
    cpu->state.options.value.edit.setPlaceholder( trans->getA( "value" )  );
    cpu->state.options.value.edit.setTooltip( showTips ? trans->getA( "edit memory tooltip") : "" );
    cpu->watcher.adder.add.setTooltip( showTips ? trans->getA( "complex conditions tooltip") : "" );

    if (c64RdyControl) {
        c64RdyControl->rdyButton.setText("RDY" );
        c64RdyControl->rdyButton.setTooltip( showTips ? trans->getA( "step next rdy" ) : "" );
    }
}

auto CpuDebugger::buildControl() -> GUIKIT::Layout* {
    if (isC64() && !isDriveCpu()) {
        c64RdyControl = new C64RdyControl();

        c64RdyControl->rdyButton.onActivate = [this]() {
            haltCpu(emulator);
        };

        return c64RdyControl;
    }
    return nullptr;
}

auto CpuDebugger::saveIdent() -> std::string {
    return "debugger_cpu";
}

auto CpuDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger CPU";
}

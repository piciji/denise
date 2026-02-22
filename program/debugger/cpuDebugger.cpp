
#include "cpuDebugger.h"
#include "memDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"

CpuDebugger::CpuDebugger( Emulator::Interface* emulator, Mode mode )
: Debugger( emulator, mode ) {}

CpuDebugger::CpuDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Mode::CPU ) {
    build();
}

CpuDebugger::BreakConditionLayout::Expression::Expression() {
    append( check, {0u, 0u}, 10 );
    append( compareCombo, {0u, 0u}, 10 );
    append( compareVal, {~0u, 0u} );

    setAlignment( 0.5 );
}

CpuDebugger::BreakConditionLayout::HitCount::HitCount() {
    append( check, {0u, 0u}, 10 );
    append( compareCombo, {0u, 0u}, 10 );
    append( compareVal, {~0u, 0u} );

    setAlignment( 0.5 );
}

CpuDebugger::BreakConditionLayout::Control::Control() {
    append( spacer, {~0u, 0u} );
    append( closeButton, {0u, 0u} );
}

CpuDebugger::BreakConditionLayout::BreakConditionLayout() {
    info.setEditable( false );
    append( expression, {~0u, 0u}, 10 );
    append( hitCount, {~0u, 0u}, 10 );
    append( info, {~0u, ~0u}, 10 );
    append( control, {~0u, 0u});

    setMargin( 10 );
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
    list.setHeaderText( { "", "address", "", "", ""} );

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
    } else if (debugger->mode == Mode::SCPU) {
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
    left.setFont( GUIKIT::Font::system( 11, "", true ) );
    leftVal.setFont( GUIKIT::Font::system( 11, "", true ) );
    right.setFont( GUIKIT::Font::system( 11, "", true ) );
    rightVal.setFont( GUIKIT::Font::system( 11, "", true ) );

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

    append(left, {getWidth(4, false, true), 0u}, 5);
    append(leftVal, {getWidth(debugger->isAmiga() ? 8 : 4, true, true), 0u}, 10);
    append(right, {getWidth(4, false, true), 0u}, 5);
    append(rightVal, {getWidth(debugger->isAmiga() ? 8 : 4, true, true), 0u});

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
    registers.resize( debugger->isAmiga() ? 11 : 4 );

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
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

CpuDebugger::CPU::TraceLayout::TraceLayout() {
    list.setHeaderText( { "address", "status","instruction" } );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
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

auto CpuDebugger::buildTheme() -> GUIKIT::Layout* {
    cpu = new CPU(this);
    cpu->watcher.adder.add.setImage( &addImg );
    cpu->watcher.excAdder.add.setImage( &addImg );
    cpu->state.trace.clear.setImage( &clearImg );

    if (isAmiga()) {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBAMI::DebuggerSnapshot::exceptions)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else if (mode == Mode::SCPU) {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBC64::DebuggerSnapshot::exceptions65816)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else {
        for (const Emulator::Interface::DebuggerIdent& debuggerException : LIBC64::DebuggerSnapshot::exceptions)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    }

    cpu->instructionLayout.list.onClick = [this](unsigned row, unsigned column) {
        if (column == 0) {
            emuThread->lock();
            auto& inst = instructions[row];
            Watcher* watcher = findWatcherBy(inst.addr, DebuggerAction::Breakpoint);
            if (!watcher) {
                addToWatcherList( inst.addr, DebuggerAction::Breakpoint );
                watcher = findWatcherBy(inst.addr, DebuggerAction::Breakpoint);
                emulator->debuggerAdd(getCpuType(), DebuggerAction::Breakpoint, inst.addr);
            } else {
                watcher->enabled ^= 1;
                if (watcher->enabled) {
                    emulator->debuggerAdd(getCpuType(), DebuggerAction::Breakpoint, inst.addr);
                    updateWatchpointCondition( *watcher );
                } else
                    emulator->debuggerRemove(getCpuType(), DebuggerAction::Breakpoint, inst.addr);
            }
            updateWatcherList();
            emuThread->unlock();
            updateInstructionBreakpointVisuals(row, watcher);
            cpu->instructionLayout.list.setSelection( currentInstRow.has_value() ? currentInstRow.value() : 0 );
        } else if (isPaused()) {
            auto& inst = instructions[row];
            cpu->state.options.address.edit.setText( GUIKIT::String::convertToHex( inst.addr ) );
        }
    };

    cpu->instructionLayout.list.onContext = [this](unsigned row, unsigned column, GUIKIT::Position position ) {
        if (column == 0) {
            auto& inst = instructions[row];
            Watcher* watcher = findWatcherBy(inst.addr, DebuggerAction::Breakpoint);
            if (watcher)
                createWatchpointConditionOverlay(watcher, position);
            cpu->instructionLayout.list.setSelection( currentInstRow.has_value() ? currentInstRow.value() : 0 );
        }
    };

    cpu->watcher.list.onContext = [this](unsigned row, unsigned column, GUIKIT::Position position ) {
        if (column == 0) {
            if (row >= watchers.size())
                return;

            auto& watcher = watchers[row];
            createWatchpointConditionOverlay(&watcher, position);
        }
    };


    cpu->watcher.list.onClick = [this](unsigned row, unsigned column) {
        if (row >= watchers.size())
            return;

        if (column != 0 && column != 4)
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
            updateWatcherBreakpointVisuals(row, &watcher);
            if (watcher.enabled) {
                emulator->debuggerAdd(getCpuType(), watcher.action, watcher.addr);
                updateWatchpointCondition( watcher );
            } else
                emulator->debuggerRemove(getCpuType(), watcher.action, watcher.addr);

            if (instRow.has_value())
                updateInstructionBreakpointVisuals(instRow.value(), &watcher);
        } else {
            emulator->debuggerRemove( getCpuType(), watcher.action, watcher.addr);
            if (instRow.has_value())
                removeInstructionBreakpoint(instRow.value());

            removeFromWatcherList(watcher.addr, watcher.action);
            updateWatcherList();
        }

        emuThread->unlock();
    };

    cpu->watcher.excAdder.add.onActivate = [this]() {
        unsigned vector = (int)cpu->watcher.excAdder.exceptionCombo.userData();

        DebuggerAction action = DebuggerAction::ExceptionPoint;

        if (findWatcherBy( vector, action ))
            return;

        emuThread->lock();
        addToWatcherList( vector, action, cpu->watcher.excAdder.exceptionCombo.text() );
        updateWatcherList();
        emulator->debuggerAdd(getCpuType(), action, vector);
        emuThread->unlock();
    };

    cpu->watcher.adder.add.onActivate = [this]() {
        cpu->watcher.adder.address.onReturn();
    };

    cpu->watcher.adder.address.onReturn = [this]() {
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

        if (findWatcherBy( address, action ))
            return;

        emuThread->lock();
        addToWatcherList( static_cast<unsigned>(address), action );
        updateWatcherList();

        if (action == DebuggerAction::Breakpoint) {
            auto instRow = findInstructionRowBy(static_cast<unsigned>(address));
            if (instRow.has_value())
                updateInstructionBreakpointVisuals(instRow.value(), findWatcherBy( address, action ));
        }

        emulator->debuggerAdd(getCpuType(), action, address);
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
        emulator->debuggerRemove( getCpuType(), DebuggerAction::History, 0 );
        emulator->debuggerAdd( getCpuType(), DebuggerAction::History, 0 );
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
    prepareTheme();
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

        auto breakPoint = findWatcherBy( inst.addr, DebuggerAction::Breakpoint );
        if (!breakPoint) {
            instructionList.setImage( i, 0, nullImg, true );
        } else {
            updateInstructionBreakpointVisuals(i, breakPoint, true);
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
    } else if (mode == Mode::SCPU) {
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

auto CpuDebugger::addToWatcherList(unsigned addr, DebuggerAction action, const std::string& ident) -> void {
    watchers.push_back( {addr, ident, action, true} );

    std::sort(watchers.begin(), watchers.end(), [](Watcher& a, Watcher& b) -> bool {
        if (a.action < b.action)
            return true;
        if (a.action > b.action)
            return false;

        return a.addr < b.addr;
    });
}

auto CpuDebugger::removeFromWatcherList(unsigned addr, DebuggerAction action) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->addr == addr && it->action == action) {
            watchers.erase(it);
            break;
        }
        ++it;
    }
}

auto CpuDebugger::findWatcherBy(unsigned addr, DebuggerAction action) -> Watcher* {
    for (auto& watcher : watchers) {
        if (watcher.addr == addr && watcher.action == action)
            return &watcher;
    }
    return nullptr;
}

auto CpuDebugger::findWatcherRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned> {
    for (unsigned i = 0; i < watchers.size(); i++) {
        Watcher& watcher = watchers[i];
        if (watcher.addr == addr && watcher.action == action)
            return i;
    }
    return std::nullopt;
}

auto CpuDebugger::findInstructionRowBy(unsigned addr) -> std::optional<unsigned> {
    for (unsigned i = 0; i < LIST_INSTRUCTIONS; i++) {
        auto& inst = instructions[i];
        if (inst.addr == addr)
            return i;
    }
    return std::nullopt;
}

auto CpuDebugger::updateWatcherList() -> void {
    auto& watcherList = cpu->watcher.list;
    watcherList.lockRedraw();
    watcherList.reset();
    char hex[7];
    std::string format = "%06x";
    if (isC64())
        format = "%04x";

    for (auto& w : watchers) {
        if (w.ident.empty()) {
            std::string _access = w.action == DebuggerAction::WatchpointWrite ? "W" : "R";
            snprintf(hex, 7, format.c_str(), w.addr);
            watcherList.append( {"", std::string(hex), _access, "", ""}, true );
        } else {
            watcherList.append( {"", w.ident, "R", "", ""}, true );
        }

        unsigned row = watcherList.rowCount() - 1;

        updateWatcherBreakpointVisuals( row, &w, true );

        if (w.action == DebuggerAction::Watchpoint || w.action == DebuggerAction::WatchpointWrite)
            watcherList.setImage( row, 3, memoryImg, true );
        else if (w.action == DebuggerAction::ExceptionPoint)
            watcherList.setImage( row, 3, exceptionImg, true );

        watcherList.setImage( row, 4, trashImg, true );
    }

    watcherList.autoSizeColumns();
    watcherList.unlockRedraw();
}

auto CpuDebugger::updateBreakpointVisuals(Watcher* watcher) -> void {
    std::optional<unsigned> instRow = findInstructionRowBy(watcher->addr);

    if (instRow.has_value())
        updateInstructionBreakpointVisuals(instRow.value(), watcher);

    instRow = findWatcherRowBy( watcher->addr, watcher->action );

    if (instRow.has_value())
        updateWatcherBreakpointVisuals(instRow.value(), watcher);
}

auto CpuDebugger::updateInstructionBreakpointVisuals(unsigned row, Watcher* watcher, bool preventColumResizing) -> void {
    auto& instructionList = cpu->instructionLayout.list;

    if (watcher->enabled) {
        if (watcher->useHitCount || watcher->useExpression)
            instructionList.setImage( row, 0, breakCondEnableImg, preventColumResizing );
        else
            instructionList.setImage( row, 0, breakEnableImg, preventColumResizing );

        instructionList.setRowForegroundColor( DEBUG_COLOR, row );
    } else {
        instructionList.setImage( row, 0, breakDisableImg, preventColumResizing);
        if (!preventColumResizing)
            instructionList.resetRowForegroundColor( row );
    }
}

auto CpuDebugger::updateWatcherBreakpointVisuals(unsigned row, Watcher* watcher, bool preventColumResizing) -> void {
    auto& watcherList = cpu->watcher.list;

    if (watcher->enabled) {
        if (watcher->useHitCount || watcher->useExpression)
            watcherList.setImage( row, 0, breakCondEnableSmallImg, preventColumResizing );
        else
            watcherList.setImage( row, 0, breakEnableSmallImg, preventColumResizing );

    } else {
        watcherList.setImage( row, 0, breakDisableSmallImg, preventColumResizing);
    }
}

auto CpuDebugger::removeInstructionBreakpoint(unsigned row) -> void {
    auto& instructionList = cpu->instructionLayout.list;
    instructionList.setImage( row, 0, nullImg);
    instructionList.resetRowForegroundColor( row );
}

auto CpuDebugger::updateWatcherSelection() -> void {
    auto& watcherList = cpu->watcher.list;
    if (snapshot->callbackAction == DebuggerAction::Watchpoint
        || snapshot->callbackAction == DebuggerAction::WatchpointWrite
        || snapshot->callbackAction == DebuggerAction::ExceptionPoint) {
        auto row = findWatcherRowBy(snapshot->callbackAddress, snapshot->callbackAction);
        if (row.has_value())
            watcherList.setSelection( row.value() );
        else if (watcherList.selected())
            watcherList.setSelected( false );
    } else if (watcherList.selected())
        watcherList.setSelected( false );
}

auto CpuDebugger::searchAddress(unsigned addr) -> void {
    auto instRow = findInstructionRowBy(static_cast<unsigned>(addr));
    if (instRow.has_value())
        cpu->instructionLayout.list.setSelection( instRow.value() );
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
        inst.disassembled = emulator->disassemble( addr, bytes );
        inst.data = emulator->disassembleData( addr, bytes );

        addr += bytes;
    }

    emulator->debuggerAdd( getCpuType(), DebuggerAction::ModifiedCode, _addr, addr );
}

auto CpuDebugger::fetchTraces() -> void {
    for (int i = 0; i < LIST_TRACES; i++) {
        uint16_t flags;
        Trace& trace = traces[i];
        trace.disassembled = emulator->disassembleTrace( i, flags );
        trace.flags = flags;
    }
}

auto CpuDebugger::prepareTheme() -> void {
    if (!snapshot)
        return;

    if (cpu->switchLayout.selection() == 1)
        fetchTraces();

    unsigned addr;
    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        addr = snap.pcEdge;
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        addr = snap.pcEdge;
    }

    currentInstRow = std::nullopt;

    if (!snapshot->codeMaybeModified)
        currentInstRow = findInstructionRowBy(addr);

    if (!currentInstRow.has_value())
        fetchInstructions(addr);
}

auto CpuDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        update68k(snap);

    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);

        if (!snap.superCpu && mode == Mode::CPU)
            update6510( snap );
        else if (snap.superCpu && mode == Mode::SCPU)
            update65816( snap );
        else
            return;
    }

    if (cpu->switchLayout.selection() == 1)
        updateTraceList();

    if (currentInstRow.has_value()) {
        cpu->instructionLayout.list.setSelection( currentInstRow.value() );
    } else {
        updateInstructionList();
    }

    updateWatcherSelection();
}

auto CpuDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::CPU, DebuggerAction::None, 0);
    emulator->debuggerAdd( getCpuType(), DebuggerAction::History, 0 );

    for (auto& watcher : watchers) {
        if (watcher.enabled) {
            emulator->debuggerAdd( getCpuType(), watcher.action, watcher.addr );
            updateWatchpointCondition(watcher);
        } else
            emulator->debuggerRemove( getCpuType(), watcher.action, watcher.addr );
    }

    // force reload of instruction cache
    emulator->debuggerAdd( getCpuType(), DebuggerAction::ModifiedCode, 0, ~0 );
}

auto CpuDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::CPU, DebuggerAction::None);
    emulator->debuggerRemove( getCpuType(), DebuggerAction::Breakpoint );
    emulator->debuggerRemove( getCpuType(), DebuggerAction::Watchpoint );
    emulator->debuggerRemove( getCpuType(), DebuggerAction::WatchpointWrite );
    emulator->debuggerRemove( getCpuType(), DebuggerAction::ExceptionPoint );
    emulator->debuggerRemove( getCpuType(), DebuggerAction::History );
    emulator->debuggerRemove( getCpuType(), DebuggerAction::ModifiedCode );
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
                reg->right.setText( mode == Mode::SCPU ? "M-E" : "I/O" );
            } else if (i == 3) {
                reg->left.setText( mode == Mode::SCPU ? "PBR" : "POR" );
                reg->right.setText( mode == Mode::SCPU ? "DBR" : "DDR" );
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
}

auto CpuDebugger::createWatchpointConditionOverlay(Watcher* watcher, GUIKIT::Position position) -> void {
    delete breakConditionWindow;
    delete breakConditionLayout;
    delete unfocusTimer;

    breakConditionWindow = new GUIKIT::Window(GUIKIT::Window::Hints::No_Title);
    breakConditionWindow->cocoa.keepMenuVisibilityOnDisplay();

    unfocusTimer = new GUIKIT::Timer();
    unfocusTimer->setInterval(100);
    unfocusTimer->onFinished = [this, watcher]() {
        unfocusTimer->setEnabled(false);
        breakConditionWindow->onUnFocus = [this, watcher]() {
            if (breakConditionWindow->visible()) {
                emuThread->lock();
                updateBreakpointVisuals(watcher);
                emuThread->unlock();
                breakConditionWindow->setVisible( false );
            }
        };
    };

    breakConditionWindow->setGeometry( {position.x + 20, position.y + 20, 500, 200} );
    breakConditionLayout = new BreakConditionLayout();
    auto* ex = &breakConditionLayout->expression;
    auto* hc = &breakConditionLayout->hitCount;

    breakConditionLayout->control.closeButton.onActivate = [this, watcher]() {
        emuThread->lock();
        updateBreakpointVisuals(watcher);
        emuThread->unlock();
        breakConditionWindow->setVisible( false );
    };

    hc->check.onToggle = [this, watcher, hc](bool checked) {
        watcher->useHitCount = checked;
        hc->compareCombo.setEnabled( checked );
        hc->compareVal.setEnabled( checked );
        emuThread->lock();
        updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->check.onToggle = [this, watcher, ex](bool checked) {
        watcher->useExpression = checked;
        ex->compareCombo.setEnabled( checked );
        ex->compareVal.setEnabled( checked );
        emuThread->lock();
        if (!updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->compareCombo.onChange = [this, watcher, hc]() {
        watcher->hitCountCompare = hc->compareCombo.selection();
        emuThread->lock();
        updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->compareCombo.onChange = [this, watcher, ex]() {
        watcher->expressionCompare = ex->compareCombo.selection();
        emuThread->lock();
        if (!updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->compareVal.onChange = [this, watcher, hc]() {
        std::string _v = hc->compareVal.text();
        watcher->hitCount = GUIKIT::String::convertToNumber( _v, 0 );
        emuThread->lock();
        updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->compareVal.onChange = [this, watcher, ex]() {
        watcher->expression = ex->compareVal.text();
        emuThread->lock();
        if (!updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->check.setText( trans->getA( trans->getA( "hit count" ) ) );
    ex->check.setText( trans->getA( trans->getA( "expression" ) ) );
    hc->compareCombo.append( "==" );
    hc->compareCombo.append( ">=" );
    ex->compareCombo.append( "true" );
    ex->compareCombo.append( trans->getA( "change" ) );
    hc->check.setChecked( watcher->useHitCount );
    ex->check.setChecked( watcher->useExpression );
    hc->compareCombo.setSelection( watcher->hitCountCompare );
    ex->compareCombo.setSelection( watcher->expressionCompare );
    hc->compareVal.setText( std::to_string( watcher->hitCount ) );
    ex->compareVal.setText( watcher->expression );
    hc->compareCombo.setEnabled( watcher->useHitCount );
    ex->compareCombo.setEnabled( watcher->useExpression );
    hc->compareVal.setEnabled( watcher->useHitCount);
    ex->compareVal.setEnabled( watcher->useExpression);

    std::string placeHolder = "";

    if (isAmiga()) {
        for (auto& cond: LIBAMI::DebuggerSnapshot::breakConditions)
            placeHolder += " " + (std::string)cond.ident;
    } else {
        if (mode == Mode::SCPU) {
            for (auto& cond: LIBC64::DebuggerSnapshot::breakConditionsSCPU)
                placeHolder += " " + (std::string)cond.ident;
        } else {
            for (auto& cond: LIBC64::DebuggerSnapshot::breakConditions)
                placeHolder += " " + (std::string)cond.ident;
        }
    }

    breakConditionLayout->info.setText(
        trans->getA("operators") + ": $ | & ^ || &&  == != <= < << >= > >> + - * / % \n" +
        trans->getA("replacements") + ": " + placeHolder + "$0000"
    );

    breakConditionLayout->control.closeButton.setText( trans->getA( "close" ) );
    breakConditionWindow->append( *breakConditionLayout );

    unsigned neededWidth = std::max(hc->check.minimumSize().width, ex->check.minimumSize().width);
    hc->children[0].size.width = neededWidth;
    ex->children[0].size.width = neededWidth;
    neededWidth = std::max(hc->compareCombo.minimumSize().width, ex->compareCombo.minimumSize().width);
    hc->children[1].size.width = neededWidth;
    ex->children[1].size.width = neededWidth;

    breakConditionWindow->setVisible(  );
    unfocusTimer->setEnabled();
}

auto CpuDebugger::updateWatchpointCondition(Watcher& watcher) -> bool {
    unsigned hitCount = watcher.useHitCount ? watcher.hitCount : 0;
    const auto& expression = watcher.useExpression ? watcher.expression : "";
    return emulator->setWatchpointCondition( watcher.action, watcher.addr, hitCount, watcher.hitCountCompare, expression, watcher.expressionCompare );
}

inline auto CpuDebugger::getCpuType() -> DebuggerTheme {
    if (isAmiga())
        return DebuggerTheme::CheckpointsCore1;
    if (mode == Mode::SCPU)
        return DebuggerTheme::CheckpointsCore2;

    return DebuggerTheme::CheckpointsCore1;
}

auto CpuDebugger::saveIdent() -> std::string {
    return "debugger_cpu";
}

auto CpuDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger CPU";
}

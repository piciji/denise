
#include "debugger.h"

#include "../../emulation/libami/interface.h"
#include "../program.h"
#include "../thread/emuThread.h"
#include "../../data/icons.h"
#include "../tools/macros.h"

#include <cstring>

// #838589
// fc0d18

GUIKIT::Timer* Debugger::timerVisibility = nullptr;
GUIKIT::Timer* Debugger::timer = nullptr;

Debugger::~Debugger() {
    delete[] memDump;
    delete[] memDumpOld;
    timer->setEnabled( false );
    setVisible(false);
}

Debugger::Debugger( Emulator::Interface* emulator, Mode mode )
    : emulator( emulator ), mode( mode ), control(this) {
    this->settings = program->getSettings( emulator );
    build();
}

Debugger::CPU::Watcher::Adder::Adder() {
    address.setMaxLength( 8 );
    append(address, {~0u, 0u}, 10);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

Debugger::CPU::Watcher::ExcAdder::ExcAdder()
: exceptionCombo(true) {
    append(exceptionCombo, {~0u, 0u}, 10);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

Debugger::CPU::Watcher::Watcher() {
    list.setHeaderText( { "", "address", "", ""} );

    append(list, {~0u, ~0u}, 5);
    append(breakPoint, {0u, 0u}, 3);
    append(watchPoint, {0u, 0u}, 3);
    append(adder, {~0u, 0u}, 5);

    append(excAdder, {~0u, 0u});

    GUIKIT::RadioBox::setGroup( breakPoint, watchPoint );
}

Debugger::CPU::State::Flags::Flags(Debugger* debugger) {
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

Debugger::CPU::State::Registers::Registers(Debugger* debugger) {
    static unsigned _wLabel = 0;
    static unsigned _wEdit = 0;
    static unsigned _wEdit16 = 0;

    if (_wLabel == 0) {
        GUIKIT::Label test;
        test.setFont( GUIKIT::Font::system( 11, "", true ) );
        test.setText( "0000" );
        _wLabel = test.minimumSize().width;

        GUIKIT::LineEdit edit;
        edit.setFont( GUIKIT::Font::system( 11, "", true ) );
        edit.setText( "00000000" );
        _wEdit = edit.minimumSize().width;

        edit.setText( "0000" );
        _wEdit16 = edit.minimumSize().width;
    }

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

    append(left, {_wLabel, 0u}, 5);
    append(leftVal, {debugger->isAmiga() ? _wEdit : _wEdit16, 0u}, 10);
    append(right, {_wLabel, 0u}, 5);
    append(rightVal, {debugger->isAmiga() ? _wEdit : _wEdit16, 0u});

    setAlignment( 0.5 );
}

Debugger::CPU::State::State::Trace::Trace() {
    append(toggle, {0u, 0u}, 10);
    append(clear, {0u, 0u});
    setAlignment( 0.5 );
}

Debugger::CPU::State::State(Debugger* debugger)
: flags(debugger) {
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
    append(trace, {0u, 0u});
}

Debugger::CPU::InstructionLayout::InstructionLayout() {
    list.setHeaderText( { "", "address", "data", "instruction" } );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

Debugger::CPU::TraceLayout::TraceLayout() {
    list.setHeaderText( { "address", "status","instruction" } );
    list.setFont( GUIKIT::Font::system( 11 ,"", true ) );
    list.setHeaderVisible( true );

    append(list, {~0u, ~0u});
}

Debugger::CPU::CPU(Debugger* debugger)
: state(debugger) {
    switchLayout.setLayout( 0, instructionLayout, {~0u, ~0u} );
    switchLayout.setLayout( 1, traceLayout, {~0u, ~0u} );

    append(switchLayout, {~0u, ~0u}, 20);
    append(watcher, {200u, ~0u}, 10);
    append(state, {0u, 0u});
}

Debugger::Memory::Memory(Debugger* debugger) {
    bankList.setHeaderText( { "bank", "mapping" } );
    bankList.setFont( GUIKIT::Font::system( 10 ,"", true ) );
    pageList.setFont( GUIKIT::Font::system( 10 ,"", true ) );
    bankList.setHeaderVisible( true );
    #define AL GUIKIT::ListView::Align::Left
    #define AR GUIKIT::ListView::Align::Right
    #define AC GUIKIT::ListView::Align::Center

    if (debugger->isAmiga()) {
        pageList.setHeaderText( { "address", "0","2", "4", "6", "8", "A", "C", "E", "ASCII" });
        pageList.setAlignment( {AL, AR, AR, AR, AR, AR, AR, AR, AR}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++) {
            bankList.append({hex(i, 2), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x1000; i++)
            pageList.append({hex(i * 16, 4), "   0", "   0", "   0", "   0", "   0", "   0", "   0", "   0", "................"}, true);
        pageList.unlockRedraw();
    } else if (debugger->mode == Mode::MemorySCPU) {
        pageList.setHeaderText( { "address", "0","1", "2", "3", "4", "5", "6", "7", "8","9", "A", "B", "C", "D", "E","F", "ASCII" });
        pageList.setAlignment( {AL, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++) {
            bankList.append({hex(i, 2), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x1000; i++)
            pageList.append({hex(i * 16, 4), "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "................"}, true);
        pageList.unlockRedraw();
    } else {
        pageList.setHeaderText( { "address", "0","1", "2", "3", "4", "5", "6", "7", "8","9", "A", "B", "C", "D", "E","F", "ASCII" });
        pageList.setAlignment( {AL, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x10; i++) {
            bankList.append({hex(i, 1), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++)
            pageList.append({hex(i * 16, 3), "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "................"}, true);
        pageList.unlockRedraw();
    }

    pageList.setHeaderVisible( true );

    bankList.setAlignment( {AR, AL}, true );
    bankList.autoSizeColumns();
    pageList.autoSizeColumns();

    append(bankList, {200u, ~0u}, 10);
    append(pageList, {~0u, ~0u});
}

Debugger::C64MemControl::Element::Element(Debugger* debugger) {
    imgView.setStore( 0 );
    append(imgView, {0u, 0u}, 2);
    append(label, {0u, 0u});
    setAlignment( 0.5 );
}

Debugger::C64MemControl::Left::Left(Debugger* debugger)
: exrom( debugger ), game( debugger ) {
    exrom.label.setText( "EXROM" );
    game.label.setText( "GAME" );
    append(exrom, {0u, 0u}, 2);
    append(game, {0u, 0u});
}

Debugger::C64MemControl::Middle::Middle(Debugger* debugger)
: charen( debugger ) {
    charen.label.setText( "CHAREN" );
    append(charen, {0u, 0u});
}

Debugger::C64MemControl::Right::Right(Debugger* debugger)
: loram( debugger ), hiram( debugger ) {
    loram.label.setText( "LORAM" );
    hiram.label.setText( "HIRAM" );
    append(loram, {0u, 0u}, 2);
    append(hiram, {0u, 0u});
}

Debugger::C64MemControl::C64MemControl(Debugger* debugger)
: left(debugger), right(debugger), middle( debugger ) {
    append(left, {0u, 0u}, 10);
    append(middle, {0u, 0u}, 10);
    append(right, {0u, 0u});

    setAlignment( 0.5 );
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

    if (debugger->isC64() && debugger->isMemMode()) {
        c64MemControl = new C64MemControl(debugger);
        append(*c64MemControl, {0u, 0u}, 20);
    }

    append( showTips, {0u, 0u} );

    setAlignment( 0.5 );
}

auto Debugger::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();

    GUIKIT::Geometry defaultGeometry = {50, 50, GUIKIT::Font::scale(1024), GUIKIT::Font::scale(570)};

    screenIdent = "debugger";
    if (mode == Mode::Memory)
        screenIdent += "_mem";
    else if (mode == Mode::MemorySCPU)
        screenIdent += "_memscpu";
    else if (mode == Mode::SCPU)
        screenIdent += "_scpu";

    GUIKIT::Geometry geometry = {settings->get<int>("debugger" + screenIdent + "_x", defaultGeometry.x)
        ,settings->get<int>("debugger" + screenIdent + "_y", defaultGeometry.y)
        ,settings->get<unsigned>("debugger" + screenIdent + "_width", defaultGeometry.width)
        ,settings->get<unsigned>("debugger" + screenIdent + "_height", defaultGeometry.height)
    };

    setGeometry( geometry );

    if (isOffscreen())
        setGeometry( defaultGeometry );

    addImg.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    breakEnableImg.loadPng((uint8_t*)Icons::ledRedRound, sizeof(Icons::ledRedRound));
    breakDisableImg.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
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

    control.resume.setImage( &pauseImg );
    control.stepOver.setImage( &stepOverImg );
    control.stepInto.setImage( &stepIntoImg );
    control.stepOut.setImage( &stepOutImg );
    control.search.setImage( &searchImg );
    control.line.setImage( &lineImg );
    control.frame.setImage( &frameImg );

    layout.setMargin( 10 );

    switch (mode) {
        case Mode::CPU:
        case Mode::SCPU:
            buildCPU();
            layout.append( *cpu, {~0u, ~0u}, 10 );
            break;

        case Mode::Memory:
        case Mode::MemorySCPU:
            buildMem();
            layout.append( *memory, {~0u, ~0u}, 10 );
            break;
    }

    layout.append( control, {~0u, 0u} );

    append( layout );

    onClose = [this]() {
        emuThread->lock();
        if (isCpuMode())
            emulator->debuggerDisableAll( getCpuType() );
        setVisible(false);

        if (!program->hasActiveDebugger()) {
            program->isPause &= ~2;
            timer->setEnabled( false );
        }
        emuThread->unlock();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>("debugger" + screenIdent + "_x", geometry.x);
        settings->set<int>("debugger" + screenIdent + "_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>("debugger" + screenIdent + "_width", geometry.width);
        settings->set<unsigned>("debugger" + screenIdent + "_height", geometry.height);
    };

    control.stepInto.onActivate = [this]() {
        stepInto( emulator );
    };

    control.stepOut.onActivate = [this]() {
        stepOut( emulator );
    };

    control.stepOver.onActivate = [this]() {
        stepOver( emulator );
    };

    control.line.onActivate = [this]() {
        stepLine( emulator );
    };

    control.frame.onActivate = [this]() {
        stepFrame( emulator );
    };

    control.resume.onActivate = [this]() {
        resume( emulator );
    };

    control.search.onClick = [this]() {
        if (emulator != activeEmulator)
            return;

        std::string addressText = control.searchEdit.text();
        if (addressText.empty())
            return;
        GUIKIT::String::remove( addressText, {"$", "0x"} );

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        if (isCpuMode()) {
            auto instRow = findInstructionRowBy(static_cast<unsigned>(address));
            if (instRow.has_value())
                cpu->instructionLayout.list.setSelection( instRow.value() );
            else {
                emuThread->lock();
                cacheInstructions(address);
                emuThread->unlock();
                updateInstructionList();
            }
        } else if (mode == Mode::Memory) {
            if (isAmiga()) {
                uint8_t bank = (address >> 16) & 0xff;
                memory->bankList.setSelection( bank );
                uint16_t page = address & 0xffff;
                memory->pageList.setSelection( page / 16 );
            } else {
                uint8_t bank = (address >> 12) & 0xff;
                memory->bankList.setSelection( bank );
                uint16_t page = address & 0xfff;
                memory->pageList.setSelection( page / 16 );
            }
            memory->bankList.onChange();
        } else if (mode == Mode::MemorySCPU) {
            uint8_t bank = (address >> 16) & 0xff;
            memory->bankList.setSelection( bank );
            uint16_t page = address & 0xffff;
            memory->pageList.setSelection( page / 16 );
        }
    };

    control.searchEdit.onReturn = [this]() {
        control.search.onClick();
    };

    control.showTips.onToggle = [this](bool checked) {
        settings->set<bool>("debugger_tips", checked);
        for (auto debugger : debuggers) {
            if (debugger->emulator == emulator) {
                if (debugger != this)
                    debugger->control.showTips.setChecked( checked );
                debugger->translate();
            }
        }
    };

    if (!timer) {
        timer = new GUIKIT::Timer();
        timer->setInterval( 50 );
        timer->onFinished = [this]() {
            if (timer->enabled()) {
                for (auto& debugger : program->getActiveDebuggers())
                    debugger->update();
            }
            timer->setEnabled( !isPaused() );
        };
    }

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

    control.showTips.setChecked( settings->get<bool>("debugger_tips", true) );

    std::string _title = emulator->ident + " Debugger ";
    switch (mode) {
        case Mode::Memory: _title += " Memory"; break;
        case Mode::MemorySCPU: _title += " Memory SCPU"; break;
        case Mode::CPU: _title += " CPU"; break;
        case Mode::SCPU: _title += " SCPU"; break;
        default: break;
    }
    
    setTitle( _title );

    translate();
}

auto Debugger::buildMem() -> void {
    memory = new Memory(this);
    if (isAmiga() || (mode == Mode::MemorySCPU)) {
        memDump = new uint8_t[0x10000];
        memDumpOld = new uint8_t[0x10000];
        std::memset(memDumpOld, 0, 0x10000);
    } else {
        memDump = new uint8_t[0x1000];
        memDumpOld = new uint8_t[0x1000];
        std::memset(memDumpOld, 0, 0x1000);
    }

    if (isC64() && isMemMode()) {
        updateC64MemControl(0, true);
    }

    memory->bankList.onChange = [this]() {
        unsigned selectedBank = memory->bankList.selection();
        emuThread->lock();
        if (isAmiga())
            loadMemoryBank<uint16_t>( selectedBank, true );
        else
            loadMemoryBank<uint8_t>( selectedBank, true );
        emuThread->unlock();
    };

    std::memset(bankListStore, 0, sizeof(bankListStore));
    memory->bankList.setSelected();
    memory->pageList.setSelected();
}

auto Debugger::buildCPU() -> void {
    cpu = new CPU(this);
    cpu->watcher.adder.add.setImage( &addImg );
    cpu->watcher.excAdder.add.setImage( &addImg );
    cpu->state.trace.clear.setImage( &clearImg );

    if (isAmiga()) {
        for (const Emulator::Interface::DebuggerException& debuggerException : LIBAMI::DebuggerSnapshot::exceptions)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else if (mode == Mode::SCPU) {
        for (const Emulator::Interface::DebuggerException& debuggerException : LIBC64::DebuggerSnapshot::exceptions65816)
            cpu->watcher.excAdder.exceptionCombo.append( debuggerException.ident, (int)debuggerException.vector );
    } else {
        for (const Emulator::Interface::DebuggerException& debuggerException : LIBC64::DebuggerSnapshot::exceptions)
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
                emulator->debuggerEnable(getCpuType(), DebuggerAction::Breakpoint, inst.addr, watcher->enabled);
            }
            updateWatcherList();
            timer->setEnabled();
            emuThread->unlock();
            enableInstructionBreakpoint(row, watcher->enabled);
        }
    };

    cpu->watcher.list.onClick = [this](unsigned row, unsigned column) {
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
            emulator->debuggerEnable( getCpuType(), watcher.action, watcher.addr, watcher.enabled );
            if (instRow.has_value())
                enableInstructionBreakpoint(instRow.value(), watcher.enabled);
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
        GUIKIT::String::remove( addressText, {"$", "0x"} );

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        DebuggerAction action = DebuggerAction::Breakpoint;
        if (cpu->watcher.watchPoint.checked())
            action = DebuggerAction::Watchpoint;

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

        emulator->debuggerAdd(getCpuType(), action, address);
        emuThread->unlock();
    };

    cpu->state.trace.toggle.onToggle = [this]() {
        if (cpu->switchLayout.selection() == 0) {
            emuThread->lock();
            updateTraceList();
            emuThread->unlock();
            cpu->switchLayout.setSelection( 1 );
        } else
            cpu->switchLayout.setSelection( 0 );
    };

    cpu->state.trace.clear.onActivate = [this]() {
        emuThread->lock();
        emulator->debuggerDisable( getCpuType(), DebuggerAction::History, 0 );
        emulator->debuggerEnable( getCpuType(), DebuggerAction::History, 0 );
        updateTraceList();
        emuThread->unlock();
    };

}

auto Debugger::translate() -> void {
    bool showTips = control.showTips.checked();
    control.searchEdit.setPlaceholder( trans->getA( "address" ) );
    control.showTips.setText( trans->getA("popup hints") );
    control.stepInto.setTooltip( showTips ? trans->getA("step into") : "" );
    control.stepOver.setTooltip( showTips ? trans->getA("step over") : "" );
    control.stepOut.setTooltip( showTips ? trans->getA("step out") : "" );
    control.line.setTooltip( showTips ? trans->getA("step end of line") : "" );
    control.frame.setTooltip( showTips ? trans->getA("step end of frame") : "" );

    if (isCpuMode()) {
        cpu->watcher.adder.address.setPlaceholder( trans->getA( "address" ) );
        cpu->watcher.breakPoint.setText( trans->getA( "instruction" ) );
        cpu->watcher.watchPoint.setText( trans->getA( "memory access" ) );

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
        cpu->state.trace.clear.setTooltip( showTips? trans->getA( "clear trace") : "" );
    } else if (isMemMode()) {
        memory->bankList.setHeaderText( {trans->getA( "address"), trans->getA( "assignment") } );
        auto header = memory->pageList.state.header;
        header[0] = trans->getA( "address" );;
        memory->pageList.setHeaderText(header);
    }
}

auto Debugger::update() -> void {
    if (emulator != activeEmulator)
        return;
    
    bool locked = emuThread->lock();
    unsigned addr;

    if (isAmiga()) {
        LIBAMI::Interface* amiEmu = dynamic_cast<LIBAMI::Interface*>(emulator);
        auto snap = amiEmu->getDebuggerSnapshot();
        addr = snap.pc;
        if (mode == Mode::CPU) {
            update68k(snap);
            if (cpu->switchLayout.selection() == 1)
                updateTraceList();
        } else if (mode == Mode::Memory)
            updateMemory( snap);
    } else {
        LIBC64::Interface* c64Emu = dynamic_cast<LIBC64::Interface*>(emulator);
        auto snap = c64Emu->getDebuggerSnapshot();
        addr = snap.pc;

        if (!snap.superCpu && mode == Mode::CPU) {
            update6510( snap );
            if (cpu->switchLayout.selection() == 1)
                updateTraceList();
        } else if (snap.superCpu && mode == Mode::SCPU) {
            update65816( snap );
            if (cpu->switchLayout.selection() == 1)
                updateTraceList();
        } else if (mode == Mode::Memory) {
            updateMemory( snap);
        } else if (snap.superCpu && mode == Mode::MemorySCPU) {
            updateMemory( snap);
        } else {
            if (locked)
                emuThread->unlock();
            return;
        }
    }

    if (isCpuMode()) {
        std::optional<unsigned> instRow = std::nullopt;

        if (!last.maybeModified)
            instRow = findInstructionRowBy(addr);

        if (instRow.has_value()) {
            if (locked)
                emuThread->unlock();
            cpu->instructionLayout.list.setSelection( instRow.value() );
        } else {
            cacheInstructions(addr);
            if (locked)
                emuThread->unlock();
            updateInstructionList();
        }

    } else if (locked)
        emuThread->unlock();
}

auto Debugger::update68k(LIBAMI::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i) {
            case 0: case 1: case 2: case 3:
            case 4: case 5: case 6: case 7:
                updateCpuReg(reg->leftVal, s.regsD[i]);
                updateCpuReg(reg->rightVal, s.regsA[i]);
                break;
            case 8:
                updateCpuReg(reg->leftVal, s.pc);
                updateCpuReg(reg->rightVal, s.ird);
                break;
            case 9:
                updateCpuReg(reg->leftVal, s.ssp);
                updateCpuReg(reg->rightVal, s.irc);
                break;
            case 10:
                updateCpuReg(reg->leftVal, s.usp);
                updateCpuReg(reg->rightVal, s.ipl);
                break;
        }
        i++;
    }

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
    updateCpuFlags(&LIBAMI::DebuggerSnapshot::flagIdent[0], s.flags);
}

auto Debugger::update6510(LIBC64::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i++) {
            case 0:
                updateCpuReg(reg->leftVal, s.pc);
                updateCpuReg(reg->rightVal, s.regS);
                break;
            case 1:
                updateCpuReg(reg->leftVal, s.regX);
                updateCpuReg(reg->rightVal, s.regY);
                break;
            case 2:
                updateCpuReg(reg->leftVal, s.regA);
                updateCpuReg(reg->rightVal, s.ioLines);
                break;
            case 3:
                updateCpuReg(reg->leftVal, s.por);
                updateCpuReg(reg->rightVal, s.ddr);
                break;
        }
    }

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
    updateCpuFlags(&LIBC64::DebuggerSnapshot::flagIdent[0], s.flags);
}

auto Debugger::update65816(LIBC64::DebuggerSnapshot& s) -> void {
    int i = 0;
    for (auto& reg : cpu->state.registers) {
        switch (i++) {
            case 0:
                updateCpuReg(reg->leftVal, s.pc);
                updateCpuReg(reg->rightVal, s.regS);
                break;
            case 1:
                updateCpuReg(reg->leftVal, s.regX);
                updateCpuReg(reg->rightVal, s.regY);
                break;
            case 2:
                updateCpuReg(reg->leftVal, s.regA);
                updateCpuReg(reg->rightVal, s.modeE);
                break;
            case 3:
                updateCpuReg(reg->leftVal, s.pbr);
                updateCpuReg(reg->rightVal, s.dbr);
                break;
        }
    }

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
    updateCpuFlags(&LIBC64::DebuggerSnapshot::flagIdent65816[0], s.flags);
}

auto Debugger::updateMemory(LIBC64::DebuggerSnapshot& s) -> void {
    auto& bankList = memory->bankList;
    int bank = 0;
    std::string ident;

    bankList.lockRedraw();
    if (mode == Mode::MemorySCPU) {
        for (auto& m : s.mapperSCPU) {
            switch (m) {
                case 1: ident = "SRAM"; break;
                case 2: ident = "ROM"; break;
                case 3: ident = "DRAM"; break;
                default:
                case 0: ident = "Unmapped"; break;
            }

            if (bankListStore[bank] != m) {
                bankList.setText( bank, 1, ident );
                if (m == 0)
                    bankList.setRowForegroundColor( UNUSED_COLOR, bank );
                else
                    bankList.resetRowForegroundColor( bank );

                bankListStore[bank] = m;
            }

            bank++;
        }
    } else {
        for (auto& m : s.mapper) {
            switch (m) {
                case 1: ident = "Ram"; break;
                case 2: ident = "I/O"; break;
                case 3: ident = "Char"; break;
                case 4: ident = "Kernal"; break;
                case 5: ident = "Basic"; break;
                case 6: ident = "RomL"; break;
                case 7: ident = "RomH"; break;
                case 8: ident = "UltimaxA0"; break;

                case 10: ident = "SramB0"; break;
                case 11: ident = "SramB1"; break;
                case 12: ident = "ROM"; break;
                default:
                case 0: ident = "Unmapped"; break;
            }

            if (bankListStore[bank] != m) {
                bankList.setText( bank, 1, ident );
                if (m == 0)
                    bankList.setRowForegroundColor( UNUSED_COLOR, bank );
                else
                    bankList.resetRowForegroundColor( bank );

                bankListStore[bank] = m;
            }

            bank++;
        }
    }
    bankList.unlockRedraw();

    unsigned selectedBank = 0;
    if (bankList.selected())
        selectedBank = bankList.selection();

    loadMemoryBank<uint8_t>(selectedBank, false);

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );

    updateC64MemControl(s.mode);
}

auto Debugger::updateC64MemControl(uint8_t _mode, bool init) -> void {
    unsigned i = 0;
    C64MemControl::Element* elements[] = {
        &control.c64MemControl->left.exrom, &control.c64MemControl->left.game,
        &control.c64MemControl->middle.charen, &control.c64MemControl->right.loram, &control.c64MemControl->right.hiram
    };

    for (auto el : elements) {
        bool _on = (_mode >> (4 - i)) & 1;

        if (init || (el->imgView.getStore() != _on)) {
            el->imgView.setImage( _on ? &onImg : &offImg );
            el->imgView.setStore(_on);
        }
        i++;
    }
}

auto Debugger::updateMemory(LIBAMI::DebuggerSnapshot& s) -> void {
    auto& bankList = memory->bankList;
    bankList.lockRedraw();

    int bank = 0;
    std::string ident;
    for (auto& m : s.mapper) {
        switch (m) {
            case 1: ident = "Chip Ram"; break;
            case 2: ident = "Slow Ram"; break;
            case 3: ident = "Kick ROM"; break;
            case 4: ident = "Ext ROM"; break;
            case 5: ident = "WOM"; break;
            case 6: ident = "Chipset"; break;
            case 7: ident = "CIA"; break;
            case 8: ident = "RTC"; break;
            case 9: ident = "Auto Conf"; break;
            case 10: ident = "Fast Ram"; break;
            case 11: ident = "Expansion"; break;

            default:
            case 0: ident = "Unmapped"; break;
        }

        if (bankListStore[bank] != m) {
            bankList.setText( bank, 1, ident );
            if (m == 0)
                bankList.setRowForegroundColor( UNUSED_COLOR, bank );
            else
                bankList.resetRowForegroundColor( bank );

            bankListStore[bank] = m;
        }

        bank++;
    }

    bankList.unlockRedraw();

    unsigned selectedBank = 0;
    if (bankList.selected())
        selectedBank = bankList.selection();

    loadMemoryBank<uint16_t>(selectedBank, false);

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
}

template<typename T>
auto Debugger::loadMemoryBank(uint8_t bank, bool noColorChanges) -> void {
    auto& pageList = memory->pageList;
    auto* pNew = reinterpret_cast<T*>(memDump);
    auto* pOld = reinterpret_cast<T*>(memDumpOld);

    if (isC64() && (mode == Mode::Memory))
        emulator->getMemoryDumpPage( bank, reinterpret_cast<uint8_t*>(pNew) );
    else
        emulator->getMemoryDumpBank( bank, pNew );

    unsigned visibleRow = pageList.getFirstVisibleRow();
    unsigned allowedTextChanges = 24 * 8;
    unsigned allowedColorChanges = 24 * 8;

    if constexpr (std::is_same_v<T, uint8_t>) {
        allowedTextChanges <<= 1;
        allowedColorChanges <<= 1;
    }

    pageList.lockRedraw();
    unsigned pos = 0;
    unsigned line = 0;
    bool lineChanged = false;

    bool colorChangeLock = noColorChanges;
    bool textChangeLock = false;

    bool pause = isPaused();
    std::string val;
    unsigned mask = (std::is_same_v<T, uint8_t>) ? 0xf : 7;
    bool _swapWords = false;
    if (isAmiga()) {
        auto mapping = bankListStore[bank];
        _swapWords = mapping == 1 || mapping == 2 || mapping == 3 || mapping == 4 || mapping == 5 || mapping == 10;
    }
    char ascii[17];
    unsigned limit = 0x1000;
    if constexpr (std::is_same_v<T, uint16_t>)
        limit = 0x8000;
    else if (mode == Mode::MemorySCPU)
        limit = 0x10000;

    while (true) {

        if (!textChangeLock && (pause || (line >= visibleRow)) && (*pNew != *pOld) ) {
            lineChanged = true;
            if (allowedTextChanges)
                allowedTextChanges--;

            if (_swapWords)
                val = hex( _swapWord(*pNew) );
            else
                val = hex( *pNew );
            pageList.setText( line, 1 + (pos & mask), val, true );
        }

        if (!colorChangeLock && (line >= visibleRow) && (*pNew != *pOld) ) {
            if (allowedColorChanges)
                allowedColorChanges--;
            pageList.setRowForegroundColor(DEBUG_COLOR, line, 1 + (pos & mask));
        } else if (pageList.rowForegroundColor( line, 1 + (pos & mask) ) != std::nullopt)
            pageList.resetRowForegroundColor(line, 1 + (pos & mask));

        pNew++;
        pOld++;

        if ((++pos & mask) == 0) {
            if (lineChanged) {
                const uint8_t* _a = (const uint8_t*)pNew;
                _a -= 16;
                toAscii(_a, 16, ascii);

                pageList.setText( line, mask + 2, ascii, true );
                lineChanged = false;
            }

            line++;
            if (pos == limit)
                break;

            if (!pause && !allowedTextChanges && !textChangeLock)
                textChangeLock = true;
            if (!allowedColorChanges && !colorChangeLock)
                colorChangeLock = true;
        }
    }

    pageList.unlockRedraw();
    std::memcpy(memDumpOld, memDump, (isAmiga() || (mode == Mode::MemorySCPU)) ? 0x10000 : 0x1000);
}

auto Debugger::updateCpuReg(GUIKIT::LineEdit& reg, unsigned val) -> void {
    if ((unsigned)reg.getStore() != val) {
        reg.setStore( static_cast<int>(val) );
        reg.setText( hex( val ) );
    }
}

auto Debugger::updateCpuFlags(const char* flagIdent, unsigned flags) -> void {
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

    emulator->debuggerAdd( getCpuType(), DebuggerAction::ModifiedCode, _addr, addr );
}

auto Debugger::updateInstructionList() -> void {
    auto& instructionList = cpu->instructionLayout.list;
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
             instructionList.setRowForegroundColor( DEBUG_COLOR, i );
         } else
             instructionList.setImage( i, 0, breakDisableImg, true );
        
    }
    instructionList.autoSizeColumns();
    instructionList.setSelection( 0 );

    instructionList.unlockRedraw();
}

auto Debugger::updateTraceList() -> void {
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

    for (int i = 0; i < 512; i++) {
        uint16_t flags;
        std::string str;
        std::string result = emulator->disassembleTrace( i, flags );
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

auto Debugger::addToWatcherList(unsigned addr, DebuggerAction action, const std::string& ident) -> void {
    watchers.push_back( {addr, ident, action, true} );

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
    auto& addrList = cpu->watcher.list;
    addrList.lockRedraw();
    addrList.reset();
    char hex[7];
    std::string format = "%06x";
    if (isC64())
        format = "%04x";

    for (auto& w : watchers) {
        if (w.ident.empty()) {
            snprintf(hex, 7, format.c_str(), w.addr);
            addrList.append( {"", std::string(hex), "", ""}, true );
        } else {
            addrList.append( {"", w.ident, "", ""}, true );
        }

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
    auto& instructionList = cpu->instructionLayout.list;

    if (state) {
        instructionList.setImage( row, 0, breakEnableImg);
        instructionList.setRowForegroundColor( DEBUG_COLOR, row );
    } else {
        instructionList.setImage( row, 0, breakDisableImg);
        instructionList.resetRowForegroundColor( row );
    }
}

auto Debugger::removeInstructionBreakpoint(unsigned row) -> void {
    auto& instructionList = cpu->instructionLayout.list;
    instructionList.setImage( row, 0, nullImg);
    instructionList.resetRowForegroundColor( row );
}

auto Debugger::enableWatcher(unsigned row, bool state) -> void {
    auto& addrList = cpu->watcher.list;
    addrList.setImage( row, 0, state ? breakEnableImg : breakDisableImg );
}

auto Debugger::updateToolboxVisibility() -> void {
    if (isPaused()) {
        if (control.resume.image() == &pauseImg) {
            control.resume.setImage( &resumeImg );
            control.stepOver.setEnabled( );
            control.stepInto.setEnabled( );
            control.stepOut.setEnabled( );
            control.line.setEnabled( );
            control.frame.setEnabled( );
        }
    } else {
        if (control.resume.image() == &resumeImg) {
            control.resume.setImage( &pauseImg );
            control.stepOver.setEnabled( false );
            control.stepInto.setEnabled( false );
            control.stepOut.setEnabled( false );
            control.line.setEnabled( false );
            control.frame.setEnabled( false );
        }
    }
}

auto Debugger::Callback(Emulator::Interface::DebuggerAction action, unsigned addr, bool maybeModified) -> void {
    for (auto debugger : program->getActiveDebuggers()) {
        if (debugger->isCpuMode()) {
            debugger->last.action = action;
            debugger->last.addr = addr;
            debugger->last.maybeModified = maybeModified;
        }
    }

    if (emuThread->enabled)
        emuThread->events |= EmuThread::EVT_DEBUGGER;
    else
        Callback();
}

auto Debugger::Callback() -> void {
    timer->setEnabled( false );
    timerVisibility->setEnabled(false);

    for (auto debugger : program->getActiveDebuggers()) {
        debugger->update();
        debugger->updateToolboxVisibility();

        if (debugger->isCpuMode()) {
            debugger->updateWatcherSelection();
        }
    }
}

auto Debugger::updateWatcherSelection() -> void {
    auto& watcherList = cpu->watcher.list;
    if (last.action == DebuggerAction::Watchpoint || last.action == DebuggerAction::ExceptionPoint) {
        auto row = findWatcherRowBy(last.addr, last.action);
        if (row.has_value())
            watcherList.setSelection( row.value() );
        else if (watcherList.selected())
            watcherList.setSelected( false );
    } else if (watcherList.selected())
        watcherList.setSelected( false );
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

auto Debugger::toAscii(const uint8_t* buf, int len, char* result, char pad) -> void {

    for (int i = 0; i < len; i++) {
        result[i] = isprint(int(buf[i])) ? char(buf[i]) : pad;
    }

    result[len] = 0;
}

auto Debugger::stepOut(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    if (emulator->debuggerStepOut()) {
        program->isPause &= ~2;
        timer->setEnabled();
        timerVisibility->setEnabled();
    }
    emuThread->unlock();
}

auto Debugger::stepInto(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerStepInto();
    emuThread->unlock();
}

auto Debugger::stepOver(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerStepOver();
    emuThread->unlock();
}

auto Debugger::stepLine(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerAdd( Emulator::Interface::DebuggerCpu::Unspecified, DebuggerAction::Line, 0 );
    emuThread->unlock();
}

auto Debugger::stepFrame(Emulator::Interface* emulator) -> void {
    if (!isPaused() || (emulator != activeEmulator))
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();

    emulator->debuggerAdd( Emulator::Interface::DebuggerCpu::Unspecified, DebuggerAction::Frame, 0 );
    emuThread->unlock();
}

auto Debugger::resume(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    program->isPause ^= 2;

    emuThread->lock();
    for (auto& debugger : program->getActiveDebuggers())
        debugger->update();

    timer->setEnabled(!isPaused());
    timerVisibility->setEnabled();
    emuThread->unlock();
}

auto Debugger::reset() -> void {
    if (timer)
        timer->setEnabled( false );

    for (auto debugger : program->getActiveDebuggers()) {

        if (debugger->isCpuMode()) {
            debugger->initWatchers();
        }

        debugger->updateToolboxVisibility();
        if (!timer->enabled())
            timer->setEnabled( );
    }
}

auto Debugger::makeVisible() -> void {
    bool result = program->hasActiveDebugger();
    setVisible();
    if (emulator != activeEmulator)
        return;

    if (!result) {
        timer->setEnabled( );
        program->isPause &= ~2;
    }

    updateToolboxVisibility();

    emuThread->lock();
    if (isCpuMode()) {
        initWatchers();
    }

    if (!timer->enabled())
        update();

    emuThread->unlock();
}

auto Debugger::initWatchers() -> void {
    emulator->debuggerEnable( getCpuType(), DebuggerAction::History, 0 );
    for (auto& watcher : watchers)
        emulator->debuggerEnable( getCpuType(), watcher.action, watcher.addr, watcher.enabled );

    last.maybeModified = true;
}

inline auto Debugger::isPaused() -> bool {
    return (program->isPause & 2) == 2;
}

inline auto Debugger::isC64() -> bool {
    return dynamic_cast<LIBC64::Interface*>(emulator);
}

inline auto Debugger::isAmiga() -> bool {
    return dynamic_cast<LIBAMI::Interface*>(emulator);
}

inline auto Debugger::getCpuType() -> Emulator::Interface::DebuggerCpu {
    if (isAmiga())
        return Emulator::Interface::DebuggerCpu::C68000;
    if (mode == Mode::SCPU)
        return Emulator::Interface::DebuggerCpu::C65c816;

    return Emulator::Interface::DebuggerCpu::C6510;
}
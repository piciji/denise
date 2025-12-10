
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

    if (dynamic_cast<LIBAMI::Interface*>(debugger->emulator)) {
        i = sizeof(LIBAMI::DebuggerSnapshot::flagIdent);
        ident = &LIBAMI::DebuggerSnapshot::flagIdent[0];
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

Debugger::CPU::State::Registers::Registers() {
    static unsigned _w = 0;

    if (_w == 0) {
        GUIKIT::Label test;
        test.setFont( GUIKIT::Font::system( 11 ) );
        test.setText( "U" );
        _w = test.minimumSize().width;
    }

    left.setFont( GUIKIT::Font::system( 11 ) );
    leftVal.setFont( GUIKIT::Font::system( 11 ) );
    right.setFont( GUIKIT::Font::system( 11 ) );
    rightVal.setFont( GUIKIT::Font::system( 11 ) );

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

    append(left, {_w * 4, 0u}, 5);
    append(leftVal, {_w * 8, 0u}, 10);
    append(right, {_w * 4, 0u}, 5);
    append(rightVal, {_w * 8, 0u});

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
    bool is68k = dynamic_cast<LIBAMI::Interface*>(debugger->emulator);
    registers.resize( is68k ? 11 : 4 );
    for (auto& reg : registers) {
        reg = new Registers;
        i++;

        if (is68k)
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

    if (dynamic_cast<LIBAMI::Interface*>(debugger->emulator)) {
        pageList.setHeaderText( { "address", "0","2", "4", "6", "8", "A", "C", "E", "ASCII" });
        pageList.setAlignment( {AL, AR, AR, AR, AR, AR, AR, AR, AR}, true );

        for (unsigned i = 0; i < 0x100; i++) {
            bankList.append({hex(i, 2), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }

        for (unsigned i = 0; i < 0x1000; i++)
            pageList.append({hex(i * 16, 4), "   0", "   0", "   0", "   0", "   0", "   0", "   0", "   0", "................"}, true);
    } else {
        pageList.setHeaderText( { "address", "0","1", "2", "3", "4", "5", "6", "7", "8","9", "A", "B", "C", "D", "E","F", "ASCII" });
        pageList.setAlignment( {AL, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC}, true );

        for (unsigned i = 0; i < 0x10; i++) {
            bankList.append({hex(i, 1), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }

        for (unsigned i = 0; i < 0x100; i++)
            pageList.append({hex(i * 16, 3), "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "................"}, true);
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

    if (dynamic_cast<LIBC64::Interface*>(debugger->emulator) && debugger->mode == Mode::Memory) {
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
            buildCPU();
            layout.append( *cpu, {~0u, ~0u}, 10 );
            break;

        case Mode::Memory:
            buildMem();
            layout.append( *memory, {~0u, ~0u}, 10 );
            break;
    }

    layout.append( control, {~0u, 0u} );

    append( layout );

    onClose = [this]() {
        emuThread->lock();
        if (mode == Mode::CPU)
            emulator->debuggerDisableAll();
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

        if (mode == Mode::CPU) {
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
            if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
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

                if ((program->isPause & 2) == 0)
                    timer->setEnabled( );
            }
        };
    }

    if (!timerVisibility) {
        timerVisibility = new GUIKIT::Timer();
        timerVisibility->setInterval( 50 );
        timerVisibility->onFinished = [this]() {
            if (timerVisibility->enabled()) {
                for (auto& debugger : program->getActiveDebuggers())
                    debugger->updateToolboxVisibility();

                timerVisibility->setEnabled(false);
            }
        };
    }

    control.showTips.setChecked( settings->get<bool>("debugger_tips", true) );
    
    setTitle( emulator->ident + " Debugger" );

    translate();
}

auto Debugger::buildMem() -> void {
    memory = new Memory(this);
    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        memDump = new uint8_t[0x10000];
        memDumpOld = new uint8_t[0x10000];
        std::memset(memDumpOld, 0, 0x10000);
    } else {
        memDump = new uint8_t[0x1000];
        memDumpOld = new uint8_t[0x1000];
        std::memset(memDumpOld, 0, 0x1000);

        updateC64MemControl(0, true);
    }

    memory->bankList.onChange = [this]() {
        unsigned selectedBank = memory->bankList.selection();
        emuThread->lock();
        if (dynamic_cast<LIBAMI::Interface*>(emulator))
            loadMemoryBank16( selectedBank, true );
        else
            loadMemoryBank12( selectedBank, true );
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

    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        for (const Emulator::Interface::DebuggerException& debuggerException : LIBAMI::DebuggerSnapshot::exceptions)
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
                emulator->debuggerAdd(DebuggerAction::Breakpoint, inst.addr);
            } else {
                watcher->enabled ^= 1;
                emulator->debuggerEnable(DebuggerAction::Breakpoint, inst.addr, watcher->enabled);
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

    cpu->watcher.excAdder.add.onActivate = [this]() {
        unsigned vector = (int)cpu->watcher.excAdder.exceptionCombo.userData();

        DebuggerAction action = DebuggerAction::ExceptionPoint;

        if (findWatcherBy( vector, action ))
            return;

        emuThread->lock();
        addToWatcherList( vector, action, cpu->watcher.excAdder.exceptionCombo.text() );
        updateWatcherList();
        emulator->debuggerAdd(action, vector);
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

        emulator->debuggerAdd(action, address);
        emuThread->unlock();
    };

    cpu->state.trace.toggle.onToggle = [this]() {
        if (cpu->switchLayout.selection() == 0) {
            updateTraceList();
            cpu->switchLayout.setSelection( 1 );
        } else
            cpu->switchLayout.setSelection( 0 );
    };

    cpu->state.trace.clear.onActivate = [this]() {
        emuThread->lock();
        emulator->debuggerDisable( Emulator::Interface::DebuggerAction::History, 0 );
        emulator->debuggerEnable( Emulator::Interface::DebuggerAction::History, 0 );
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

    if (mode == Mode::CPU) {
        cpu->watcher.adder.address.setPlaceholder( trans->getA( "address" ) );
        cpu->watcher.breakPoint.setText( trans->getA( "instruction" ) );
        cpu->watcher.watchPoint.setText( trans->getA( "memory access" ) );

        int i = 0;
        if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
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
                    reg->right.setText( "I/O" );
                } else if (i == 3) {
                    reg->left.setText( "POR" );
                    reg->right.setText( "DDR" );
                }

                i++;
            }
        }

        cpu->instructionLayout.list.setHeaderText( {"", trans->getA( "address"), trans->getA( "data"), trans->getA( "instruction") } );
        cpu->traceLayout.list.setHeaderText( {trans->getA( "address"), trans->getA( "status"), trans->getA( "instruction") } );
        cpu->state.trace.toggle.setText( trans->getA( "trace") );
        cpu->state.trace.toggle.setTooltip( showTips ? trans->getA( "toggle trace") : "" );
        cpu->state.trace.clear.setTooltip( showTips? trans->getA( "clear trace") : "" );
    } else if (mode == Mode::Memory) {
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

    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        LIBAMI::Interface* amiEmu = dynamic_cast<LIBAMI::Interface*>(emulator);
        auto snap = amiEmu->getDebuggerSnapshot();
        addr = snap.pc;
        if (mode == Mode::CPU)
            update68k(snap);
        else if (mode == Mode::Memory)
            updateMemory( snap);
    } else {
        LIBC64::Interface* c64Emu = dynamic_cast<LIBC64::Interface*>(emulator);
        auto snap = c64Emu->getDebuggerSnapshot();
        addr = snap.pc;
        if (mode == Mode::CPU)
            update6510( snap );
        else if (mode == Mode::Memory)
            updateMemory( snap);
    }

    if (mode == Mode::CPU) {
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

        if (cpu->switchLayout.selection() == 1)
            updateTraceList();

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

auto Debugger::updateMemory(LIBC64::DebuggerSnapshot& s) -> void {
    auto& bankList = memory->bankList;
    bankList.lockRedraw();

    int bank = 0;
    std::string ident;
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

    loadMemoryBank12(selectedBank, false);

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

    loadMemoryBank16(selectedBank, false);

    control.position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
}

auto Debugger::loadMemoryBank16(uint8_t bank, bool swap) -> void {
    auto& pageList = memory->pageList;
    auto* pNew = reinterpret_cast<uint16_t*>(memDump);
    auto* pOld = reinterpret_cast<uint16_t*>(memDumpOld);
    emulator->getMemoryDump( bank, pNew );

    unsigned visibleRow = pageList.getFirstVisibleRow();
    //fprintf(stderr, "%d\n", visibleRow);
    unsigned allowedTextChanges = 24 * 8;
    unsigned allowedColorChanges = 24 * 8;

    pageList.lockRedraw();
    unsigned pos = 0;
    unsigned line = 0;
    bool lineChanged = false;
    
    bool colorChangeLock = swap;
    bool textChangeLock = false;
    
    bool pause = !!(program->isPause & 2);

    std::string val;
    auto mapping = bankListStore[bank];
    bool _swapWords = mapping == 1 || mapping == 2 || mapping == 3 || mapping == 4 || mapping == 5 || mapping == 10;
    char ascii[17];

    while (true) {
        
        if (!textChangeLock && (pause || (line >= visibleRow)) && (*pNew != *pOld) ) {
            lineChanged = true;
            if (allowedTextChanges)
                allowedTextChanges--;

            if (_swapWords)
                val = hex( _swapWord(*pNew) );
            else
                val = hex( *pNew );
            pageList.setText( line, 1 + (pos & 7), val, true );
        }

        if (!colorChangeLock && (line >= visibleRow) && (*pNew != *pOld) ) {
            if (allowedColorChanges)
                allowedColorChanges--;
            pageList.setRowForegroundColor(DEBUG_COLOR, line, 1 + (pos & 7));
        } else if (pageList.rowForegroundColor( line, 1 + (pos & 7) ) != std::nullopt)
            pageList.resetRowForegroundColor(line, 1 + (pos & 7));

        pNew++;
        pOld++;

        if ((++pos & 7) == 0) {
            if (lineChanged) {
                const uint8_t* _a = (const uint8_t*)pNew;
                _a -= 16;
                toAscii(_a, 16, ascii);

                pageList.setText( line, 9, ascii, true );
                lineChanged = false;
            }

            line++;
            if (pos == 0x8000)
                break;

            if (!pause && !allowedTextChanges && !textChangeLock)
                textChangeLock = true;
            if (!allowedColorChanges && !colorChangeLock)
                colorChangeLock = true;
        }
    }

    pageList.unlockRedraw();
    std::memcpy(memDumpOld, memDump, 0x10000);
}

auto Debugger::loadMemoryBank12(uint8_t bank, bool swap) -> void {
    auto& pageList = memory->pageList;
    auto* pNew = memDump;
    auto* pOld = memDumpOld;
    emulator->getMemoryDump( bank, pNew );
    
    unsigned visibleRow = pageList.getFirstVisibleRow();
    //fprintf(stderr, "%d\n", visibleRow);
    unsigned allowedTextChanges = 24 * 16;
    unsigned allowedColorChanges = 24 * 16;

    pageList.lockRedraw();
    unsigned pos = 0;
    unsigned line = 0;
    bool lineChanged = false;
    
    bool colorChangeLock = swap;
    bool textChangeLock = false;
    
    bool pause = !!(program->isPause & 2);

    char ascii[17];

    while (true) {

        if (!textChangeLock && (pause || (line >= visibleRow)) && (*pNew != *pOld) ) {
            lineChanged = true;
            if (allowedTextChanges)
                allowedTextChanges--;
            std::string val = hex( *pNew );
            pageList.setText( line, 1 + (pos & 0xf), val, true );
        }

        if (!colorChangeLock && (line >= visibleRow) && (*pNew != *pOld) ) {
            if (allowedColorChanges)
                allowedColorChanges--;
            pageList.setRowForegroundColor(DEBUG_COLOR, line, 1 + (pos & 0xf));
        } else if (pageList.rowForegroundColor( line, 1 + (pos & 0xf) ) != std::nullopt)
            pageList.resetRowForegroundColor(line, 1 + (pos & 0xf));

        pNew++;
        pOld++;

        if ((++pos & 0xf) == 0) {
            if (lineChanged) {
                const uint8_t* _a = (const uint8_t*)pNew;
                _a -= 16;
                toAscii(_a, 16, ascii);

                pageList.setText( line, 17, ascii, true );
                lineChanged = false;
            }

            line++;
            if (pos == 0x1000)
                break;

            if (!pause && !allowedTextChanges && !textChangeLock)
                textChangeLock = true;
            if (!allowedColorChanges && !colorChangeLock)
                colorChangeLock = true;
        }
    }

    pageList.unlockRedraw();
    std::memcpy(memDumpOld, memDump, 0x1000);
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

    emulator->debuggerAdd( Emulator::Interface::DebuggerAction::ModifiedCode, _addr, addr );
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
    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        flagSize = sizeof(LIBAMI::DebuggerSnapshot::flagIdent);
        flagIdent = &LIBAMI::DebuggerSnapshot::flagIdent[0];
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
    if (dynamic_cast<LIBC64::Interface*>(emulator))
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
    if (program->isPause & 2) {
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
        if (debugger->mode == Mode::CPU) {
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

        if (debugger->mode == Mode::CPU) {
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
    if (emulator != activeEmulator)
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
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerStepInto();
    emuThread->unlock();
}

auto Debugger::stepOver(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerStepOver();
    emuThread->unlock();
}

auto Debugger::stepLine(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();
    emulator->debuggerAdd( DebuggerAction::Line, 0 );
    emuThread->unlock();
}

auto Debugger::stepFrame(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    emuThread->lock();
    program->isPause &= ~2;

    timer->setEnabled();
    timerVisibility->setEnabled();

    emulator->debuggerAdd( DebuggerAction::Frame, 0 );
    emuThread->unlock();
}

auto Debugger::resume(Emulator::Interface* emulator) -> void {
    if (emulator != activeEmulator)
        return;
    program->isPause ^= 2;

    emuThread->lock();
    for (auto& debugger : program->getActiveDebuggers())
        debugger->update();

    timer->setEnabled((program->isPause & 2) == 0);
    timerVisibility->setEnabled();
    emuThread->unlock();
}

auto Debugger::reset() -> void {
    if (timer)
        timer->setEnabled( false );

    for (auto debugger : program->getActiveDebuggers()) {

        if (debugger->mode == Mode::CPU)
            debugger->initWatchers();

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
    if (mode == Mode::CPU)
        initWatchers();

    if (!timer->enabled())
        update();

    emuThread->unlock();
}

auto Debugger::initWatchers() -> void {
    emulator->debuggerEnable( Emulator::Interface::DebuggerAction::History, 0 );
    for (auto& watcher : watchers)
        emulator->debuggerEnable( watcher.action, watcher.addr, watcher.enabled );

    last.maybeModified = true;
}

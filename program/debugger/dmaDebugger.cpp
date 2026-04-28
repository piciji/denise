
#include "dmaDebugger.h"

#include "../program.h"
#include "../thread/emuThread.h"

DmaDebugger::DmaDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

DmaDebugger::~DmaDebugger() {
    if (dmaControl) {
        if (control)
            control->remove( *dmaControl );
        delete dmaControl;
    }
}

DmaDebugger::Dma::Legend::Legend::Watcher::Watcher() {
    append( spacer, {10u, 0u}, 0);
    append( button, {90u, 0u}, 0);
}

DmaDebugger::Dma::Legend::Legend() {
    dma.setAlign(GUIKIT::Label::Align::Right);
    dmaAddr.setAlign(GUIKIT::Label::Align::Right);
    dmaData.setAlign(GUIKIT::Label::Align::Right);
    mnemonic.setAlign(GUIKIT::Label::Align::Right);
    cpu.setAlign(GUIKIT::Label::Align::Right);
    cpuAddr.setAlign(GUIKIT::Label::Align::Right);
    cpuData.setAlign(GUIKIT::Label::Align::Right);
    
    append( spacer, {0u, 0u}, 37 );
    append( dma, {100u, 0u}, 14 );
    append( dmaAddr, {100u, 0u}, 14 );
    append( dmaData, {100u, 0u}, 14 );
    append( mnemonic, {100u, 0u}, 16 );
    append( cpu, {100u, 0u}, 14 );
    append( cpuAddr, {100u, 0u}, 14 );
    append( cpuData, {100u, 0u}, 16 );

    int i = 0;
    for (auto& watcher : watchers) {
        watcher.position = i++;
        append( watcher, {0u, 0u}, 13 );
    }
}

DmaDebugger::DmaControl::DmaControl(DmaDebugger* debugger) {
    if (debugger->isAmiga()) {
        append(cycleButton, {0u, 0u} );
    } else {
        append(rdyButton, {0u, 0u} );
    }
}

DmaDebugger::Dma::DmaLine::DmaLine(DmaDebugger* debugger) {
    append(viewer, {~0u, 440u});
    if (debugger->isAmiga())
        viewer.setAddrAs24bit();
}

DmaDebugger::Dma::DmaFrame::BusUsage::BusUsage() {
    append(enableUsage, {110u, 0u}, 10);
    append(canvas, {15u, 10u});

    setAlignment( 0.5 );
}

DmaDebugger::Dma::DmaFrame::DmaFrame(DmaDebugger* debugger) {
    append(showUsage, {0u, 0u}, 10);

    unsigned length = debugger->isAmiga() ? std::size(LIBAMI::DebuggerSnapshot::dmaModeGroups) : std::size(LIBC64::DebuggerSnapshot::dmaModeGroups);
    const auto& entry = debugger->isAmiga() ? LIBAMI::DebuggerSnapshot::dmaModeGroups : LIBC64::DebuggerSnapshot::dmaModeGroups;

    // i == 0 is free BUS
    for (unsigned int i = 1; i < length; i++) {
        auto busUsage = new BusUsage;
        std::string _str = entry[i];
        busUsage->enableUsage.setText( _str );
        busUsage->canvas.setStore( i );
        if (_str != "Cpu") // CPU
            busUsage->enableUsage.setChecked();
        append( *busUsage, {0u, 0u}, 10 );
        usages.push_back(busUsage);
    }

    slider.setLength( 101 );
    slider.setPosition( 50 );
    append( slider, {~0u, 0u} );
    setPadding( 10 );
}


DmaDebugger::Dma::Dma(DmaDebugger* debugger)
: dmaFrame( debugger ), dmaLine( debugger ) {
    append(legend, {100u, 0u}, 10);
    append(dmaLine, {~0u, 0u}, 10);
    append(dmaFrame, {0u, 0u});
}

auto DmaDebugger::buildControl() -> GUIKIT::Layout* {
    dmaControl = new DmaControl(this);

    if (isC64()) {
        dmaControl->rdyButton.onActivate = [this]() {
            haltCpu(emulator);
        };
    } else {
        dmaControl->cycleButton.setImage( &nextImg );
        dmaControl->cycleButton.onActivate = [this]() {
            if (emulator != activeEmulator)
                return;
            emuThread->lock();
            timerVisibility->setEnabled();
            emulator->debuggerAdd( getTheme(), DebuggerAction::Softstop, 0 );
            emuThread->unlockDebugger();
            emuThread->unlock();
        };
    }

    return dmaControl;
}

auto DmaDebugger::updateColor(Dma::DmaFrame::BusUsage* busUsage, unsigned id, unsigned _col) -> void {
    emuThread->lock();
    dmaColors[ id ].color = _col;
    settings->set<unsigned>(saveIdent() + "_color_" + std::to_string( id ), _col);
    busUsage->canvas.setBackgroundColor( _col );
    emuThread->unlock();
}

auto DmaDebugger::buildTheme() -> GUIKIT::Layout* {
    dma = new Dma( this );

    loadColors();

    dma->dmaFrame.showUsage.onToggle = [this](bool checked) {
        emuThread->lock();
        VideoManager::getInstance( emulator )->dmaColors = checked ? &dmaColors[0] : nullptr;
        if (checked)
            emulator->debuggerAdd( getTheme(), DebuggerAction::DmaView, isPaused() ? 1 : 0);
        else
            emulator->debuggerRemove( getTheme(), DebuggerAction::DmaView, isPaused() ? 1 : 0);
        emuThread->unlock();
    };

    for (auto& busUsage : dma->dmaFrame.usages) {
        unsigned id = busUsage->canvas.getStore();
        busUsage->canvas.setBackgroundColor( dmaColors[id].color );

        busUsage->canvas.onMouseRelease = [this, busUsage, id](GUIKIT::Mouse::Button button) {

            GUIKIT::ColorChooser colorChooser;
            colorChooser.onChoose = [this, id, busUsage](unsigned color) {
                updateColor(busUsage, id, color);
            };
            
            colorChooser.setWindow( *this );
            unsigned defaultColor = dmaColors[ id ].color;
            colorChooser.setDefault( defaultColor );
            auto result = colorChooser.choose();
            if (GUIKIT::Application::isQuit)
                return;

            updateColor(busUsage, id, result.value_or(defaultColor));
        };

        busUsage->enableUsage.onToggle = [this, id](bool checked) {
            dmaColors[ id ].enabled = checked;
        };
    }

    dma->dmaFrame.slider.onChange = [this](unsigned position) {
        emuThread->lock();
        for (auto& dmaColor : dmaColors)
            dmaColor.alpha = position;
        emuThread->unlock();
    };

    scrollTimer.setInterval( 100 );
    scrollTimer.onFinished = [this]() {
        dma->dmaLine.viewer.scrollToActive();
        scrollTimer.setEnabled( false );
    };

    for (auto& watcher : dma->legend.watchers) {
        auto* w = &watcher;

        watcher.button.onMenu = [this, w]() {
            for (auto& w : dma->legend.watchers) {
                if (w.remove( w.edit )) {
                    w.append( w.button, {90u, 0u} );
                    w.synchronizeLayout();
                }
            }
            dma->legend.currentWatcher = w;
            return &watcherMenu;
        };

        watcher.edit.onReturn = [this]() {
            auto* w = dma->legend.currentWatcher;
            if (w) {
                auto _text = w->edit.text();
                int val = GUIKIT::String::convertHexToInt( _text, -1 );

                if (val != -1 && val <= 0xffffff) {
                    emuThread->lock();
                    w->button.setStore( -1 );
                    updateTheme();
                    w->button.setStore( val );
                    emulator->debuggerAdd( getTheme(), DebuggerAction::DmaWatch, val, w->position );
                    emuThread->unlock();
                    w->button.setText( _text );
                }

                w->remove( w->edit );
                w->append( w->button, {90u, 0u} );
                w->synchronizeLayout();
            }
        };
    }

    auto* item = new GUIKIT::MenuItem();
    item->onActivate = [this]() {
        auto* w = dma->legend.currentWatcher;
        if (w) {
            w->remove( w->button );
            w->append( w->edit, {90u, 0u} );
            w->edit.setMaxLength(6);
            w->edit.setFocused();
            w->synchronizeLayout();
        }
    };
    item->setText( "<" + trans->getA( "address" ) + ">" );
    watcherMenu.append(*item);
    watchItems.push_back(item);

    item = new GUIKIT::MenuItem();
    item->onActivate = [this]() {
        auto* w = dma->legend.currentWatcher;

        if (w) {
            emuThread->lock();
            emulator->debuggerRemove( getTheme(), DebuggerAction::DmaWatch, w->position );
            w->button.setStore( -1 );
            updateTheme();
            emuThread->unlock();
            w->button.setText( trans->getA( "connect" ) );
        }
    };
    item->setText( trans->getA( "remove" ) );
    watcherMenu.append(*item);
    watchItems.push_back(item);

    item = new GUIKIT::MenuItem();
    item->onActivate = [this]() {
        auto* w = dma->legend.currentWatcher;

        if (w) {
            emuThread->lock();
            w->button.setStore( -1 );
            updateTheme();
            w->button.setStore( 1 << 24 );
            emulator->debuggerAdd( getTheme(), DebuggerAction::DmaWatch, 1 << 24, w->position );
            emuThread->unlock();
            w->button.setText( isAmiga() ? "IPL" : "IRQ/NMI" );
        }
    };
    item->setText( isAmiga() ? "IPL" : "IRQ/NMI" );
    watcherMenu.append(*item);
    watchItems.push_back(item);

    if (isAmiga()) {
        for (auto& ri :  LIBAMI::DebuggerSnapshot::registerIdents) {
            unsigned addr = ri.vector & 0xfff;

            if (addr >= 0x01f)
                break;

            std::string _ident = ri.ident;

            if (_ident.empty())
                continue;

            GUIKIT::MenuItem* item = new GUIKIT::MenuItem();
            item->onActivate = [this, addr, _ident]() {
                auto* w = dma->legend.currentWatcher;

                if (w) {
                    unsigned _addr = (0xdff << 12) | addr;
                    emuThread->lock();
                    w->button.setStore( -1 );
                    updateTheme();
                    w->button.setStore( _addr );
                    emulator->debuggerAdd( getTheme(), DebuggerAction::DmaWatch, _addr, w->position );
                    emuThread->unlock();
                    w->button.setText( _ident );
                }
            };
            item->setText( ri.ident );
            watcherMenu.append(*item);
            watchItems.push_back(item);
        }
    }
    watcherMenu.update();

    return dma;
}

auto DmaDebugger::loadColors() -> void {
    unsigned length = isAmiga() ? std::size(LIBAMI::DebuggerSnapshot::dmaModeGroups) : std::size(LIBC64::DebuggerSnapshot::dmaModeGroups);
    const auto& entry = isAmiga() ? LIBAMI::DebuggerSnapshot::dmaModeGroups : LIBC64::DebuggerSnapshot::dmaModeGroups;

    // i == 0 is free BUS
    dmaColors[0].enabled = false;

    for (unsigned int i = 1; i < length; i++) {
        std::string _str = entry[i];
        std::string _saveIdent = saveIdent() + "_color_" + std::to_string( i );
        dmaColors[i].color = settings->get<unsigned>(_saveIdent, defaultColor[i]);
        dmaColors[i].alpha = 50;
        dmaColors[i].enabled = _str != "Cpu";
    }
}

auto DmaDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        updateView(snap);
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        updateView(snap);
    }

    scrollTimer.setEnabled(  );
}

auto DmaDebugger::updateView(LIBC64::DebuggerSnapshot& s) -> void {
    auto& canvas = dma->dmaLine.viewer;
    unsigned slots = s.lineCycles;
    canvas.setLength( slots );
    auto& logics = canvas.getDataRef();
    Emulator::Interface::DebuggerDma* dStateBefore = nullptr;
    Emulator::Interface::DebuggerDma* dStateNext = nullptr;

    for (unsigned i = 0; i < slots; i++) {
        auto& lState = logics[i];
        dStateNext = ( (i+1) == slots) ? nullptr : &s.debuggerDma[i+1];
        auto& debugState = s.debuggerDma[i];
        auto& usage = LIBC64::DebuggerSnapshot::dmaModes[ debugState.usage ];

        lState.position = i;
        lState.color = dmaColors[ usage.vector & 0xf ].color;
        lState.display = debugState.usage ? GUIKIT::LogicState::Display::SingleBlock : GUIKIT::LogicState::Display::EmptyBlock;

        lState.usage = (std::string)usage.ident;
        lState.addr = debugState.address;
        lState.data = debugState.data;

        lState.mnemonic = debugState.mnemonic;
        lState.hilight = (GUIKIT::LogicState::Hilight)debugState.hilight;

        if (debugState.usageCpu) {
            lState.display2 = GUIKIT::LogicState::Display::SingleBlock;
            if (debugState.usageCpu & 0x80)
                lState.usage2 = (std::string)LIBC64::DebuggerSnapshot::cpuAccess[ debugState.usageCpu & 0x7f ];
            else
                lState.usage2 = (std::string)LIBC64::DebuggerSnapshot::dmaModes[ debugState.usageCpu ].ident;
            lState.addr2 = debugState.addrCpu;
            lState.data2 = debugState.dataCpu;
        } else {
            lState.display2 = GUIKIT::LogicState::Display::EmptyBlock;
        }

        int j = 0;
        for (auto& data : debugState.watcher) {
            if (dma->legend.watchers[j].button.getStore() != -1) {
                bool dataChangeBefore = !dStateBefore || (dStateBefore->watcher[j] != data);
                bool dataChangeNext = !dStateNext || (dStateNext->watcher[j] != data);

                if (dataChangeBefore && dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::SingleBlock, data};
                } else if (dataChangeBefore && !dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::BeginBlock, data};
                } else if (!dataChangeBefore && dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::EndBlock, data};
                } else {
                    lState.watches[j] = {GUIKIT::LogicState::Display::KeepBlock, data};
                }
            } else
                lState.watches[j].first = GUIKIT::LogicState::Display::EmptyBlock;

            j++;
        }

        lState.active = i <= s.hPos;

        dStateBefore = &debugState;
    }

    canvas.update();
    updateControl( s.vPos, s.hPos );
}

auto DmaDebugger::updateView(LIBAMI::DebuggerSnapshot& s) -> void {
    auto& snap = s.agnus;
    auto& canvas = dma->dmaLine.viewer;
    unsigned slots = snap.lastHPos > s.hPos ? snap.lastHPos : s.hPos;
    slots += 1;
    slots &= 0xff;

    canvas.setLength( slots );
    auto& logics = canvas.getDataRef();
    Emulator::Interface::DebuggerDma* dStateBefore = nullptr;
    Emulator::Interface::DebuggerDma* dStateNext = nullptr;

    for (unsigned i = 0; i < slots; i++) {
        auto& lState = logics[i];
        dStateNext = ( (i+1) == slots) ? nullptr : &snap.debuggerDma[i+1];
        auto& debugState = snap.debuggerDma[i];
        auto& usage = LIBAMI::DebuggerSnapshot::dmaModes[ debugState.usage ];

        lState.position = i;
        lState.color = dmaColors[ usage.vector & 0xf ].color;
        lState.display = debugState.usage ? GUIKIT::LogicState::Display::SingleBlock : GUIKIT::LogicState::Display::EmptyBlock;

        lState.usage = (std::string)usage.ident;
        lState.symbolicAddr = "";
        if (debugState.usage == 1) { // CPU
            if (debugState.usageCpu == 1)
                lState.symbolicAddr = "CHIP";
            else if (debugState.usageCpu == 2)
                lState.symbolicAddr = "SLOW";
            else if (debugState.usageCpu == 6) { // register
                unsigned _rg = debugState.address & 0x1fe;
                auto& ri = LIBAMI::DebuggerSnapshot::registerIdents[_rg >> 1];
                uint8_t newRegister = (ri.vector >> 12) & 0xf;
                if ((snap.model < 4 ) && newRegister) // no OCS register
                    lState.symbolicAddr = "OpenBUS";
                else if ((snap.model == 4 ) && (newRegister & 2)) // no ECS register
                    lState.symbolicAddr = "OpenBUS";
                else
                    lState.symbolicAddr = (std::string)ri.ident;
            }
        }
        lState.addr = debugState.address;

        lState.data = debugState.data;
        lState.mnemonic = debugState.mnemonic;
        lState.hilight = (GUIKIT::LogicState::Hilight)debugState.hilight;

        if (debugState.usageCpu != 0xff) {
            lState.display2 = GUIKIT::LogicState::Display::SingleBlock;
            lState.usage2 = LIBAMI::DebuggerSnapshot::cpuAccess[debugState.usageCpu];
            lState.addr2 = debugState.addrCpu;
            lState.data2 = debugState.dataCpu;
        } else {
            lState.display2 = GUIKIT::LogicState::Display::EmptyBlock;
        }

        int j = 0;
        for (auto& data : debugState.watcher) {
            if (dma->legend.watchers[j].button.getStore() != -1) {
                bool dataChangeBefore = !dStateBefore || (dStateBefore->watcher[j] != data);
                bool dataChangeNext = !dStateNext || (dStateNext->watcher[j] != data);

                if (dataChangeBefore && dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::SingleBlock, data};
                } else if (dataChangeBefore && !dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::BeginBlock, data};
                } else if (!dataChangeBefore && dataChangeNext) {
                    lState.watches[j] = {GUIKIT::LogicState::Display::EndBlock, data};
                } else {
                    lState.watches[j] = {GUIKIT::LogicState::Display::KeepBlock, data};
                }
            } else
                lState.watches[j].first = GUIKIT::LogicState::Display::EmptyBlock;

            j++;
        }

        lState.active = i <= s.hPos;

        dStateBefore = &debugState;
    }

    canvas.update();

    updateControl( s.vPos, s.hPos );
}

auto DmaDebugger::initTheme() -> void {
    emulator->debuggerAdd( getTheme(), DebuggerAction::DmaLog, 0);

    if (dma->dmaFrame.showUsage.checked()) {
        VideoManager::getInstance( emulator )->dmaColors = &dmaColors[0];
        emulator->debuggerAdd( getTheme(), DebuggerAction::DmaView, isPaused() ? 1 : 0);
    }

    for (auto& watcher : dma->legend.watchers) {
        if (watcher.button.getStore() == -1)
            continue;

        emulator->debuggerAdd(getTheme(), DebuggerAction::DmaWatch, watcher.button.getStore(), watcher.position );
    }

    std::vector<unsigned> offsets;
    offsets.push_back(dma->legend.dma.geometry().y + (dma->legend.dma.geometry().height >> 1));
    offsets.push_back(dma->legend.dmaAddr.geometry().y + (dma->legend.dmaAddr.geometry().height >> 1));
    offsets.push_back(dma->legend.dmaData.geometry().y + (dma->legend.dmaData.geometry().height >> 1));
    offsets.push_back(dma->legend.mnemonic.geometry().y + (dma->legend.mnemonic.geometry().height >> 1));
    offsets.push_back(dma->legend.cpu.geometry().y + (dma->legend.cpu.geometry().height >> 1));
    offsets.push_back(dma->legend.cpuAddr.geometry().y + (dma->legend.cpuAddr.geometry().height >> 1));
    offsets.push_back(dma->legend.cpuData.geometry().y + (dma->legend.cpuData.geometry().height >> 1));

    for (auto& watcher : dma->legend.watchers) {
        offsets.push_back(watcher.button.geometry().y + (watcher.button.geometry().height >> 1));
    }

    int i = 0;
    for (auto& offset : offsets) {
        dma->dmaLine.viewer.setOffset( GUIKIT::LogicState::Offset(i), offset );
        i++;
    }
}

auto DmaDebugger::closeTheme() -> void {
    emulator->debuggerRemove( getTheme(), DebuggerAction::DmaView, isPaused() ? 1 : 0);
    emulator->debuggerRemove( getTheme(), DebuggerAction::DmaLog);
    VideoManager::getInstance( emulator )->dmaColors = nullptr;

    for (auto& watcher : dma->legend.watchers)
        emulator->debuggerRemove(getTheme(), DebuggerAction::DmaWatch, watcher.position );
}

auto DmaDebugger::translateTheme() -> void {
    bool showTips = showTipsItem.checked();

    if (dmaControl) {
        dmaControl->rdyButton.setText("RDY" );
        dmaControl->rdyButton.setTooltip( showTips ? trans->getA( "step next rdy" ) : "" );
        dmaControl->cycleButton.setTooltip( showTips ? trans->getA( "step next cycle" ) : "" );
    }

    dma->dmaFrame.showUsage.setText( trans->getA( "Show DMA usage" ) );

    dma->legend.dma.setText( "DMA" );
    dma->legend.dmaAddr.setText( "Addr" );
    dma->legend.dmaData.setText( "Data" );
    dma->legend.mnemonic.setText( "Mnemonic" );
    dma->legend.cpu.setText( "CPU" );
    dma->legend.cpuAddr.setText( "Addr" );
    dma->legend.cpuData.setText( "Data" );

    for (auto& watcher : dma->legend.watchers) {
        if (watcher.button.getStore() == -1)
            watcher.button.setText( trans->getA( "connect" ) );
    }
}

auto DmaDebugger::saveIdent() -> std::string {
    return "debugger_dma";
}

auto DmaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger DMA";
}

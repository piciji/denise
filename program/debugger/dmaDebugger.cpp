
#include "dmaDebugger.h"

#include "../program.h"
#include "../thread/emuThread.h"

DmaDebugger::DmaDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Debugger::Mode::DMA ) {
    build();
}

DmaDebugger::Dma::Legend::Legend::Watcher::Watcher() {
    append( button, {100u, 0u}, 0);
}

DmaDebugger::Dma::Legend::Legend() {
    dma.setAlign(GUIKIT::Label::Align::Right);
    dmaAddr.setAlign(GUIKIT::Label::Align::Right);
    dmaData.setAlign(GUIKIT::Label::Align::Right);
    cpu.setAlign(GUIKIT::Label::Align::Right);
    cpuAddr.setAlign(GUIKIT::Label::Align::Right);
    cpuData.setAlign(GUIKIT::Label::Align::Right);
    
    append( spacer, {0u, 0u}, 37 );
    append( dma, {100u, 0u}, 14 );
    append( dmaAddr, {100u, 0u}, 14 );
    append( dmaData, {100u, 0u}, 14 );
    append( cpu, {100u, 0u}, 14 );
    append( cpuAddr, {100u, 0u}, 14 );
    append( cpuData, {100u, 0u}, 16 );

    int i = 0;
    for (auto& watcher : watchers) {
        watcher.position = i++;
        append( watcher, {0u, 0u}, 13 );
    }
}

DmaDebugger::DmaControl::DmaControl() {
    append( symbolic, {0u, 0u} );
}

DmaDebugger::Dma::DmaLine::DmaLine(DmaDebugger* debugger) {
    append(viewer, {~0u, 400u});
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

    if (debugger->isAmiga()) {
        for (auto& dmaMode : LIBAMI::DebuggerSnapshot::dmaModes) {
            if (dmaMode.vector == 0)
                continue;
            auto busUsage = new BusUsage;
            busUsage->enableUsage.setText( dmaMode.ident );
            busUsage->canvas.setStore( dmaMode.vector );
            if (dmaMode.vector != 5) // CPU
                busUsage->enableUsage.setChecked();
            append( *busUsage, {0u, 0u}, 10 );
            usages.push_back(busUsage);
        }
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
    return &dmaControl;
}

auto DmaDebugger::buildTheme() -> GUIKIT::Layout* {
    dma = new Dma( this );

    control->remove( control->searchEdit );
    control->remove( control->search );

    loadColors();

    dma->dmaFrame.showUsage.onToggle = [this](bool checked) {
        emuThread->lock();
        VideoManager::getInstance( emulator )->dmaColors = checked ? &dmaColors[0] : nullptr;
        if (checked)
            emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaView, isPaused() ? 1 : 0);
        else
            emulator->debuggerRemove( DebuggerTheme::Bus, DebuggerAction::DmaView, isPaused() ? 1 : 0);
        emuThread->unlock();
    };

    for (auto& busUsage : dma->dmaFrame.usages) {
        unsigned id = busUsage->canvas.getStore();
        busUsage->canvas.setBackgroundColor( dmaColors[id].color );

        busUsage->canvas.onMouseRelease = [this, busUsage, id](GUIKIT::Mouse::Button button) {

            GUIKIT::ColorChooser colorChooser;
            colorChooser.onChoose = [this, id, busUsage](unsigned color) {
                emuThread->lock();
                dmaColors[ id ].color = color;
                settings->set<unsigned>(saveIdent() + "_color_" + std::to_string( id ), color);
                busUsage->canvas.setBackgroundColor( color );
                emuThread->unlock();
            };
            
            colorChooser.setWindow( *this );
            colorChooser.setDefault( dmaColors[ id ].color );
            auto result = colorChooser.choose();
            if (result.has_value()) {
                emuThread->lock();
                dmaColors[ id ].color = result.value();
                settings->set<unsigned>(saveIdent() + "_color_" + std::to_string( id ), result.value());
                busUsage->canvas.setBackgroundColor( result.value() );
                emuThread->unlock();
            }
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

    dmaControl.symbolic.onToggle = [this](bool checked) {
        emuThread->lock();
        settings->set<bool>(saveIdent() + "_symbolic", checked);
        dma->dmaLine.viewer.setSymbolicAddr(checked);
        updateTheme();
        scrollTimer.setEnabled( false );
        emuThread->unlock();
    };

    scrollTimer.setInterval( 100 );
    scrollTimer.onFinished = [this]() {
        dma->dmaLine.viewer.scrollToActive();
        scrollTimer.setEnabled( false );
    };

    dmaControl.symbolic.setChecked( settings->get<bool>(saveIdent() + "_symbolic", true) );
    dma->dmaLine.viewer.setSymbolicAddr(dmaControl.symbolic.checked());

    for (auto& watcher : dma->legend.watchers) {
        auto* w = &watcher;

        watcher.button.onMenu = [this, w]() {
            for (auto& w : dma->legend.watchers) {
                if (w.remove( w.edit )) {
                    w.append( w.button, {100u, 0u} );
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
                    emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaWatch, val, w->position );
                    emuThread->unlock();
                    w->button.setText( _text );
                }

                w->remove( w->edit );
                w->append( w->button, {100u, 0u} );
                w->synchronizeLayout();
            }
        };
    }

    auto* item = new GUIKIT::MenuItem();
    item->onActivate = [this]() {
        auto* w = dma->legend.currentWatcher;
        if (w) {
            w->remove( w->button );
            w->append( w->edit, {100u, 0u} );
            w->edit.setMaxLength(6);
            w->edit.setFocused();
            w->synchronizeLayout();
        }
    };
    item->setText( "<address>" );
    watcherMenu.append(*item);
    watchItems.push_back(item);

    item = new GUIKIT::MenuItem();
    item->onActivate = [this]() {
        auto* w = dma->legend.currentWatcher;

        if (w) {
            emuThread->lock();
            emulator->debuggerRemove( DebuggerTheme::Bus, DebuggerAction::DmaWatch, w->position );
            w->button.setStore( -1 );
            updateTheme();
            emuThread->unlock();
            w->button.setText( "Connect" );
        }
    };
    item->setText( "clear" );
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
            emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaWatch, 1 << 24, w->position );
            emuThread->unlock();
            w->button.setText( "IPL" );
        }
    };
    item->setText( "IPL" );
    watcherMenu.append(*item);
    watchItems.push_back(item);

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
                emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaWatch, _addr, w->position );
                emuThread->unlock();
                w->button.setText( _ident );
            }
        };
        item->setText( ri.ident );
        watcherMenu.append(*item);
        watchItems.push_back(item);
    }

    watcherMenu.update();

    return dma;
}

auto DmaDebugger::loadColors() -> void {
    if (isAmiga()) {

        for (auto& dmaMode : LIBAMI::DebuggerSnapshot::dmaModes) {
            if (dmaMode.vector == 0) {
                dmaColors[dmaMode.vector].enabled = false;
                continue;
            }

            std::string _saveIdent = saveIdent() + "_color_" + std::to_string( dmaMode.vector );
            dmaColors[dmaMode.vector].color = settings->get<unsigned>(_saveIdent, defaultColor[dmaMode.vector]);
            dmaColors[dmaMode.vector].alpha = 50;
            dmaColors[dmaMode.vector].enabled = dmaMode.vector != 5;
        }
    }
}

auto DmaDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        updateView(snap);
    }

    scrollTimer.setEnabled(  );
}

auto DmaDebugger::updateView(LIBAMI::DebuggerSnapshot& s) -> void {
    auto& snap = s.agnus;
    auto& canvas = dma->dmaLine.viewer;
    unsigned slots = snap.lastHPos > s.hPos ? snap.lastHPos : s.hPos;
    slots += 1;
    slots &= 0xff;

    canvas.setLength( slots );
    auto& logics = canvas.getDataRef();
    bool symbolic = dmaControl.symbolic.checked();
    Emulator::Interface::DebuggerDma* dStateBefore = nullptr;
    Emulator::Interface::DebuggerDma* dStateNext = nullptr;
    unsigned watcherStates = 0;
    int i = 0;
    for (auto& watcher : dma->legend.watchers) {
        if (watcher.button.getStore() != -1) {
            watcherStates |= (1 << i);
        }
        i++;
    }

    if (snap.debuggerDma) {
        for (unsigned i = 0; i < slots; i++) {
            auto& lState = logics[i];
            dStateNext = ( (i+1) == slots) ? nullptr : &snap.debuggerDma[i+1];
            auto& debugState = snap.debuggerDma[i];

            lState.position = i;
            lState.color = dmaColors[ debugState.usage & 0xf ].color;
            lState.display = debugState.usage ? GUIKIT::LogicState::Display::SingleBlock : GUIKIT::LogicState::Display::EmptyBlock;

            lState.usage = (std::string)LIBAMI::DebuggerSnapshot::dmaModesShort[ debugState.usage ].ident;
            if (symbolic) {
                if (debugState.usage == 5) { // CPU
                    if (debugState.mapper == 1)
                        lState.symbolicAddr = "CHIP";
                    else if (debugState.mapper == 2)
                        lState.symbolicAddr = "SLOW";
                    else if (debugState.mapper == 6) { // register
                        unsigned _rg = debugState.address & 0x1fe;
                        auto& ri = LIBAMI::DebuggerSnapshot::registerIdents[_rg >> 1];
                        uint8_t newRegister = (ri.vector >> 12) & 0xf;
                        if ((snap.model < 4 ) && newRegister) // no OCS register
                            lState.symbolicAddr = "OpenBUS";
                        else if ((snap.model == 4 ) && (newRegister & 2)) // no ECS register
                            lState.symbolicAddr = "OpenBUS";
                        else
                            lState.symbolicAddr = (std::string)ri.ident;
                    } else
                        lState.symbolicAddr = "";
                } else {
                    if ((debugState.address & snap.chipMemMask) != debugState.address)
                        lState.symbolicAddr = "SLOW";
                    else
                        lState.symbolicAddr = "CHIP";
                }
            } else
                lState.addr = debugState.address;

            lState.data = debugState.data;

            if (debugState.mapper != 0xff) {
                lState.display2 = GUIKIT::LogicState::Display::SingleBlock;
                lState.usage2 = LIBAMI::DebuggerSnapshot::cpuAccess[debugState.mapper];
                lState.addr2 = debugState.addrCpu;
                lState.data2 = debugState.dataCpu;
            } else {
                lState.display2 = GUIKIT::LogicState::Display::EmptyBlock;
            }

            int j = 0;
            for (auto& data : debugState.watcher) {
                if (watcherStates & (1 << j)) {
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
    }
    canvas.update();

    updateControl( s.vPos, s.hPos );
}

auto DmaDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaLog, 0);

    if (dma->dmaFrame.showUsage.checked()) {
        VideoManager::getInstance( emulator )->dmaColors = &dmaColors[0];
        emulator->debuggerAdd( DebuggerTheme::Bus, DebuggerAction::DmaView, isPaused() ? 1 : 0);
    }

    for (auto& watcher : dma->legend.watchers) {
        if (watcher.button.getStore() == -1)
            continue;

        emulator->debuggerAdd(DebuggerTheme::Bus, DebuggerAction::DmaWatch, watcher.button.getStore(), watcher.position );
    }

    std::vector<unsigned> offsets;


    offsets.push_back(dma->legend.dma.geometry().y + (dma->legend.dma.geometry().height >> 1));
    offsets.push_back(dma->legend.dmaAddr.geometry().y + (dma->legend.dmaAddr.geometry().height >> 1));
    offsets.push_back(dma->legend.dmaData.geometry().y + (dma->legend.dmaData.geometry().height >> 1));
    offsets.push_back(dma->legend.cpu.geometry().y + (dma->legend.cpu.geometry().height >> 1));
    offsets.push_back(dma->legend.cpuAddr.geometry().y + (dma->legend.cpuAddr.geometry().height >> 1));
    offsets.push_back(dma->legend.cpuData.geometry().y + (dma->legend.cpuData.geometry().height >> 1));

    for (auto& watcher : dma->legend.watchers) {
        offsets.push_back(watcher.button.geometry().y + (watcher.button.geometry().height >> 1));
    }

    dma->dmaLine.viewer.setOffsets(offsets);
}

auto DmaDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Bus, DebuggerAction::DmaView, isPaused() ? 1 : 0);
    emulator->debuggerRemove( DebuggerTheme::Bus, DebuggerAction::DmaLog);
    VideoManager::getInstance( emulator )->dmaColors = nullptr;

    for (auto& watcher : dma->legend.watchers)
        emulator->debuggerRemove(DebuggerTheme::Bus, DebuggerAction::DmaWatch, watcher.position );
}

auto DmaDebugger::translateTheme() -> void {
    dmaControl.symbolic.setText( "Symbolic" );

    dma->dmaFrame.showUsage.setText( "Show DMA Usage" );

    dma->legend.dma.setText( "DMA" );
    dma->legend.dmaAddr.setText( "Addr" );
    dma->legend.dmaData.setText( "Data" );
    dma->legend.cpu.setText( "CPU" );
    dma->legend.cpuAddr.setText( "Addr" );
    dma->legend.cpuData.setText( "Data" );

    for (auto& watcher : dma->legend.watchers) {
        watcher.button.setText( "Connect" );
    }
}

auto DmaDebugger::saveIdent() -> std::string {
    return "debugger_dma";
}

auto DmaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger DMA";
}

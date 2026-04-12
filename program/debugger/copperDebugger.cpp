
#include "copperDebugger.h"

#include <cstring>

#include "copperDebugger.h"

#include "../program.h"
#include "../../emulation/libami/interface.h"
#include "../../emulation/libami/system/debuggerSnapshot.h"
#include "../thread/emuThread.h"

CopperDebugger::Copper::List::Control::Control() {
    append(labelCopLc, {0u, 0u}, 10);
    append(copLc, {70u, 0u});

    append(spacer, {~0u, 0u});
    append(addrEdit, {80u, 0u}, 10);
    append(addrView, {0u, 0u}, 10);
    append(valueEdit, {80u, 0u}, 10);
    append(valueView, {0u, 0u}, 10);

    copLc.setAlign( GUIKIT::LineEdit::Align::Right );
    copLc.setEditable( false );
    setAlignment( 0.5 );
}

CopperDebugger::Copper::List::List() {
    listView.setHeaderText( { "", "address","instruction" } );
    listView.setFont( GUIKIT::Font::monospace() );
    listView.setHeaderVisible( true );

    append( listView, {~0u, ~0u}, 10 );
    append( control, {~0u, 0u} );
    append( spacer, {~0u, 10u} );
}

CopperDebugger::Copper::Watcher::Adder::Adder() {
    append(address, {~0u, 0u}, 10u);
    append(add, {0u, 0u});

    setAlignment( 0.5 );
}

CopperDebugger::Copper::Watcher::TypeLayout::TypeLayout() {
    append( breakPoint, {0u, 0u}, 10 );
    append( watchPoint, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( breakPoint, watchPoint );

    setAlignment( 0.5 );
}

CopperDebugger::Copper::Watcher::Control::Control() {
    append(labelCopPc, {0u, 0u}, 10u);
    append(copPc, {50u, 0u}, 10u);
    append(cdang, {0u, 0u});

    cdang.setReadonly(  );
    copPc.setEditable( false );
    setAlignment( 0.5 );
}

CopperDebugger::Copper::Watcher::Watcher() {
    listView.setHeaderText( { "", "","", "" } );

    append( listView, {~0u, ~0u}, 10 );
    append( typeLayout, {0u, 0u}, 10 );
    append( adder, {~0u, 0u}, 10 );
    append( control, {~0u, 0u} );
}

CopperDebugger::Copper::Copper() {
    append( lists[0], {~0u, ~0u}, 10 );
    append( lists[1], {~0u, ~0u}, 10 );
    append( watcher, {210u, ~0u} );
}

CopperDebugger::CopperControl::CopperControl() {
    append( softStopButton, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    append( symbolic, {0u, 0u} );
    setAlignment( 0.5 );
}

CopperDebugger::CopperDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Debugger::Mode::Copper ) {
    build();
}

CopperDebugger::~CopperDebugger() {
    if (copperControl) {
        if (control)
            control->remove( *copperControl );
        delete copperControl;
    }
}

auto CopperDebugger::updateInstructionBreakpointVisualsInOtherList(Copper::List* lPtr, unsigned addr, DbgWatcher* watcher) -> void {
    for (auto& list : copper->lists) {
        if (&list != lPtr) {
            auto row = findInstructionRowBy( &list, addr );
            if (row.has_value())
                updateInstructionBreakpointVisuals(list.listView, row.value_or( 0 ), watcher);
        }
    }
}

auto CopperDebugger::buildTheme() -> GUIKIT::Layout* {
    copper = new Copper;
    copper->watcher.adder.add.setImage( &addImg );

    watcherHelper.debugger = this;
    watcherHelper.watcherList = &copper->watcher.listView;

    for (auto& list : copper->lists) {
        Copper::List* lPtr = &list;

        list.listView.onClick = [this, lPtr](unsigned row, unsigned column) {
            if (column == 0) {
                emuThread->lock();
                auto& inst = lPtr->instructions[row];

                DbgWatcher* watcher = watcherHelper.findBy(inst.addr, DebuggerAction::BreakpointCopper);
                if (!watcher) {
                    watcherHelper.addToList( inst.addr, DebuggerAction::BreakpointCopper );
                    watcher = watcherHelper.findBy(inst.addr, DebuggerAction::BreakpointCopper);
                    emulator->debuggerAdd(DebuggerTheme::Copper, DebuggerAction::BreakpointCopper, inst.addr);
                } else {
                    watcher->enabled ^= 1;
                    if (watcher->enabled) {
                        emulator->debuggerAdd(DebuggerTheme::Copper, DebuggerAction::BreakpointCopper, inst.addr);
                        updateWatchpointCondition( *watcher );
                    } else
                        emulator->debuggerRemove(DebuggerTheme::Copper, DebuggerAction::BreakpointCopper, inst.addr);
                }
                watcherHelper.updateList();
                emuThread->unlock();
                updateInstructionBreakpointVisuals(lPtr->listView, row, watcher);

                updateInstructionBreakpointVisualsInOtherList(lPtr, inst.addr, watcher);

              //  if (lPtr->currentInstRow.has_value())
                //    lPtr->listView.setSelection( lPtr->currentInstRow.value_or(0) );
            } else if (isPaused()) {
                if (row < lPtr->instructions.size()) {
                    auto& inst = lPtr->instructions[row];
                    lPtr->control.addrEdit.setText( GUIKIT::String::convertToHex( inst.addr ) );
                }
            }
        };

        list.listView.onContext = [this, lPtr](unsigned row, unsigned column, GUIKIT::Position position ) {
            if (column == 0) {
                auto& inst = lPtr->instructions[row];
                DbgWatcher* watcher = watcherHelper.findBy(inst.addr, DebuggerAction::BreakpointCopper);
                if (watcher)
                    openConditionView( watcher, position);

             //   if (lPtr->currentInstRow.has_value())
               //     lPtr->listView.setSelection( lPtr->currentInstRow.value_or(0) );
            }
        };

        list.control.addrView.setImage( &searchImg );
        list.control.valueView.setImage( &editImg );

        list.control.addrEdit.onReturn = [lPtr]() {
            lPtr->control.addrView.onClick();
        };

        list.control.valueEdit.onReturn = [lPtr]() {
            lPtr->control.valueView.onClick();
        };

        list.control.addrView.onClick = [this, lPtr]() {
            if (emulator != activeEmulator)
                return;

            std::string addressText = lPtr->control.addrEdit.text();
            if (addressText.empty())
                return;

            int address = GUIKIT::String::convertHexToInt(addressText, -1);
            if (address == -1)
                return;

            searchAddress(lPtr, address);
        };

        list.control.valueView.onClick = [this, lPtr]() {
            auto addrStr = lPtr->control.addrEdit.text();
            auto valStr = lPtr->control.valueEdit.text();

            changeMemory( addrStr, valStr );
        };
    }

    copper->watcher.listView.onContext = [this](unsigned row, unsigned column, GUIKIT::Position position ) {
        if (column == 0) {
            if (row >= watcherHelper.elements())
                return;

            auto& watcher = watcherHelper.getWatcher(row);
            openConditionView(&watcher, position);
        }
    };

    copper->watcher.listView.onClick = [this](unsigned row, unsigned column) {
        if (row >= watcherHelper.elements())
            return;

        if (column != 0 && column != 3)
            return;

        emuThread->lock();
        auto& watcher = watcherHelper.getWatcher(row);
        if (column == 0) {
            watcher.enabled ^= 1;
            watcherHelper.updateBreakpointVisuals(row, &watcher);
            if (watcher.enabled) {
                emulator->debuggerAdd(DebuggerTheme::Copper, watcher.action, watcher.addr);
                updateWatchpointCondition( watcher );
            } else
                emulator->debuggerRemove(DebuggerTheme::Copper, watcher.action, watcher.addr);
        }

        for (auto& list : copper->lists) {
            Copper::List* lPtr = &list;

            std::optional<unsigned> instRow = std::nullopt;
            if (watcher.action == DebuggerAction::BreakpointCopper)
                instRow = findInstructionRowBy(lPtr, watcher.addr);

            if (!instRow.has_value())
                continue;

            if (column == 0) {
                updateInstructionBreakpointVisuals(lPtr->listView, instRow.value_or(0), &watcher);
            } else if (column == 3) {
                removeInstructionBreakpointVisuals(lPtr->listView, instRow.value_or(0));
            }
        }

        if (column == 3) {
            emulator->debuggerRemove( DebuggerTheme::Copper, watcher.action, watcher.addr);
            watcherHelper.removeFromList(watcher.addr, watcher.action);
            watcherHelper.updateList();
        }

        emuThread->unlock();
    };

    copper->watcher.adder.add.onActivate = [this]() {
        copper->watcher.adder.address.onReturn();
    };

    copper->watcher.adder.address.onReturn = [this]() {
        std::string addressText = copper->watcher.adder.address.text();
        if (addressText.empty())
            return;

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        DebuggerAction action = DebuggerAction::BreakpointCopper;
        if (copper->watcher.typeLayout.watchPoint.checked()) {
            action = DebuggerAction::WatchpointCopper;
        }

        if (watcherHelper.findBy( address, action ))
            return;

        emuThread->lock();
        watcherHelper.addToList( static_cast<unsigned>(address), action );
        watcherHelper.updateList();

        if (action == DebuggerAction::BreakpointCopper) {
            for (auto& list : copper->lists) {
                Copper::List* lPtr = &list;

                auto instRow = findInstructionRowBy(lPtr, static_cast<unsigned>(address));
                if (instRow.has_value())
                    updateInstructionBreakpointVisuals(lPtr->listView, instRow.value_or(0), watcherHelper.findBy( address, action ));
            }
        }

        emulator->debuggerAdd(DebuggerTheme::Copper, action, address);
        emuThread->unlock();
    };

    if (copperControl)
        copperControl->symbolic.setChecked( settings->get<bool>(saveIdent() + "_symbolic", true) );

    return copper;
}

auto CopperDebugger::translateTheme() -> void {
    bool showTips = showTipsItem.checked();

    copper->watcher.adder.address.setPlaceholder( trans->getA( "address" ) );
    copper->watcher.typeLayout.breakPoint.setText( trans->getA( "instruction" ) );
    copper->watcher.typeLayout.watchPoint.setText( trans->getA( "register access" ) );
    copper->watcher.control.labelCopPc.setText( "COP-PC" );
    copper->watcher.control.cdang.setText( "CDANG" );
    copper->watcher.adder.add.setTooltip( showTips ? trans->getA( "complex conditions tooltip") : "" );

    copper->lists[0].control.labelCopLc.setText( "COP1LC" );
    copper->lists[1].control.labelCopLc.setText( "COP2LC" );

    copper->lists[0].listView.setHeaderText( {"", trans->getA( "address"), trans->getA( "instruction") } );
    copper->lists[1].listView.setHeaderText( {"", trans->getA( "address"), trans->getA( "instruction") } );

    copper->lists[0].control.addrEdit.setPlaceholder( trans->getA( "address" )  );
    copper->lists[1].control.addrEdit.setPlaceholder( trans->getA( "address" )  );
    copper->lists[0].control.valueEdit.setPlaceholder( trans->getA( "value" )  );
    copper->lists[1].control.valueEdit.setPlaceholder( trans->getA( "value" )  );

    copperControl->symbolic.setText( trans->getA("symbolic") );
    copperControl->softStopButton.setTooltip( showTips ? trans->getA( "step next copper" ) : "" );
}

auto CopperDebugger::updateTheme() -> void {
    if (emulator != activeEmulator || !snapshot)
        return;

    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.copper;

    updateReg( copper->lists[0].control.copLc, s.cop1LC );
    updateReg( copper->lists[1].control.copLc, s.cop2LC );

    updateReg( copper->watcher.control.copPc, s.copPC );
    updateReg( copper->watcher.control.cdang, s.cdang );

    updateControl( snap.vPos, snap.hPos );

    updateInstructionViews();

    updateWatcherSelection();
}

auto CopperDebugger::updateInstructionViews(bool forceUpdate) -> void {
    if (!snapshot)
        return;

    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.copper;

    for (auto& list : copper->lists) {
        if (list.inUse) {
            if (list.dirty) {
                list.dirty = false;
                if (!list.instructions.empty() || list.memory)
                    updateInstructionList(&list, forceUpdate);
            }

            list.currentInstRow = findInstructionRowBy( &list, s.copPCEdge );
            if (list.currentInstRow.has_value())
                list.listView.setSelection( list.currentInstRow.value_or(0) );
            else if (list.listView.selected())
                list.listView.setSelected( false );
        } else if (list.listView.selected()) {
            list.listView.setSelected( false );
            list.currentInstRow = std::nullopt;
        }
    }
}

auto CopperDebugger::prepareTheme(bool external) -> void {
    if (!snapshot)
        return;

    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.copper;

    int listNr = 1;
    for (auto& list : copper->lists) {
        if (s.listUse == 0 || (listNr == 1 && !s.list1) || (listNr == 2 && !s.list2)) {
            // empty list after reset
            delete[] list.memory;
            list.memory = nullptr;
            list.memorySize = 0;
            list.dirty = true;
            list.inUse = true;
            list.startAddr = 0;

        } else {
            list.inUse = external || (listNr == 1 && s.listUse == 1) || (listNr == 2 && s.listUse == 2);

            if (list.inUse) {
                auto* range = listNr == 1 ? s.list1 : s.list2;
                unsigned startAddr = range->first;
                unsigned endAddr = range->second;
                unsigned memorySize = endAddr - startAddr;
                uint8_t* data = dynamic_cast<LIBAMI::Interface*>(emulator)->getCopperDump( startAddr, endAddr );

                if (!list.memory || (startAddr != list.startAddr) || (memorySize != list.memorySize) || std::memcmp(data, list.memory, memorySize) != 0) {
                    delete[] list.memory;
                    list.memory = data;
                    list.memorySize = memorySize;
                    list.dirty = true;
                } else
                    delete[] data;

                list.startAddr = startAddr;
            }
        }
        listNr++;
    }
}

auto CopperDebugger::buildControl() -> GUIKIT::Layout* {
    copperControl = new CopperControl();

    copperControl->softStopButton.setImage( &nextImg );
    copperControl->softStopButton.onActivate = [this]() {
        if (emulator != activeEmulator)
            return;
        emuThread->lock();
        timerVisibility->setEnabled();
        emulator->debuggerAdd( DebuggerTheme::Copper, DebuggerAction::SoftstopCopper, 0 );
        emuThread->unlockDebugger();
        emuThread->unlock();
    };

    copperControl->symbolic.onToggle = [this](bool checked) {
        settings->set<bool>(saveIdent() + "_symbolic", checked);
        emuThread->lock();
        for (auto& list : copper->lists) {
            list.dirty = true;
            list.inUse = true;
        }
        updateInstructionViews(true);
        emuThread->unlock();
    };

    return copperControl;
}

auto CopperDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Copper, DebuggerAction::None, 0);

    for (auto& watcher : watcherHelper.watchers) {
        if (watcher.enabled) {
            emulator->debuggerAdd( DebuggerTheme::Copper, watcher.action, watcher.addr );
            updateWatchpointCondition(watcher);
        } else
            emulator->debuggerRemove( DebuggerTheme::Copper, watcher.action, watcher.addr );
    }
}

auto CopperDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Copper, DebuggerAction::None);
    emulator->debuggerRemove( DebuggerTheme::Copper, DebuggerAction::BreakpointCopper );
    emulator->debuggerRemove( DebuggerTheme::Copper, DebuggerAction::WatchpointCopper );
    emulator->debuggerRemove( DebuggerTheme::Copper, DebuggerAction::SoftstopCopper );
}

auto CopperDebugger::saveIdent() -> std::string {
    return "debugger_copper";
}

auto CopperDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Copper";
}

auto CopperDebugger::updateBreakpointVisuals(DbgWatcher* watcher) -> void {
    for (auto& list : copper->lists) {
        Copper::List* lPtr = &list;

        std::optional<unsigned> instRow = findInstructionRowBy(lPtr, watcher->addr);

        if (instRow.has_value())
            updateInstructionBreakpointVisuals(lPtr->listView, instRow.value_or(0), watcher);
    }

    std::optional<unsigned> instRow = watcherHelper.findRowBy( watcher->addr, watcher->action );

    if (instRow.has_value())
        watcherHelper.updateBreakpointVisuals(instRow.value_or(0), watcher);
}

auto CopperDebugger::findInstructionRowBy(Copper::List* list, unsigned addr) -> std::optional<unsigned> {
    auto size = list->instructions.size();

    for (unsigned i = 0; i < size; i++) {
        auto& inst = list->instructions[i];
        if (inst.addr == addr)
            return i;
    }
    return std::nullopt;
}

auto CopperDebugger::updateInstructionList(Copper::List* list, bool forceUpdate) -> void {
    auto& instructionList = list->listView;
    unsigned rows = instructionList.rowCount();
    unsigned size = list->memorySize / 4;
    unsigned addr = list->startAddr;
    uint16_t data1;
    uint16_t data2;
    std::string disassembled;
    bool symbolic = copperControl->symbolic.checked();
    list->instructions.resize(size);

    instructionList.lockRedraw();
    for (unsigned i = 0; i < size; i++) {
        auto& inst = list->instructions[i];
        data1 = *(uint16_t*)(list->memory + i * 4);
        data2 = *(uint16_t*)(list->memory + i * 4 + 2);

        if (forceUpdate || i >= rows || inst.addr != addr || inst.data1 != data1 || inst.data2 != data2) {
            if (symbolic) {
                if (data1 & 1) {
                    disassembled = data2 & 1 ? "SKIP" : "WAIT";

                    if ((data2 & 0x8000) == 0)
                        disassembled += "B";

                    uint8_t vMask = (data2 | 0x8000) >> 8;
                    uint8_t hMask = data2 & 0xfe;

                    uint8_t vPos = data1 >> 8;
                    uint8_t hPos = data1 & 0xfe;

                    disassembled += " ($" + GUIKIT::String::convertToHex(vPos, 2) + ",$" + GUIKIT::String::convertToHex(hPos, 2) + ")";

                    if (vMask != 0xff || hMask != 0xfe)
                        disassembled += " M($" + GUIKIT::String::convertToHex(vMask, 2) + ",$" + GUIKIT::String::convertToHex(hMask, 2) + ")";
                } else {
                    auto& ri = LIBAMI::DebuggerSnapshot::registerIdents[(data1 & 0x1fe) >> 1];
                    std::string _ident = ri.ident;
                    disassembled = "MOVE " + (_ident.empty() ? "NO-OP" : _ident)  + ", " + GUIKIT::String::convertToHex( data2, 4 );
                }

            } else
                disassembled = "dc.w " + GUIKIT::String::convertToHex( data1, 4 ) + ", " + GUIKIT::String::convertToHex( data2, 4 );

            instructionList.resetRowForegroundColor( i );

            if (i >= rows)
                instructionList.append({ "", GUIKIT::String::convertToHex( addr, 6), disassembled }, true);
            else {
                instructionList.setText( i, 1, GUIKIT::String::convertToHex( addr, 6), true );
                instructionList.setText( i, 2, disassembled, true );
            }

            auto breakPoint = watcherHelper.findBy( addr, DebuggerAction::BreakpointCopper );
            if (!breakPoint) {
                instructionList.setImage( i, 0, nullImg, true );
            } else {
                updateInstructionBreakpointVisuals(instructionList, i, breakPoint, true);
            }

            inst.addr = addr;
            inst.data1 = data1;
            inst.data2 = data2;
        }

        addr += 4;
    }

    if (!size)
        instructionList.reset();
    else if (size < rows) {
        unsigned toDelete = rows - size;
        while (toDelete) {
            toDelete--;
            instructionList.remove( size + toDelete, true );
        }
    }

    instructionList.autoSizeColumns();
    instructionList.unlockRedraw();
}

auto CopperDebugger::updateWatcherSelection() -> void {
    auto& watcherList = copper->watcher.listView;
    if (snapshot->callbackAction == DebuggerAction::WatchpointCopper
        || snapshot->callbackAction == DebuggerAction::BreakpointCopper) {
        auto row = watcherHelper.findRowBy(snapshot->callbackAddress, snapshot->callbackAction);
        if (row.has_value())
            watcherList.setSelection( row.value_or(0) );
        else if (watcherList.selected())
            watcherList.setSelected( false );
    } else if (watcherList.selected())
        watcherList.setSelected( false );
}

auto CopperDebugger::searchAddress(Copper::List* list, unsigned addr) -> void {
    auto instRow = findInstructionRowBy(list, addr);
    if (instRow.has_value())
        list->listView.setSelection( instRow.value_or(0) );
}

auto CopperDebugger::memChanged() -> void {
    prepareTheme(true);
    updateTheme();
}
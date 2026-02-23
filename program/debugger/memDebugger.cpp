#include "memDebugger.h"
#include "cpuDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"
#include "../tools/macros.h"
#include <cstring>

MemDebugger::MemDebugger( Emulator::Interface* emulator, Mode mode )
: Debugger( emulator, mode ) {

}

MemDebugger::MemDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Mode::Memory ) {
    build();
}

MemDebugger::~MemDebugger() {
    delete[] memDump;
    delete[] memDumpOld;
}

MemDebugger::Memory::Memory(Debugger* debugger)
: options( debugger ) {
    bankList.setHeaderText( { "bank", "mapping" } );
    bankList.setFont( GUIKIT::Font::monospace( 10  ) );
    pageList.setFont( GUIKIT::Font::monospace( 10 ) );
    bankList.setHeaderVisible( true );
    #define AL GUIKIT::ListView::Align::Left
    #define AR GUIKIT::ListView::Align::Right
    #define AC GUIKIT::ListView::Align::Center

    if (debugger->isAmiga()) {
        pageList.setHeaderText( { "address", "0","2", "4", "6", "8", "A", "C", "E", "ASCII" });
        pageList.setAlignment( {AL, AR, AR, AR, AR, AR, AR, AR, AR}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++) {
            bankList.append({GUIKIT::String::convertToHex(i, 2), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x1000; i++)
            pageList.append({GUIKIT::String::convertToHex(i * 16, 4), "   0", "   0", "   0", "   0", "   0", "   0", "   0", "   0", "................"}, true);
        pageList.unlockRedraw();
    } else if (debugger->mode == Mode::MemorySCPU) {
        pageList.setHeaderText( { "address", "0","1", "2", "3", "4", "5", "6", "7", "8","9", "A", "B", "C", "D", "E","F", "ASCII" });
        pageList.setAlignment( {AL, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++) {
            bankList.append({GUIKIT::String::convertToHex(i, 2), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x1000; i++)
            pageList.append({GUIKIT::String::convertToHex(i * 16, 4), "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "................"}, true);
        pageList.unlockRedraw();
    } else {
        pageList.setHeaderText( { "address", "0","1", "2", "3", "4", "5", "6", "7", "8","9", "A", "B", "C", "D", "E","F", "ASCII" });
        pageList.setAlignment( {AL, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC, AC}, true );

        bankList.lockRedraw();
        for (unsigned i = 0; i < 0x10; i++) {
            bankList.append({GUIKIT::String::convertToHex(i, 1), "Unmapped" }, true);
            bankList.setRowForegroundColor( UNUSED_COLOR, i );
        }
        bankList.unlockRedraw();

        pageList.lockRedraw();
        for (unsigned i = 0; i < 0x100; i++)
            pageList.append({GUIKIT::String::convertToHex(i * 16, 3), "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "................"}, true);
        pageList.unlockRedraw();
    }

    pageList.setHeaderVisible( true );

    bankList.setAlignment( {AR, AL}, true );
    bankList.autoSizeColumns();
    pageList.autoSizeColumns();

    append(bankList, {200u, ~0u}, 10);
    append(pageList, {~0u, ~0u}, 10);
    append(options, {0u, 0u} );
}

MemDebugger::Memory::Options::Address::Address(Debugger* debugger) {
    edit.setMaxLength( debugger->isAmiga() || debugger->mode == Mode::MemorySCPU ? 6 : 4 );
    append(edit, {80u, 0u}, 5);
    append(view, {0u, 0u});
    setAlignment( 0.5 );
}

MemDebugger::Memory::Options::Value::Value(Debugger* debugger) {
    append(edit, {80u, 0u}, 5);
    append(view, {0u, 0u});
    setAlignment( 0.5 );
}

MemDebugger::Memory::Options::Options(Debugger* debugger)
: address( debugger ), value( debugger ) {
    append( address, {0u, 0u}, 10 );
    append( value, {0u, 0u}, 10 );
}

MemDebugger::C64MemControl::Element::Element(Debugger* debugger) {
    imgView.setStore( 0 );
    append(imgView, {0u, 0u}, 2);
    append(label, {0u, 0u});
    setAlignment( 0.5 );
}

MemDebugger::C64MemControl::Left::Left(Debugger* debugger)
: exrom( debugger ), game( debugger ) {
    exrom.label.setText( "EXROM" );
    game.label.setText( "GAME" );
    append(exrom, {0u, 0u}, 2);
    append(game, {0u, 0u});
}

MemDebugger::C64MemControl::Middle::Middle(Debugger* debugger)
: charen( debugger ) {
    charen.label.setText( "CHAREN" );
    append(charen, {0u, 0u});
}

MemDebugger::C64MemControl::Right::Right(Debugger* debugger)
: loram( debugger ), hiram( debugger ) {
    loram.label.setText( "LORAM" );
    hiram.label.setText( "HIRAM" );
    append(loram, {0u, 0u}, 2);
    append(hiram, {0u, 0u});
}

MemDebugger::C64MemControl::C64MemControl(Debugger* debugger)
: left(debugger), right(debugger), middle( debugger ) {
    append(left, {0u, 0u}, 10);
    append(middle, {0u, 0u}, 10);
    append(right, {0u, 0u});

    setAlignment( 0.5 );
}

auto MemDebugger::buildControl() -> GUIKIT::Layout* {
    if (isC64()) {
        c64MemControl = new C64MemControl(this);
        return c64MemControl;
    }
    return nullptr;
}

auto MemDebugger::searchAddress(unsigned addr) -> void {
    if (mode == Mode::Memory) {
        if (isAmiga()) {
            uint8_t bank = (addr >> 16) & 0xff;
            memory->bankList.setSelection( bank );
            uint16_t page = addr & 0xffff;
            memory->pageList.setSelection( page / 16 );
        } else {
            uint8_t bank = (addr >> 12) & 0xff;
            memory->bankList.setSelection( bank );
            uint16_t page = addr & 0xfff;
            memory->pageList.setSelection( page / 16 );
        }
    } else if (mode == Mode::MemorySCPU) {
        uint8_t bank = (addr >> 16) & 0xff;
        memory->bankList.setSelection( bank );
        uint16_t page = addr & 0xffff;
        memory->pageList.setSelection( page / 16 );
    }
    memory->bankList.onChange();
}

auto MemDebugger::buildTheme() -> GUIKIT::Layout* {
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

    memory->options.address.view.setImage( &searchImg );
    memory->options.value.view.setImage( &editImg );

    memory->options.address.edit.onReturn = [this]() {
        memory->options.address.view.onClick();
    };

    memory->options.value.edit.onReturn = [this]() {
        memory->options.value.view.onClick();
    };

    memory->options.address.view.onClick = [this]() {
        if (emulator != activeEmulator)
            return;

        std::string addressText = memory->options.address.edit.text();
        if (addressText.empty())
            return;

        int address = GUIKIT::String::convertHexToInt(addressText, -1);
        if (address == -1)
            return;

        searchAddress(address);
    };

    memory->options.value.view.onClick = [this]() {
        auto valStr = memory->options.value.edit.text();
        auto addrStr = memory->options.address.edit.text();

        changeMemory( addrStr, valStr );
    };

    memory->bankList.onChange = [this]() {
        emuThread->lock();
        memChanged();
        emuThread->unlock();
    };

    memory->pageList.onClick = [this](unsigned row, unsigned col) {
        if (col == 0)
            return;

        uint32_t addr = memory->bankList.selection();

        if (isAmiga()) {
            if (col == 9)
                return;

            addr = (addr & 0xff) << 16;
            addr |= row * 16 + ((col - 1) << 1);

        } else {
            if (col == 17)
                return;

            if (mode == Mode::MemorySCPU) {
                addr = (addr & 0xff) << 16;
                addr |= row * 16 + (col - 1);
            } else {
                addr = (addr & 0xf) << 12;
                addr |= row * 16 + (col - 1);
            }
        }

        memory->options.address.edit.setText( GUIKIT::String::convertToHex( addr ) );
    };

    std::memset(bankListStore, 0, sizeof(bankListStore));
    memory->bankList.setSelected();
    memory->pageList.setSelected();

    return memory;
}

auto MemDebugger::translateTheme() -> void {
    bool showTips = showTipsItem.checked();
    memory->bankList.setHeaderText( {trans->getA( "address"), trans->getA( "assignment") } );
    auto header = memory->pageList.state.header;
    header[0] = trans->getA( "address" );;
    memory->pageList.setHeaderText(header);
    memory->options.address.edit.setPlaceholder( trans->getA( "address" )  );
    memory->options.value.edit.setPlaceholder( trans->getA( "value" )  );
    memory->options.value.edit.setTooltip( showTips ? trans->getA( "edit memory tooltip") : "" );
}

auto MemDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        updateMemory( snap);
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        if (mode == Mode::Memory) {
            updateMemory( snap);
        } else if (snap.superCpu && mode == Mode::MemorySCPU) {
            updateMemory( snap);
        }
    }
}

auto MemDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Memory, DebuggerAction::None, 0);
}

auto MemDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Memory, DebuggerAction::None);
}

auto MemDebugger::prepareTheme() -> void {
    isAmiga() ? fetchDump<uint16_t>() : fetchDump<uint8_t>();
}

auto MemDebugger::memChanged(bool noColorChanges) -> void {
    unsigned selectedBank = memory->bankList.selection();
    if (isAmiga()) {
        fetchDump<uint16_t>();
        loadMemoryBank<uint16_t>( selectedBank, noColorChanges );
    } else {
        fetchDump<uint8_t>();
        loadMemoryBank<uint8_t>( selectedBank, noColorChanges );
    }
}

auto MemDebugger::updateMemory(LIBAMI::DebuggerSnapshot& s) -> void {
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

    updateControl( s.vPos, s.hPos );
}

auto MemDebugger::updateMemory(LIBC64::DebuggerSnapshot& s) -> void {
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

    updateControl( s.vPos, s.hPos );

    updateC64MemControl(s.mode);
}

auto MemDebugger::updateC64MemControl(uint8_t _mode, bool init) -> void {
    unsigned i = 0;
    C64MemControl::Element* elements[] = {
        &c64MemControl->left.exrom, &c64MemControl->left.game,
        &c64MemControl->middle.charen, &c64MemControl->right.loram, &c64MemControl->right.hiram
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

template<typename T>
auto MemDebugger::fetchDump() -> void {
    auto& bankList = memory->bankList;
    unsigned selectedBank = 0;
    if (bankList.selected())
        selectedBank = bankList.selection();

    auto* pNew = reinterpret_cast<T*>(memDump);

    if (isC64() && (mode == Mode::Memory))
        emulator->getMemoryDumpPage( selectedBank, reinterpret_cast<uint8_t*>(pNew) );
    else {
        emulator->getMemoryDumpBank( selectedBank, pNew );
    }
}

template<typename T>
auto MemDebugger::loadMemoryBank(uint8_t bank, bool noColorChanges) -> void {
    auto& pageList = memory->pageList;
    auto* pNew = reinterpret_cast<T*>(memDump);
    auto* pOld = reinterpret_cast<T*>(memDumpOld);

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
                val = GUIKIT::String::convertToHex( _swapWord(*pNew) );
            else
                val = GUIKIT::String::convertToHex( *pNew );
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

auto MemDebugger::toAscii(const uint8_t* buf, int len, char* result, char pad) -> void {
    for (int i = 0; i < len; i++) {
        result[i] = isprint(int(buf[i])) ? char(buf[i]) : pad;
    }

    result[len] = 0;
}

auto MemDebugger::saveIdent() -> std::string {
    return "debugger_mem";
}

auto MemDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Memory";
}

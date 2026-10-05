
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include "dasmHandler.h"
#include "../../../tools/watcher.h"

// a reference can be passed as an alternative to virtual methods.
#define REF Agnus
#define REF_NS LIBAMI   // optional
#define REF_TYPE struct // or class
#define REF_INCLUDE "../../agnus/agnus.h"

#ifdef REF
    #ifdef REF_NS
        namespace REF_NS { REF_TYPE REF; }
    #else
        REF_TYPE REF;
        #define REF_NS
    #endif
#endif

namespace M68FAMILY {

class M68Core {
    typedef void (M68Core::*OpTable)(uint16_t);
    typedef void (M68Core::*OpDasm)(uint16_t, uint32_t&, DasmHandler&);

    OpTable opTable[0x10000] = {nullptr};
    OpDasm* dasmTable = nullptr;
    uint8_t* mulCycleLookup = nullptr;

    friend struct WatchPoints;
    friend struct ModifiedCodes;
    friend struct HistoryHandler;

public:
    enum class Engine { M68000, M68020 };

protected:
    const char** mnemonics = nullptr;
#ifdef REF
    M68Core(REF_NS::REF& ref) : ref(ref) { create(); }
    REF_NS::REF& ref;
#else
    M68Core() { create(); }
#endif

    ~M68Core();

    enum { Asl, Asr, Lsl, Lsr, Rol, Ror, Roxl, Roxr,
        Bchg, Bset, Bclr, Btst,
        Sub, Subx, Add, Addx, Not, Cmp, And, Or, Eor, Abcd, Sbcd,
        Adda, Suba, Cmpa,
        Mulu, Muls, Divu, Divs,
        Clr, Nbcd, Neg, Negx, Move, Movea,
        St, Sf, Shi, Sls, Scc, Scs, Sne, Seq,
        Svc, Svs, Spl, Smi, Sge, Slt, Sgt, Sle,
        Bra, Bhi, Bls, Bcc, Bcs, Bne, Beq, Bvc,
        Bvs, Bpl, Bmi, Bge, Blt, Bgt, Ble,
        Dbt, Dbf, Dbhi, Dbls, Dbcc, Dbcs, Dbne, Dbeq,
        Dbvc, Dbvs, Dbpl, Dbmi, Dbge, Dblt, Dbgt, Dble,
        Addi, Subi, Cmpi, Andi, Ori, Eori, Cmpm,
        Addq, Subq, Moveq, Movep,
        Bsr, Jmp, Jsr, Link, Unlk, Tas, Tst,
        Lea, Pea, Movem, Chk, Exg, Ext, Nop, Trap, Trapv,
        Reset, Rte, Rtr, Rts, _Stop, Swap,
        Illegal, LineA, LineF,
        Bfchg, Bfset, Bfclr, Bftst, Bfexts, Bfextu, Bfffo, Bfins,
        Chk2, Cmp2, Bkpt, Cas, Cas2, Extb, Pack, Unpk, Rtd,
        Trapt, Trapf, Traphi, Trapls, Trapcc, Trapcs, Trapne, Trapeq,
        Trapvc, Trapvs, Trappl, Trapmi, Trapge, Traplt, Trapgt, Traple,
        Movec, Moves, MoveCcr,
    };

    enum {  None = 0, SampleIPL = 1, SkipExtension = 2,
            PRG = 8, ConcurrentAdrCalc = 0x10 /* happens while prefetching */, Reverse = 0x20,
            SF_DFC = 0x40, SF_SFC = 0x80, FirstPrefetch = 0x100, ExtPrefetch = 0x200 };

    enum { SF_DATA = 1, SF_PRG = 2, SF_S = 4, SF_EXC = 8, SF_READ = 0x10 };

public:
    enum { Byte = 1, Word = 2, Long = 4, BWL = 7, WL = 6, BW = 3 };

    enum {
        DataRegisterDirect = 0,
        AddressRegisterDirect = 1,
        AddressRegisterIndirect = 2,
        AddressRegisterIndirectWithPostIncrement = 3,
        AddressRegisterIndirectWithPreDecrement = 4,
        AddressRegisterIndirectWithDisplacement = 5,
        AddressRegisterIndirectWithIndex = 6,
        AbsoluteShort = 7 + 0,
        AbsoluteLong = 7 + 1,
        ProgramCounterIndirectWithDisplacement = 7 + 2,
        ProgramCounterIndirectWithIndex = 7 + 3,
        Immediate = 7 + 4,
    };

    enum { Normal = 0, IRQ = 1, Trace = 2, Halt = 4, Stop = 8,
        TraceScheduled = 0x10, IRQScheduled = 0x20, ResetRoutine = 0x40,
        WatchPoint = 0x80, WatchPointWrite = 0x100, BreakPoint = 0x200, ExceptionPoint = 0x400,
        SoftStop = 0x800, ModifiedCode = 0x1000, History = 0x2000
    };

protected:
    Engine engine = Engine::M68000;
    uint32_t regsD[8];
    uint32_t regsA[8];
    uint32_t pc;
    uint32_t pcEdge;

    uint32_t usp;
    uint32_t ssp;
    uint32_t msp;

    uint16_t irc;
    uint16_t ird;

    bool c;
    bool v;
    bool z;
    bool n;
    bool x;

    uint8_t i;
    bool s;

    bool m;
    bool t0;
    uint32_t vbr;
    uint8_t group;

    uint8_t iplPins;
    uint8_t iplSample;
    int control;
    std::vector<uint32_t> stepOuts;

    uint32_t cacr;
    uint32_t caar;

    uint8_t dfc;
    uint8_t sfc;
    uint8_t fc;

    struct CacheEntry {
        uint32_t data;
        uint32_t tag;
        bool valid;
    };
    CacheEntry cacheEntries[64];

    uint32_t cahr; // cache holding register

    int busCycles;

    Emulator::WatchPoints watchPoints = Emulator::WatchPoints();
    Emulator::WatchPoints watchPointsWrite = Emulator::WatchPoints();
    Emulator::WatchPoints breakPoints = Emulator::WatchPoints();
    Emulator::WatchPoints exceptionPoints = Emulator::WatchPoints();
    Emulator::ModifiedCodes modifiedCode = Emulator::ModifiedCodes();
    Emulator::HistoryHandler<uint16_t> historyHandler = Emulator::HistoryHandler<uint16_t>();

    std::optional<uint32_t> softStep = std::nullopt;

    struct AddressError : std::exception {
        AddressError(uint32_t ea, uint32_t pc, unsigned flags) : ea( ea ), pc( pc ), flags( flags ) {}

        uint32_t ea;
        uint32_t pc;
        unsigned flags;
    };

    struct BusError : AddressError {
        BusError(uint32_t ea, uint32_t pc, unsigned flags) : AddressError( ea, pc, flags ) {}
    };

public:
    enum { USER_VECTOR = 0, AUTO_VECTOR = 1, UNINITIALIZED = 2,  SPURIOUS = -1};

    auto setEngine(Engine engine) -> void;
    auto getEngine() const -> Engine { return this->engine; }
    auto process() -> void;
    auto setInterrupt( uint8_t level ) -> void;
    auto reset() -> void;
    auto power() -> void;
    auto isHalted() -> bool { return !!(control & Halt); }
    auto setHalt() -> void; // I/O - could be halted from external device
    auto getCCR() -> uint8_t;
    auto getSR() -> uint16_t;

    /* external devices can raise BERR (BUS error) as a result of unwanted memory access.
     * prefetching often takes place at the same time as ALU operations,
     * which leaves status flags with intermediate values. (not emulated)
     * for Amiga 500/600, BERR is wired to expansions port. Amiga 1200 and Atari can cause BUS errors directly.
     */
    auto setBusError(uint32_t addr, bool read, bool prg) -> void;

    auto debuggerStepOver(bool subroutineOnly) -> void;
    auto debuggerStepInto() -> void;
    auto debuggerStepOut() -> bool;

    auto disassemble(uint32_t addr, unsigned& bytes, uint16_t* memSnap = nullptr) -> std::string;
    auto disassembleData(uint32_t addr, unsigned words) -> std::string;
    auto disassembleTrace(unsigned i, uint16_t& flags) -> std::string;
    auto hasModifiedCode() -> bool { return modifiedCode.getAndForget(); }
    auto appendStepOut(uint32_t addr) -> void;

    // use this to calculate the needed wait states by terminating a BUS cycle with VPA line
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnEClock(int eCyclePos) -> uint8_t;
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnMainClockCycles(int clockCycles) -> uint8_t;

    // function codes: optional, when periphery needs to distinguish between data (FC0) and program (FC1) access.
    auto getFc() -> uint8_t;
    
protected:
#ifndef REF
    // reset line is bi-directional
    virtual auto resetOut() -> void {}
    // sync to external devices in reasonable steps
    // internal cycles will not be prolonged with wait states, only BUS cycles
    virtual auto sync(unsigned cycles) -> void {}
    // methods to handle BUS cycles and wait states
    virtual auto readByte(uint32_t adr) -> uint8_t = 0;
    virtual auto readWord(uint32_t adr) -> uint16_t = 0;
    virtual auto readLong(uint32_t adr) -> uint32_t = 0;
    virtual auto peekWord(uint32_t adr) -> uint16_t = 0;
    virtual auto writeByte(uint32_t adr, uint8_t data) -> void = 0;
    virtual auto writeWord(uint32_t adr, uint16_t data) -> void = 0;
    virtual auto writeLong(uint32_t adr, uint32_t data) -> void = 0;

    // BUS cycle to fetch the Interrupt vector with following return types:
    // USER_VECTOR: e.g. Amiga, external device place vector on data BUS
    // AUTO_VECTOR: for slower 6800 peripheral devices (like CIA)
    // SPURIOUS: respond with BERR (BUS error)
    // UNINITIALIZED: default
    //
    // NOTE: vector reference is needed for USER_VECTOR only
    virtual auto iackCycle(uint8_t level, uint8_t& vector) -> int { return UNINITIALIZED; }

    virtual auto debugPointReached(int source, Emulator::WatchPoints& wp, unsigned addr) -> void {}

    // 68020 CPU may trigger additional BUS cycles to access a 32 bit value from 8 or 16 bit port
    virtual auto portSize(uint32_t adr) -> unsigned { return 4; }
#endif

    auto flagDebugAction(int action, bool state) -> void;
private:
    auto controlBreaks() -> void;
    auto setCCR(uint8_t data) -> void;
    auto setSR(uint16_t data) -> void;
    template<Engine E> auto build() -> void;
    auto create() -> void;
    auto sampleInterrupt() -> void;
    auto setSuperVisor(bool state = true) -> void;
    auto setMasterSP(bool state) -> void;
    auto syncSequencer(unsigned cycles) -> void;
    template<uint8_t Size = Word> auto misaligned(uint32_t adr) -> bool;

    template <Engine E, unsigned Flags, bool Scale = false> auto indirectIndexBrief(uint32_t B) -> uint32_t;
    template <Engine E, unsigned Flags> auto indirectIndexFull(uint32_t B) -> uint32_t;
    template <Engine E> auto fetchDisplacement(bool longDis) -> uint32_t;

    template<uint8_t Size = Long> inline auto readRegD(int reg) -> uint32_t;
    template<uint8_t Size = Long> inline auto writeRegD(int reg, uint32_t data) -> void;
    
    template<uint8_t Size = Long> inline auto readRegA(int reg) -> uint32_t;
    inline auto writeRegA(int reg, uint32_t data) -> void;
    template<uint8_t Size = Long> auto readRegR( int reg ) -> uint32_t;
    template<uint8_t Size = Long> auto writeRegR( int reg, uint32_t val ) -> void;

    template<Engine E, uint8_t Size, unsigned Flags = 0> auto read(uint32_t adr) -> uint32_t;
    template<Engine E, uint8_t Size, unsigned Flags = 0> auto write(uint32_t adr, uint32_t data) -> void;
    template<Engine E>  auto firstPrefetch() -> void;
    template<Engine E, unsigned Flags = None> inline auto prefetch() -> void;
    template<unsigned Flags> auto cachedPrefetch(uint32_t adr) -> uint16_t;
    template<Engine E, unsigned Flags = None> auto fullPrefetch() -> void;
    template<Engine E, unsigned Flags = None> inline auto readExtensionWord() -> void;

    auto peek(uint32_t adr) -> uint16_t;
    template<uint8_t Size> auto peekInc(uint32_t& adr, DasmHandler& d) -> uint32_t;
    
    template<Engine E, uint8_t Mode, uint8_t Size, unsigned Flags = None> auto calcEA(int reg) -> uint32_t;
    template<Engine E, uint8_t Mode, uint8_t Size, unsigned Flags = None> auto readEA(int reg, uint32_t& ea) -> uint32_t;
    template<Engine E, uint8_t Mode, uint8_t Size, unsigned Flags = None> auto writeEA(uint32_t ea, uint32_t data) -> void;
    template<uint8_t Mode, uint8_t Size> auto prepareEaForDasm(uint32_t& adr, uint8_t reg, DasmHandler& d) -> void;

    template<unsigned Flags> auto readWordDS(uint32_t adr) -> uint16_t;
    template<unsigned Flags> auto readLongDS(uint32_t adr) -> uint32_t;
    template<unsigned Flags> auto writeWordDS(uint32_t adr, uint16_t data) -> void;
    template<unsigned Flags> auto writeLongDS(uint32_t adr, uint32_t data) -> void;

    auto parse(const char* s, uint16_t sum = 0) -> uint16_t;
    template<uint8_t Size> inline auto clip(uint32_t data) -> uint32_t;
    template<uint8_t Size> inline constexpr auto msb() -> uint32_t;
    template<uint8_t Size> inline auto sign(uint32_t data) -> int32_t;
    template<uint8_t Size> inline auto negative(uint32_t data) -> bool;
    template<uint8_t Size> inline auto carry(uint64_t data) -> bool;
    template<uint8_t Size> inline constexpr auto mask() -> uint32_t;
    template<uint8_t Size> inline constexpr auto inverseMask() -> uint32_t;
    template<uint8_t Size> inline constexpr auto bits() -> uint8_t;

    template<uint8_t Size> auto zero(uint32_t result) -> bool {
        return clip<Size>(result) == 0;
    }

    #define OP_DECLARE(ident) \
        template<Engine E, uint8_t Inst, uint8_t Size> auto op##ident(uint16_t) -> void; \
        template<uint8_t Inst, uint8_t Size> auto dasm##ident(uint16_t, uint32_t&, DasmHandler&) -> void;

    #define OP_DECLARE_EA(ident) \
        template<Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto op##ident(uint16_t) -> void; \
        template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto dasm##ident(uint16_t, uint32_t&, DasmHandler&) -> void;

    OP_DECLARE(ImmShift)
    OP_DECLARE(RegShift)
    OP_DECLARE_EA(Shift)
    OP_DECLARE_EA(Bit)
    OP_DECLARE_EA(ImmBit)
    OP_DECLARE_EA(BitField)
    OP_DECLARE_EA(BitFieldEA)
    OP_DECLARE_EA(Clr)
    OP_DECLARE_EA(Nbcd)
    OP_DECLARE_EA(Neg)
    OP_DECLARE_EA(Scc)
    OP_DECLARE_EA(Tas)
    OP_DECLARE_EA(Tst)
    OP_DECLARE_EA(Arithmetic)
    OP_DECLARE_EA(Cmp)
    OP_DECLARE_EA(ArithmeticEA)
    OP_DECLARE_EA(ArithmeticA)
    OP_DECLARE_EA(Cmpa)
    OP_DECLARE_EA(Mul)
    OP_DECLARE_EA(Mull)
    OP_DECLARE_EA(Div)
    OP_DECLARE_EA(Divl)

    OP_DECLARE_EA(Move)
    OP_DECLARE_EA(MoveDataRegisterDirect)
    OP_DECLARE_EA(MoveAddressRegisterIndirect)
    OP_DECLARE_EA(MoveAddressRegisterIndirectWithPostIncrement)
    OP_DECLARE_EA(MoveAddressRegisterIndirectWithPreDecrement)
    OP_DECLARE_EA(MoveAddressRegisterIndirectWithDisplacement)
    OP_DECLARE_EA(MoveAddressRegisterIndirectWithIndex)
    OP_DECLARE_EA(MoveAbsoluteShort)
    OP_DECLARE_EA(MoveAbsoluteLong)

    OP_DECLARE_EA(MoveA)
    OP_DECLARE_EA(ArithmeticI)
    OP_DECLARE_EA(ArithmeticQ)
    OP_DECLARE(MoveQ)
    OP_DECLARE(Bcc)
    OP_DECLARE(Bsr)
    OP_DECLARE(Dbcc)
    OP_DECLARE_EA(Jmp)
    OP_DECLARE_EA(Jsr)
    OP_DECLARE_EA(Lea)
    OP_DECLARE_EA(Pea)
    OP_DECLARE_EA(MovemToReg)
    OP_DECLARE_EA(MovemToEa)
    OP_DECLARE(ArithmeticX)
    OP_DECLARE(ArithmeticXEa)
    OP_DECLARE(ArithmeticBCD)
    OP_DECLARE(Cmpm)
    OP_DECLARE(Ccr)
    OP_DECLARE(Sr)
    OP_DECLARE_EA(Chk)
    OP_DECLARE_EA(Chk2)
    OP_DECLARE_EA(MoveFromSr)
    OP_DECLARE_EA(MoveToSr)
    OP_DECLARE_EA(MoveToCcr)
    OP_DECLARE_EA(MoveCcr)
    OP_DECLARE(ExgDxDy)
    OP_DECLARE(ExgAxAy)
    OP_DECLARE(ExgAxDy)
    OP_DECLARE(Ext)
    OP_DECLARE(Extb)
    OP_DECLARE(Link)
    OP_DECLARE(MoveUspAn)
    OP_DECLARE(MoveAnUsp)
    OP_DECLARE(Swap)
    OP_DECLARE(Trap)
    OP_DECLARE(Trapcc)
    OP_DECLARE(Unlink)
    OP_DECLARE(MovepReg)
    OP_DECLARE(MovepEa)

    OP_DECLARE(Bkpt)
    OP_DECLARE_EA(Cas)
    OP_DECLARE(Cas2)

    OP_DECLARE(PackDn)
    OP_DECLARE(PackPd)
    OP_DECLARE(UnpackDn)
    OP_DECLARE(UnpackPd)

    OP_DECLARE(MovecRcRx)
    OP_DECLARE(MovecRxRc)
    OP_DECLARE_EA(Moves)

    #undef OP_DECLARE_EA
    #undef OP_DECLARE

    template<Engine E> auto opTrapv(uint16_t opcode) -> void;
    template<Engine E> auto opNop(uint16_t opcode) -> void;
    template<Engine E> auto opReset(uint16_t opcode) -> void;
    template<Engine E> auto opRtd(uint16_t opcode) -> void;
    template<Engine E> auto opRte(uint16_t opcode) -> void;
    template<Engine E> auto opRtr(uint16_t opcode) -> void;
    template<Engine E> auto opRts(uint16_t opcode) -> void;
    template<Engine E> auto opStop(uint16_t opcode) -> void;

    auto dasmTrapv(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmNop(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmReset(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRtd(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRte(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRtr(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRts(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmStop(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmIllegal(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmLineA(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmLineF(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;

    template<uint8_t Inst> auto bit(uint32_t data, int bit) -> uint32_t;
    template<uint8_t Inst> auto bitField(uint32_t data, uint32_t offset, uint32_t mask) -> uint32_t;
    template<uint8_t Inst> auto bitFieldExtract(uint32_t data, uint32_t width) -> uint32_t;
    template<bool withRotation = true> auto bitFieldInsert(uint32_t result, uint32_t insert, uint32_t offset, uint32_t width) -> uint32_t;
    auto bitFieldFind(uint32_t data, uint32_t offset, uint32_t width) -> uint32_t;
    template<uint8_t Inst, uint8_t Size, bool SingleShift = false> auto shifter(uint32_t data, int shift) -> uint32_t;
    
    template<uint8_t Inst> auto singleShifter(uint32_t data) -> uint32_t {
        return shifter<Inst, Word, true>( data, 1 );
    }

    template<uint8_t Inst, uint8_t Size> auto arithmetic(uint32_t src, uint32_t dest) -> uint32_t;
    template<typename T, typename TSign, uint8_t Inst, uint8_t Size> auto arithmeticT(uint32_t src, uint32_t dest) -> uint32_t;
    template<uint8_t Inst> auto bcd(uint32_t src, uint32_t dest) -> uint8_t;
    template<uint8_t Inst> auto testCondition() -> bool;

    template<bool quadWord> auto mullu(uint32_t src, uint32_t dest) -> uint64_t;
    template<bool quadWord> auto mulls(uint32_t src, uint32_t dest) -> uint64_t;
    
    template<uint8_t Size> auto applyRoxRange(int& shift) -> void;
    template<uint8_t Size> auto defaultFlags(uint32_t data) -> void;
    template<uint8_t Mode, uint8_t destMode, uint8_t Size> auto setMoveCCWhenAddressError(uint32_t data) -> void;

    template<Engine E> auto executeAt(uint32_t adr) -> void;
    template<Engine E, uint8_t Mode, uint8_t Size> auto addressExceptionEA(uint32_t ea) -> void;
    template<Engine E, uint8_t Mode, uint8_t Size> auto addressExceptionMoveEA(uint32_t ea) -> void;
    auto group0Exception(const std::exception& exc) -> void;
    template<Engine E> auto group0Exception(const std::exception& exc) -> void;
    template<Engine E> auto group0Exception(uint32_t vector, uint32_t adr, uint32_t _pc, unsigned Flags) -> void;

    template<Engine E> auto resetRoutine() -> void;
    auto resetRoutine() -> void;
    template<Engine E> auto group1Exception(uint8_t vector) -> void;
    template<Engine E> auto IRQException() -> void;
    auto IRQException() -> void;
    auto traceException() -> void;
    template<Engine E> auto trapException(uint8_t vector) -> void;
    auto privilegeException() -> void;
    template<Engine E> auto privilegeException() -> void;
    template<Engine E> auto illegal(uint16_t opcode) -> void;
    template<Engine E> auto lineF(uint16_t opcode) -> void;
    template<Engine E> auto lineA(uint16_t opcode) -> void;
    auto getInterruptVector(uint8_t level) -> uint8_t;

    template<uint8_t Inst> auto ccr(uint8_t data) -> void;
    template<uint8_t Inst> auto sr(uint16_t data) -> void;
    
    template<uint8_t Inst> auto cyclesBit(int bit) -> void;
    template<uint8_t Inst> auto cyclesMul(uint16_t data) -> void;
    template<uint8_t Inst> auto cyclesDiv(uint32_t dividend, uint16_t divisor) -> int;

    auto setChk2UndefinedFlags(int32_t lower, int32_t upper, int32_t value) -> void;
    auto setDivslUndefinedFlags(uint8_t size, int64_t dividend, int32_t divisor) -> void;
    template<uint8_t Size> auto setChkUndefinedFlags(int32_t src, int32_t dst) -> void;

    template<uint8_t Mode> constexpr static auto isRegisterMode() -> bool { return Mode == AddressRegisterDirect || Mode == DataRegisterDirect; }
    template<uint8_t Mode> constexpr static auto isDirectMode() -> bool { return isRegisterMode<Mode>() || Mode == Immediate; }
    template<uint8_t Mode> constexpr static auto isAbsMode() -> bool { return Mode == AbsoluteShort || Mode == AbsoluteLong; }
    template<uint8_t Mode> constexpr static auto isIndexMode() -> bool { return Mode == AddressRegisterIndirectWithIndex || Mode == ProgramCounterIndirectWithIndex; }
    template<uint8_t Mode> constexpr static auto isDisplacementMode() -> bool { return Mode == AddressRegisterIndirectWithDisplacement || Mode == ProgramCounterIndirectWithDisplacement; }
    template<uint8_t Mode> constexpr static auto isPcMode() -> bool { return Mode == ProgramCounterIndirectWithIndex || Mode == ProgramCounterIndirectWithDisplacement; }

    template<Engine E> auto nextIsGroup1Exception() -> bool;

    auto checkSoftStop(uint32_t addr) -> bool;
    auto loadTrace(Emulator::HistoryEntry<uint16_t>& entry) -> void;

    auto invalidateCache() -> void;
    auto setCacr(uint32_t value) -> void;

    template<bool withRotation = true> auto bfMask(unsigned width, unsigned offset) -> uint32_t;
    template<bool withRotation = true> auto bfShiftLeft(uint32_t data, unsigned offset) -> uint32_t;
    template<bool withRotation = true> auto bfShiftRight(uint32_t data, unsigned offset) -> uint32_t;
};

}

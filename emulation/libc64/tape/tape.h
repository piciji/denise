
#pragma once

#include "../../tools/event.h"
#include "../../tools/serializer.h"
#include "../interface.h"

#define TAPE_MOTOR_DELAY 32000
#define TAPE_ZERO_GAP 20000
#define TAPE_MAX_EVENT_DELAY 20000

#define TAPE_FETCH_SIZE 50 * 1024
#define TAPE_WRITE_SIZE 10 * 1024

#include <cstdlib>

namespace LIBC64 {	
    
struct Tape {
    
    Tape( Emulator::Events* events );
    ~Tape();

	enum Mode : uint8_t { Stop = 0, Play = 1, Record = 2, Forward = 3, Rewind = 4, ResetCounter = 5 };
	enum CounterState : uint8_t { NoOperation = 0, Read = 1, Write = 2, List = 3 };
	
	std::function<void ()> setReadTransition = [](){};
	std::function<void ( unsigned, unsigned )> updateState = [](unsigned mode, unsigned counter){};
    std::function<unsigned (uint8_t*, unsigned, unsigned)> read = [](uint8_t* buffer, unsigned length, unsigned offset){ return 0; };
	std::function<unsigned (uint8_t*, unsigned, unsigned)> write = [](uint8_t* buffer, unsigned length, unsigned offset){ return 0; };
	std::function<void (bool)> senseOut = [](bool state){};	

	auto setEnabled( bool state ) -> void;
	auto isEnabled() -> bool { return enabled; }
	auto writeIn(bool bit) -> void;
	auto clock() -> void;
	auto setMotorIn( bool state ) -> void;
	auto load(Emulator::Interface::Media* media, uint8_t* data, unsigned size) -> void;
    auto unload() -> void;
	auto reset() -> void;
	auto setWriteProtect(bool state) -> void;
    auto isWriteProtected() -> bool;
	auto setCyclesPerSecond( unsigned value ) -> void;	
	auto setMode( unsigned mode ) -> void;
    auto getMode( ) -> Mode;
	auto createTap( unsigned& imageSize ) -> uint8_t*;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeLight(Emulator::Serializer& s) -> void;
    auto selectListing( unsigned pos ) -> void;
    auto getMedia() -> Emulator::Interface::Media* { return media; }
	
protected:	
	Emulator::Interface::Media* media;
    Emulator::Events* events;
    std::function<void ()> worker;
    std::function<void ()> motorOff;
	std::function<void ()> delayMode;
    
    uint8_t* data = nullptr;
    unsigned size;           
    
	uint8_t* fetchData;
	uint8_t* writeData;
	
	bool enabled;
	
	Mode mode;
	Mode nextMode;
	unsigned writePos;
	bool writeProtect = false;
    uint8_t writeQuestionState = 0;
	bool writeBit;
    unsigned cyclesElapsed;
    uint64_t cycles;
    uint64_t cycles999;
	unsigned cylcesPerSecond;
	uint64_t cyclesTotal;
    unsigned gapsRemaining;
	unsigned counter;
	unsigned counterOffset;
	
	bool motorIn;
	bool loaded = false;
    bool directionForward;
	bool lastDirectionForward;
    uint8_t version;
    int adjust; // random value
    unsigned fetchPos; // position in fetched chunk
    unsigned fetchSize; // size of fetched chunk
    unsigned pos; // overall position in tap file
    
    struct {
        CounterState cstate;
        unsigned counter;        
    } currentCounter;
    
    auto readHeader() -> bool;			
	
    // counter
    auto calculateCounter() -> unsigned;    
    auto updateCounter() -> void;
    auto resetCounter() -> void;
    auto speedAdjustment() -> double;
    auto calculateCounterForNoTape() -> unsigned;
    
	// write
	auto addByteToWriteBuffer(uint8_t byte) -> void;
	auto writeBuffer() -> void;	
	
    // fetch
    auto nextGap() -> unsigned;
    auto fetchGap( bool& _longGap ) -> unsigned;
    auto shortGap( uint8_t byte ) -> unsigned;
    auto longGap( ) -> unsigned;
    auto randomizeGap( unsigned gap ) -> unsigned;
    auto readBackward( uint8_t& byte, unsigned count ) -> bool;
    auto readBackward( uint8_t& byte ) -> bool;
    auto readForward( uint8_t& byte, unsigned count ) -> bool;
    auto readForward( uint8_t& byte ) -> bool;
};    
    
extern Tape* tape;
}

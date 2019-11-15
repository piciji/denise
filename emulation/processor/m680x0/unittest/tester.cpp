
#include "results.h"
#include "tester.h"
#include <iostream>

namespace M68FAMILY {	

#define _parse(s) (uint16_t)Base::parse(s)
	
#include "cases/nbcd.cpp"
#include "cases/sbcd.cpp"

	
	
M68000Tester::M68000Tester() : M68000() {	
	sampleNbcd();
	sampleSbcd();
}	

auto M68000Tester::run() -> void {	
	testNbcd();
	testSbcd();
}

auto M68000Tester::errorCount() -> void {	
	std::cout << std::endl << "Errors: " + std::to_string( errorCounter ) << std::endl;
	
	std::system("PAUSE");	
}

auto M68000Tester::setContext( M68Context* context ) -> void {	
	context->read = [&](uint32_t adr) {
		return memory->read(adr);
	};
	
	context->readWord = [&](uint32_t address) {
		return memory->readWord(address);
	};
	
	context->write = [&](uint32_t address, uint8_t value) {
		memory->write( address, value );
	};

	context->writeWord = [&](uint32_t address, uint16_t value) {
		memory->writeWord( address, value );
	};

	context->sampleIrq = [&]() {
		calced->irqChecked = true;
	};
	
	context->sync = [&](unsigned cycles) {
		calced->cycles += cycles;
	};
	
	Base::setContext( context );
}

auto M68000Tester::group0exception(uint32_t addr, uint8_t type) -> void {
	if (type == BusError) calced->busError = true;
	else calced->addressError = true;
	
	M68000::group0exception(addr, type);
}

auto M68000Tester::illegalException(uint8_t vector) -> void {
	if (vector == 8) calced->privilegeException = true;
	else calced->illegalException = true;
	
	M68000::illegalException( vector );
}

auto M68000Tester::trapException(uint8_t vector) -> void {
	calced->group2Exception = true;
	
	M68000::trapException(vector);
}

auto M68000Tester::setUp() -> void {
	if (calced) delete calced;
	if (memory) delete memory;
	calced = new Results("", false);
	memory = new Memory;	
	calced->setMemory( memory );	
	calced->setContext( ctx );
	calced->setCycleCount( -40 );	
    adrCounter = 8;
    memory->write(7, adrCounter); //init pc
}

auto M68000Tester::addWord(uint16_t word) -> void {
	memory->writeWord( adrCounter, word );	
	adrCounter += 2;
}

auto M68000Tester::check(std::string ident) -> void {
	std::string error = "";
	Results* sampled = Results::get( ident );
	static unsigned testCounter = 0;
	
	assert( sampled ); //no sample prepared for ident		
	
	if ( !Results::compare( sampled, calced, error ) ) {
		std::cout << ident + " -> error: " + error << std::endl;
		errorCounter++;
	} else {
		std::cout << ident + " -> success " << std::endl;
	}
	
	if (testCounter++ > 100) {
        testCounter = 0;
        std::cout << std::endl;
        std::system("PAUSE");
    }	
}

auto M68000Tester::getEA(uint8_t mode, uint8_t reg) -> unsigned {
	if (mode < 7)
		return mode << 3 | reg;
	
	return 7 << 3 | (mode - 7);	
}
	
}
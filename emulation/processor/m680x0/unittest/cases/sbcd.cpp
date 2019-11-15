
//#include "../tester.h"
//#include "../results.h"

void M68000Tester::sampleSbcd() {
    Results* oObj;
    Memory* oSampleMem;

    oObj = new Results("SBCD D2, D1");
    oObj->setRegD(1, 0x12345611);
    oObj->setRegD(2, 0xf745ff78);
    oObj->setCycleCount(6);

    oObj = new Results("SBCD D2, D1 zero");
    oObj->setRegD(1, 0x12345600);
    oObj->setRegD(2, 0xf745ffff);
    oObj->setZ();
    oObj->setCycleCount(6);

    oObj = new Results("SBCD D2, D1 neg");
    oObj->setRegD(1, 0x12345688);
    oObj->setRegD(2, 0xf745ff45);
    oObj->setX()->setN()->setC();
    oObj->setCycleCount(6);

    oObj = new Results("SBCD D2, D1 overflow");
    oObj->setRegD(1, 0x12345643);
    oObj->setRegD(2, 0xf745ffff);
    oObj->setX()->setV()->setC();
    oObj->setCycleCount(6);

    oObj = new Results("SBCD -(A2), -(A1)");
    oObj->setN();
    oObj->setRegA(1, 0x3000);
    oObj->setRegA(2, 0x4000);
    oSampleMem = new Memory;
    oSampleMem->write(0x3000, 0x82);
    oSampleMem->write(0x4000, 0x19);
    oObj->setMemory(oSampleMem);
    oObj->setCycleCount(18);
	
	oObj = new Results("SBCD -(A1), -(A1)");
    oObj->setRegA(1, 0x2fff);
    oSampleMem = new Memory;
    oSampleMem->write(0x3000, 0xff);
    oSampleMem->write(0x2fff, 0);
    oObj->setMemory(oSampleMem);
    oObj->setCycleCount(18);
	
	oObj = new Results("SBCD D2, D1 a1");
    oObj->setRegD(1, 0x12345621);
    oObj->setRegD(2, 0xf745ff23);
    oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a2");
	oObj->setX()->setV()->setC();
    oObj->setRegD(1, 0x12345656);
    oObj->setRegD(2, 0xf745ffaa);
    oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a3");
    oObj->setRegD(1, 0x12345644);
    oObj->setRegD(2, 0xf745ff66);
    oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a4");
	oObj->setN();
    oObj->setRegD(1, 0x123456ff);
    oObj->setRegD(2, 0xf745ff00);
    oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a5");
	oObj->setN()->setX()->setC();
	oObj->setRegD(1, 0x1234569b);
	oObj->setRegD(2, 0xf745ffff);
	oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a6");
	oObj->setC()->setX()->setV();
	oObj->setRegD(1, 0x12345652);
	oObj->setRegD(2, 0xf745ffb2);
	oObj->setCycleCount(6);
	
	oObj = new Results("SBCD D2, D1 a7");
	oObj->setRegD(1, 0x12345601);
	oObj->setRegD(2, 0xf745ff99);
	oObj->setCycleCount(6);
	
	oObj = new Results("SBCD D2, D1 a8");
	oObj->setN();
	oObj->setRegD(1, 0x12345687);
	oObj->setRegD(2, 0xf745ff12);
	oObj->setCycleCount(6);

	oObj = new Results("SBCD D2, D1 a9");
	oObj->setN()->setX()->setC();
	oObj->setRegD(1, 0x12345696);
	oObj->setRegD(2, 0xf745ff0a);
	oObj->setCycleCount(6);
}

void M68000Tester::testSbcd() {
    uint16_t opcode;

    //MOVE.L    #$12345689, D1
    //MOVE.L    #$f745ff78, D2
    //MOVE #$4, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345689);
    setRegD(2, 0xf745ff78);
    process();
    check("SBCD D2, D1");

    //MOVE.L    #$12345605, D1
    //MOVE.L    #$f745ff05, D2
    //MOVE #$4, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x123456ff);
    setRegD(2, 0xf745ffff);
    process();
    check("SBCD D2, D1 zero");

    //MOVE.L    #$12345634, D1
    //MOVE.L    #$f745ff45, D2
    //MOVE #$10, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x12345634);
    setRegD(2, 0xf745ff45);
    process();
    check("SBCD D2, D1 neg");

    //MOVE.L    #$12345634, D1
    //MOVE.L    #$f745ff45, D2
    //MOVE #$10, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x123456a9);
    setRegD(2, 0xf745ffff);
    process();
    check("SBCD D2, D1 overflow");

    //MOVE.L    #$3001, A1
    //MOVE.L    #$4001, A2
    //MOVE.B    #$a2, $3000
    //MOVE.B    #$19, $4000
    //MOVE #$10, CCR
    //SBCD -(A2), -(A1)
    //MOVE.B    $3000, D1
    //MOVE.B    $4000, D2
    setUp();
    opcode = _parse("1000 ---1 0000 1---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    memory->write(0x3000, 0xa2);
    memory->write(0x4000, 0x19);
    setCCR(0x10);
    setRegA(1, 0x3001);
    setRegA(2, 0x4001);
    process();
    check("SBCD -(A2), -(A1)");
	
	//MOVE.L    #$3001, A1
    //MOVE.B    #$ff, $3000
    //MOVE.B    #$ff, $2fff
    //MOVE #$0, CCR
    //SBCD -(A1), -(A1)
    //MOVE.B    $3000, D1
    //MOVE.B    $2fff, D2
    setUp();
    opcode = _parse("1000 ---1 0000 1---") | (1 << 9) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    memory->write(0x3000, 0xff);
    memory->write(0x2fff, 0xff);
    setCCR(0);
    setRegA(1, 0x3001);
    process();
    check("SBCD -(A1), -(A1)");

	//MOVE.L    #$12345644, D1
    //MOVE.L    #$f745ff23, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x12345644);
    setRegD(2, 0xf745ff23);
    process();
    check("SBCD D2, D1 a1");

	//MOVE.L    #$12345666, D1
    //MOVE.L    #$f745ffaa, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x12345666);
    setRegD(2, 0xf745ffaa);
    process();
    check("SBCD D2, D1 a2");

	//MOVE.L    #$123456aa, D1
    //MOVE.L    #$f745ff66, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x123456aa);
    setRegD(2, 0xf745ff66);
    process();
    check("SBCD D2, D1 a3");

	//MOVE.L    #$123456ff, D1
    //MOVE.L    #$f745ff00, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x123456ff);
    setRegD(2, 0xf745ff00);
    process();
    check("SBCD D2, D1 a4");
	
	//MOVE.L    #$12345600, D1
    //MOVE.L    #$f745ffff, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x12345600);
    setRegD(2, 0xf745ffff);
    process();
    check("SBCD D2, D1 a5");
	
	//MOVE.L    #$12345664, D1
    //MOVE.L    #$f745ffb2, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x12345664);
    setRegD(2, 0xf745ffb2);
    process();
    check("SBCD D2, D1 a6");
	
	//MOVE.L    #$1234569a, D1
    //MOVE.L    #$f745ff99, D2
    //MOVE #$0, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x1234569a);
    setRegD(2, 0xf745ff99);
    process();
    check("SBCD D2, D1 a7");
	
	//MOVE.L    #$1234569a, D1
    //MOVE.L    #$f745ff12, D2
    //MOVE #$14, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x1234569a);
    setRegD(2, 0xf745ff12);
    process();
    check("SBCD D2, D1 a8");
	
	//MOVE.L    #$12345607, D1
    //MOVE.L    #$f745ff0a, D2
    //MOVE #$14, CCR
    //SBCD D2, D1
    setUp();
    opcode = _parse("1000 ---1 0000 0---") | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x12345607);
    setRegD(2, 0xf745ff0a);
    process();
    check("SBCD D2, D1 a9");

}

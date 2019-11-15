
auto M68000Tester::sampleNbcd() -> void {
    Results* oObj;
    Memory* oSampleMem;

    oObj = new Results("NBCD D1");
    oObj->setX()->setC()->setV();
    oObj->setRegD(1, 0x20);
    oObj->setCycleCount(6);

    oObj = new Results("NBCD D1 with x");
    oObj->setX()->setC()->setV();
    oObj->setRegD(1, 0x1234561f);
    oObj->setCycleCount(6);

    oObj = new Results("NBCD D1 zero");
    oObj->setZ();
    oObj->setRegD(1, 0x12345600);
    oObj->setCycleCount(6);

    oObj = new Results("NBCD D1 neg");
    oObj->setX()->setN()->setC();
    oObj->setRegD(1, 0x1234569a);
    oObj->setCycleCount(6);

    oObj = new Results("NBCD ($3000).W");
    oObj->setX()->setN()->setC();
    oSampleMem = new Memory();
    oSampleMem->write(0x3000, 0x99);
    oObj->setMemory(oSampleMem);
    oObj->setCycleCount(16);
	
	oObj = new Results("NBCD D1 a1");
    oObj->setRegD(1, 0x0);
    oObj->setCycleCount(6);
	
	oObj = new Results("NBCD D1 a2");
    oObj->setRegD(1, 0x0);
	oObj->setZ();
    oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a3");
    oObj->setRegD(1, 0x99);
	oObj->setX()->setN()->setC();
    oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a4");
    oObj->setRegD(1, 0x36);
	oObj->setX()->setV()->setC();
    oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a5");
    oObj->setRegD(1, 0x67);
	oObj->setX()->setV()->setC();
    oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a6");
	oObj->setRegD(1, 0x02);
	oObj->setX()->setC();
	oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a7");
	oObj->setRegD(1, 0);
	oObj->setX()->setC();
	oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a8");
	oObj->setRegD(1, 0xff);
	oObj->setX()->setC()->setN();
	oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a9");
	oObj->setRegD(1, 0x6b);
	oObj->setX()->setC()->setV();
	oObj->setCycleCount(6);

	oObj = new Results("NBCD D1 a10");
	oObj->setRegD(1, 0xb7);
	oObj->setX()->setC()->setN();
	oObj->setCycleCount(6);

}

auto M68000Tester::testNbcd() -> void {
    uint16_t opcode;

    //MOVE.B    #$7a, D1
    //MOVE #$0, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(1, 0x7a);
    process();
    check("NBCD D1");

    //MOVE.L    #$1234567a, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x1234567a);
    process();
    check("NBCD D1 with x");

    //MOVE.L    #$12345600, D1
    //MOVE #$4, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345600);
    process();
    check("NBCD D1 zero");

    //MOVE.L    #$123456ff, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x123456ff);
    process();
    check("NBCD D1 neg");

    //MOVE.B    #$01, $3000
    //MOVE #$0, CCR
    //NBCD ($3000).W
    //MOVE.B $3000, D2
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(AbsoluteShort);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0x0);
    memory->write(0x3000, 0x01);
    process();
    check("NBCD ($3000).W");
	
	//MOVE.B    #$0, D1
    //MOVE #$0, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(1, 0);
    process();
    check("NBCD D1 a1");
	
	//MOVE.B    #$0, D1
    //MOVE #$4, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0);
    process();
    check("NBCD D1 a2");
	
	//MOVE.B    #$0, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0);
    process();
    check("NBCD D1 a3");

	//MOVE.B    #$33, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x33);
    process();
    check("NBCD D1 a4");

	//MOVE.B    #$33, D1
    //MOVE #$4, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x33);
    process();
    check("NBCD D1 a5");

	//MOVE.B    #$98, D1
    //MOVE #$0, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x98);
    process();
    check("NBCD D1 a6");

	//MOVE.B    #$9a, D1
	//MOVE #$0, CCR
	//NBCD D1
	setUp();
	opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
	addWord(opcode);
	addWord(0xffff);
	power();
	setCCR(0);
	setRegD(1, 0x9a);
	process();
	check("NBCD D1 a7");

	//MOVE.B    #$9a, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x9a);
    process();
    check("NBCD D1 a8");

	//MOVE.B    #$2e, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x2e);
    process();
    check("NBCD D1 a9");

	//MOVE.B    #$e2, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = _parse("0100 1000 00-- ----") | getEA(DataRegisterDirect, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0xe2);
    process();
    check("NBCD D1 a10");

}

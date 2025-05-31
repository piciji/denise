
namespace LIBAMI {

auto Agnus::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer((uint8_t&)model);
    s.array(eventClock);
    s.integer( clock );
    s.integer( nextClock );
    s.integer( dmaClock );

    s.integer(rJob1.job);
    s.integer(rJob1.data);
    s.integer(rJob1.clock);
    s.integer(rJob2.job);
    s.integer(rJob2.data);
    s.integer(rJob2.clock);
    s.integer(rJob3.job);
    s.integer(rJob3.data);
    s.integer(rJob3.clock);

    s.integer( hTotalFirst );

    s.integer(actions);
    s.integer(busUsage);
    s.integer(hPos);
    s.integer(vPos);
    s.integer(vStart);
    s.integer(vStop);
    s.integer(dmal);

    s.integer(vPosLocked);
    s.integer(hPosLocked);

    s.integer(frameClock);
    s.integer(fpsChange);
    s.integer(hBlank);
    s.integer(vBlankEnd);
    s.integer(vBlankEndNext);
    s.integer(vBlank);
    s.integer(vBlankStart);
    s.integer(sprInhibited);
    s.integer(blitterConflict);
    s.integer(lines);
    s.integer(vTotal);
    s.integer(vBStrt);
    s.integer(vBStop);
    s.integer(hTotal);
    s.integer(beamCon);
    s.integer(hsStrt);
    s.integer(hsStop);

    for(int i = 0; i < 8; i++) {
        Sprite& spr = sprites[i];
        s.integer(spr.ptr);
        s.integer(spr.pos);
        s.integer(spr.ctl);
        s.integer(spr.vStart);
        s.integer(spr.vStop);
        s.integer(spr.fetchData);
        s.integer(spr.enable);
    }

    for(int i = 0; i < 4; i++) {
        AudioDmaChannel& cha = audioDmaChannels[i];
        s.integer(cha.ptr);
        s.integer(cha.ptrLatch);
    }

    s.integer(dskpt);
    s.integer(ddfStart);
    s.integer(ddfStop);
    s.integer(bpl1pt);
    s.integer(bpl2pt);
    s.integer(bpl3pt);
    s.integer(bpl4pt);
    s.integer(bpl5pt);
    s.integer(bpl6pt);
    s.integer(bpl1Mod);
    s.integer(bpl2Mod);

    s.integer(useRTC);
    s.integer(dataBus);
    s.integer(dmaCon);
    s.integer(dmaConImm);
    s.integer(dmaConCop);
    s.integer(dmaConBlt);
    s.integer(dmaConSpr);
    s.integer(bplCon0);
    s.integer(countWaitCycles);
    s.integer(rDmaPtr);
    s.integer(eClockCycle);
    s.integer(lol);
    s.integer(lof);
    s.integer(lolToggle);
    s.integer(ntsc);
    s.integer(laceMode);
    s.integer(laceFrame);

    s.integer(lineVCounter);
    s.integer(crop.left);
    s.integer(crop.right);
    s.integer(crop.top);
    s.integer(crop.bottom);

    s.integer(initVCounter);
    s.integer(shortLineBefore);
    s.integer(stopFetching);
    s.integer(bplCycle);
    s.integer(bplQueue);
    s.integer(sprQueue);
    s.integer(ddfStartMatch);
    s.integer(harddisH);
    s.integer(harddisV);
    s.integer(ddfEnableBefore);
    s.integer(ddfEnableCache);
    s.integer(bplState);
    s.integer(hardStop);
    s.integer(diwFlipFlop);
    s.integer(womLock);
    s.integer(resetFromKeyboard);
    s.integer(overclock.cycles);
    s.integer(hardDrivesBusy);

    if (!light) {
        auto fpsOld = fps;
        s.floatingpoint(fps);

        s.array(mapper);

        if (s.mode() == Emulator::Serializer::Mode::Load) {
            for (auto& expansionConf : expansionsConfigured)
                expansionConf = nullptr;

            unsigned _chipMemMask;
            unsigned _slowMemSize;
            unsigned _fastMemSize;
            s.integer(_chipMemMask);
            s.integer(_slowMemSize);
            s.integer(_fastMemSize);

            setChipmem(_chipMemMask + 1);
            setSlowmem(_slowMemSize);
            setFastmem(_fastMemSize);

            checkForRomEncryption();
            mapRom(false);

            if (std::fabs(fpsOld - fps) > 0.03) {
                //interface->log(fpsOld);
                //interface->log(fps, 0);
                interface->fpsChanged();
            }

            setRas();
        } else {
            s.integer(chipMemMask);
            s.integer(slowMemSize);
            s.integer(fastMemSize);
        }

        s.integer(dmaChipMemMask);

        s.array(chipMem, chipMemMask + 1 + slowMemSize);

        if (fastMemSize)
            s.array(fastMem, fastMemSize);

        if (model == OCS_A1000)
            s.array(wom, 256 * 1024);

        s.integer(overclock.speed);        
    } else {
        s.array(&mapper[0], 8);
        s.array(&mapper[0xf8], 4);

        if (s.mode() == Emulator::Serializer::Mode::Load) {
            MemChange* ptr;
            if (chipMemChangePos) {
                for (int i = chipMemChangePos - 1; i >= 0; i--) {
                    ptr = &chipMemChange[i];
                    *(uint16_t*)(chipMem + ptr->address) = ptr->value;
                }
            }
            if (slowMemChangePos) {
                for (int i = slowMemChangePos - 1; i >= 0; i--) {
                    ptr = &slowMemChange[i];
                    *(uint16_t*)(slowMem + ptr->address) = ptr->value;
                }
            }
            if (fastMemChangePos) {
                for (int i = fastMemChangePos - 1; i >= 0; i--) {
                    ptr = &fastMemChange[i];
                    *(uint16_t*)(fastMem + ptr->address) = ptr->value;
                }
            }
            trackMemChanges = false;
        } else {
            chipMemChangePos = slowMemChangePos = fastMemChangePos = 0;
            trackMemChanges = true;
        }
    }

    blitter.serialize(s);
    copper.serialize(s);
    powerSupply.serialize(s);

    for (auto expansion : expansions)
        expansion->serialize(s, light);
}

}

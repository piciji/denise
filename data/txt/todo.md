
# todo's are not ordered by priority. changes frequently
## to do C64
* expansion port
    * RetroReplay
    * simple non game mapper: Simons Basic, Magic Formel, WarpSpeed, ...
    * EasyFlash 3
    * Final Cartridge 3
    * GeoRAM
    * MMC64
    * MMC Replay    
    * SuperCPU
    * Turbo Chameleon 64
    * Dongles
    * Expansion expander
    * clockport emulation (i.e. Retro Replay, MMC)
    * all the others
* user port
    * SpeedDOS
    * 4 player adapter
* floppy
    * 1541 track change delay time
    * non standard floppy models: 1571, 1581
* tape port   
    * tape content listing in user interface, like the 1541
    * click entries and load them without fast-forward to counter position before
    * dongles
* pseudo stereo SID
* autoload for all media types with D64/T64 viewer in file dialog
* color banding

# to do Amiga A500, A1200
* 68000/68010 opcode tester
* blitter
* copper
* denise ocs
* system / memory map
* disk
* paula	
* ipf disk image format
* hard disk
* 68020
* lisa aga	

# todo for all systems
* screenshots/movie recording
* screenshots for savestates
* run frame ahead for reduced input lag
* drive leds / sounds 
* beam racing
* rewind support
* debug monitor for developers
* directx 10 and higher because of SPIR-V support
* vulkan support
* SPIR-V Shader support
* RetroArch Shader support
* 7z support
* refactoring Linux GTK2 UI to GTK3
* use a CI system like AppVeyor

# todo improvements
* fullscreen with custom refresh rate
* separate settings file by emulation cores
* auto start should reuse already opened instance (enable in UI)
* rework some UI tags to make host menu better understandable for user
* remove all C64 individual stuff from host menu (mostly hotkeys)
* autofire with frequency, option to fire without button press (don't forget override logic)
* switch joyports as hotkey
* load/save custom settings
* load PRG files from disk as simple RAM insertion for reduced loading times
* UI switch to invert RAM init pattern
* write (SID/PAULA) output to WAV 
* open files as read only from OS point of view
* set "integer scaling" multiplicator without sizing application window
* Hotkeys for Power/Softreset

# bugs
* Raphnet Adapter for N64 doesn't work with RawInput
* japanese characters aren't displayed correctly in OpenGL onscreen text
* more Ram Init pattern problems (https://csdb.dk/release/?id=172238  https://csdb.dk/release/?id=172347)


# completed emulation
* cpu 68000 / 68010
* cpu 6502 / 6510	
* cia 6526 / 8520	
* vicII 6569R3 / 8565 / 6567R8 / 8562
* sid 6581 / 8580
* c64 system / memory / ultimax
* c64 8k/16k standard cartridges
* c64 game cartridge mapper (ocean, system 3, funplay, supergames, zaxxon)
* c64 memory injection (prg, p00, T64) + save memory to file
* c64 tape + write support
* c64 multi diskdrive 1541-II + write support (d64, g64)
* c64 mouse 1351, mouse neos, paddles, inkwell lightpen, stack lightpen
* c64 gun sticks, magnum light phaser, stack light rifle
* c64 savestates
* c64C (custom ic logic)
* c64 custom color palettes + color spectrum emulation
* c64 ActionReplay MK2, MK3, MK4, V 4.1 - 6
* c64 REU
* c64 EasyFlash + write support

# completed features for all emu cores
* multi driver support
* platform independant OS UI support
* Dynamic Rate Control
* external GLSL shader support
* PAL/CRT emulation ( cpu driven or GLSL shader)
* Command line support
* Drag'n'drop
* multi bios support
* multi keyboard layouts
* free border cropping
* Hotkeys
* disk swapper (more usefull for amiga)
* screenshots for VICE testbench
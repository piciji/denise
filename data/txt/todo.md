
# todo's are not ordered by priority. changes frequently
## to do C64
* expansion port    
    * simple non game mapper: Simons Basic, Magic Formel, WarpSpeed, ...
    * EasyFlash 3
    * Final Cartridge 3
    * GeoRAM
    * MMC64
    * MMC Replay    
    * SuperCPU
    * Dongles
    * Expansion expander
    * clockport emulation (i.e. Retro Replay, MMC)
	* calling BBS's
    * all the others
* user port
    * SpeedDOS
    * 4 player adapter
* disk    
    * support command line start of disk listings
    * load PRG files from disk as simple RAM insertion for reduced loading times [quickload]
    * mount OS folder as D64
    * non standard floppy models: 1571, 1581
* tape   
    * content listing in user interface, like disks
    * click entries and load them without fast-forward to counter position before
    * Dongles
* color banding
* support to put clipboard in C64 memory
* UI for custom RAM init patterns
    

# todo Amiga A500, A1200
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

# todo for all emulated systems
* load savestates per drag'n'drop or within File Explorer
* screenshots/movie recording
* screenshots for savestates
* drive sounds 
* beam racing
* rewind support
* debug monitor for developers
* directx 10 and higher
* vulkan support
* SPIR-V Shader support
* RetroArch Shader support
* 7z support
* 100 Hz Black Frame insertion
* fullscreen with custom refresh rate
* autofire with frequency and option to fire without button press
* set "integer scaling" multiplicator without sizing application window OR recalculate while resizing (never show black border)
* PNG overlay for 16:9 fullscreen
* Netplay

# completed emulation
* cpu 68000 / 68010
* cpu 6502 / 6510	
* cia 6526 / 8520	
* vicII 6569R3 / 8565 / 6567R8 / 8562 / 6569R1 / 6567R56A / 6572 / 6573
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
* c64 RetroReplay / Nordic Replay + write support
* c64 stereo + 8x SID support

# completed features for all emu cores
* multi driver support
* platform independant native UI support
* Dynamic Rate Control
* external GLSL shader support
* PAL/CRT emulation ( cpu driven or GLSL shader)
* Command line support
* Drag'n'drop
* multi bios support
* multi keyboard layouts
* free border cropping
* Hotkeys
* disk swapper
* screenshots for VICE testbench
* use a CI system for automated builds (AppVeyor)
* runAhead for reduced input lag
* write audio output to WAV
* DSP: Bass Boost and Reverb
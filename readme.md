
[![Build status](https://ci.appveyor.com/api/projects/status/5xq83txi0tfv222a?svg=true)](https://ci.appveyor.com/project/piciji/denise)

![Denise Logo](https://deniseemu.sourceforge.io/logo.png)

[Downloads](https://sourceforge.net/projects/deniseemu/files/) - [IssueTracker](https://bitbucket.org/piciji/denise/issues) - [Nightlies](https://ci.appveyor.com/project/piciji/denise/history) - [Build info](https://bitbucket.org/piciji/denise/src/master/data/txt/buildinfo) - [License](https://bitbucket.org/piciji/denise/src/master/data/txt/licence)

#### developed with

[![CLion logo](https://deniseemu.sourceforge.io/CLion_logo1.png)](https://www.jetbrains.com/?from=Denise)

Copyright © 2021 JetBrains s.r.o. CLion and the CLion logo are registered trademarks of JetBrains s.r.o.

# changelog

## 1.1.2 (not yet released)
* P64 support
* 1541, 1541C support
* 1570/1571 support
    * support of D71 / G71 / P71 formats
    * Burst Modification
    * MFM support
      * P64
      * G64 (U-II+/U64 compatibility mode)
* Floppy RAM expansions
* Floppy Fastloader
    * SpeedDOS
    * DolphinDOS v2, v3, Ultimate Hack
    * ProfDOS v1, R3, R4, R5, R6
    * PrologicDos Original and Classic
    * Turbo Trans ( includes Turbo Access )
    * ProSpeed 1571 GTI v2.0 
    * support Userport Kernals for all fast loader, expect Turbo Trans
    * support Expansionsport Kernals for ProfDOS, PrologicDOS, Turbo Trans
* bugfix: switch to custom resolutiuon in fullscreen
* auto insert newly created disks or tapes
* increase App initial loading time
* add drive sounds
  * support for multiple profiles
  * simply create own WAV folders and select it
* support command line start of disk entries, besides Load "*"
    * Frontends like Assembly64 support this
* Virtual Device Traps
    * Fast Load first file
    * can be combined with Warp
* add hungarian translation [thanks to Ferenc]
* add Just in time polling for faster input recognition
* Catalina and above prompts to allow keyboard monitoring if not already so

## 1.1.1
* add Final Cartridge 3 support to EF³
* add DataBlast, SwiftLink, Turbo232 emulation
    * BBS support (use tcpser for virtual modem)
    * only virtual IP based connections, no connections to physical COM ports
* fix VIA Latch Bug  (thanks to enigma)
* prepare fullscreen with custom resolution and refresh rate
* auto fast forward (warp) on startup
    * stop when first file was loaded
    * or fast forward while drive (tape, floppy) is running
* Bugfix: creating new disks/tapes on macOS Catalina and above was not possible

## 1.1.0
* save/load additional settings files for a lot of purposes, like
    * define keyboard inputs for individual games one time only
    * prepare different C64 models
    * prepare individual Multi SID configurations
* rework sub-menu handling within configuration window
* rework status bar (Drive LED's, Tape control)
    * status bar is switchable via Hotkey in windows/fullscreen (Options / Hotkeys)
    * show LED for EasyFlash and EF³
* add UI for custom RAM init patterns
* load savestates per drag'n'drop or from File Explorer
* support to paste clipboard or copy screen to clipboard
* fix OpenGL 1/4 screen BUG for newer macOS versions
* Gmod2 cartridge mapper (i.e. Sams Journey)
    * support Flash and Eeprom writes
* Magic Desk cartridge mapper	
* Final Cartridge I, II, Plus, III, III+
* Simons Basic, Warp Speed
* Atomic Power, Mach5, Pagefox, Ross, Westermann
* expansion port expander to use REU + Retro Replay together
* GeoRAM
* EasyFlash³
    * hotkey for menu switch 
    * support Super Snapshot 5, Retro(Nordic) Replay, Atomic(Nordic) Power
    * Kernal replacement
    * optional 64 MBit mode
        * create a single EF3 file (slot 0) from all slots during emulation
        * strip down single EF3 file (slot 0) to all slots (Note: multi CRT file writing can trigger a false positive in your security app) 

## 1.0.9.1
* fix TAPE emulation (broken in 1.0.9)
* nice performance improvement
* load follow-up disks via hotkey
    * map hotkeys for disk 1, 2, 3, ...
    * map disk 0 for reinserting boot disk
    * emulator guesses file name for requested disk and insert it
    * override guessing of file names by assigning disks in "Disk Swapper"

## 1.0.9
* runAhead
* add faster scanline renderer (optional)
    * adequate for games with higher runAhead
    * inadequate for some demos, which depend on cycle renderer
* emulate missing VIC-II models
* improve SID emulation
    * multiple filter models
    * pseudo stereo
    * 8x SID support
    * DSP: Bass Boost and Reverb
* write audio output to WAV file
* PRG can be loaded as D64
* performance improvements
* thanks to user AW182 for the countless tests and reports


## 1.0.8
* added Retro/Nordic Replay support
* added macOS DMG installer [thanks to Retrofan for background image]
* added xInput emulation for Windows rawInput driver [XBOX Controller, xMode devices]
* fixed a few input handling bugs
* added slider for analog trigger point when using for digital inputs
* added help output in console: Denise -h
* refactored GTK2 to GTK3 for Linux port
* added hotkey to switch controller ports
* added hotkeys to trigger power and soft reset
* reworked menu structure and moved some settings in order to find them faster
* moved some global hotkeys to emulator specific hotkeys, i.e. load/save states, SID control
* reworked firmware view
* added posibillity to swap in CHAR roms during active emulation
* reworked software view
* show placeholder picture when emulator is opened but still not running an emulation [thanks to Retrofan]
* autoload for all media types with D64/T64 viewer in file dialog
* added possibility to associate files with Denise for macOS
* emulated left vertical line anomaly in overscan area
* added confirmation dialog to write on disk/tape/flash permanently 
* open files as read only from OS point of view, if not able to open it in read/write mode
* added possibility to customize D64 preview box in file dialog
* improved D64 preview generation in UI to better match original

## 1.0.7
* fixed a critical bug that caused OSX builds to use illegal instructions for some architectures
* added EasyFlash support
* fixed widget layouting, when app/text scaling is activated by Windows OS
* added screenshot generation for testbench
* added double step function of drive head motor (Primitive 7 Sins)
* added drive motor deceleration
* added slider to adjust drive motor speed and wobble
* removed read latch from drive mechanic, only VIA is latching readed byte
* distinguish between physical and logical tracks for disk content preview in UI
* fixed a rare bug in gpu driven RF Modulation, when disabling luma 'fall' but not 'rise'
* added a new aggressive fast-forward mode, which disables VIC-II Sequencer for a few frames
* combined key presses (ALT + W) don't trigger single keys when partially released
    * i.e. if ALT is released a few milliseconds sooner than 'W', it doesn't print a 'W'
* bugfixed RawInput: some joystick types were not registered
* unplugged joypads will not be forgot anymore
* transfer file names of loaded software to the savestate description field
* added diagonal joypad directions as optional virtual keys to activate it by single keyboard stroke
* added new application icon [thanks to Retrofan]
* added new application logo in project pages [thanks to Retrofan]
* added japanese translation [thanks to Ulgon]
* launch associated files in fullscreen (from command line, or file association)
* fixed a few minor macOS display bugs

## 1.0.6
* polished OS X UI ... looks ok now for Mojave dark theme
* Windows command line support is now independant from working directory of caller
* added option to manually save settings
* reworked expansion port emulation
    * added REU support with additional 8k rom
    * added Action Replay MK2, MK3, MK4, V4.1 and higher
    * support Cartridge bin format
* GIT repo is public now: [Bitbucket](https://bitbucket.org/piciji/denise/src/master/)
* simplified build process

## 1.0.5
* drag'n'drop support
* firmware paths will be saved and applied for loaded save states
* prepare multiple firmware configurations and switch between them
* auto detect language and keyboard layout during first start
* added alternate configuration for input elements (i.e. shift left/right and plus )
* command line support + testbench support
* color palette selection and creation
* color spectrum generation by Pepto's new findings (Colodore)
* added integer scaling
* cpu driven CRT emulation
    * pal delay line
    * chroma subsampling
    * hanover bars
    * rf modulation 
* gpu driven CRT emulation (OpenGL only at the moment)
    * like cpu with more effects
    * Sinc FIR lowpass filter for bandwidth reduction
    * Vic-II luma glitches
    * luma/chroma noise, random line offset
    * shadow mask, aperture grille, cromaclear
    * bloom
    * radial distortion 
* fixed a lot of misspellings in german translation (thanks Arndt)

## 1.0.4
* added Dynamic Rate Control
    * smooth audio + video at the same time
    * rewrote all audio drivers
    * added xaudio2, wasapi exclusive mode
    * show audio buffer usage statistics
    * added hotkey to enable/disable all sync options at once
* disabled emulator input recognition when minimized
* separated c64 and amiga sub menus from each other

## 1.0.3.1
* reduced drive thread cpu usage greatly (a extra core has consumed permanently 100%)
    * some system settings, which consumes additional cpu power, are highlighted
* improved user input capture process
    * it's now easier to assign multiple inputs at once (i.e. Alt + Shift + S)
    * it's now selectable to overwrite (default) or append an existing mapping
* added kernal, basic, char and 1541 bios files
    * can be replaced by custom versions  

## 1.0.3
* added more control port devices
    * mouse 1351, mouse neos, paddles, magnum light phaser, stack light rifle, inkwell lightpen, stack lightpen
    * dual Gun Sticks support
* multi mice support [for windows raw input driver only at the moment]
    * fast swap of connected control port devices
* keyboard auto assignable [free assigning of single keys is still possible]
    * french, german, uk and us keyboard layouts supported
    * macOS keyboard layouts supported
    * virtual keys added [means: single key triggers key combination of emulated keyboard ]
* savestate support
    * save/load your program at any position ( even possible while disk/tape is loading )
    * method 1: standard save/load file dialog
    * method 2: hotkeys for save / load / slot up / slot down
    * assign save slots per game
    * Note: hotkeys can be assigned not only to keyboards  
* added custom ic logic (C64C)
* french translation added
    * thanks Ben
* bugfixes
    * [sid] fixed osc3 register read
    * [sid] accidently delayed Triangle/Sawtooth output for 6581 instead of 8580
    * [via] reworked shift emulation: fixes vmax4 galaxian thunder mountain

* read below, i have added more usage hints for savestates, keyboard layouts and control port devices

## 1.0.2
* added floppy 1541-II support
    * support for d64, g64 images
    * emulates up to 4 drives
    * correct drive <-> drive synchronisation (e.g. Maverick Dual Drive Copy)

## 1.0.1
* added tape support
    * includes write emulation
    * counter calculation + complete derivation of formula
* fixed some undocumented opcodes: arr, sha, shs, shx, shy
* fixed sei/cli behaviour while cpu enters wait state
* rewrote vic sequencer again
    * sequencer can react now on changed x-scroll during scanline
    * implemented all known pixel delays for register changes [ thanks to vice team ]
    * added sprite crunching
    * differs between 65xx and 85xx chip revisions
    * added grey dot bug emulation
* improved cia code
    * fixed delays for control register changes
    * differs between new and old cia
    * improved tod emulation
    * improved keyboard matrix emulation [ thanks to vice team ]

## 1.0.0
* added t64, prg, p00 support
* included C64 TrueType v1.2/Style for prg listing
* loaded prg or self written code can be saved as prg file
* rewrote vic sequencer ... fixes hyper screen demo: Intro Collection (19xx)(Jewels)

## 0.9.9 beta
* initial release
* c64 cartdrige emulation only


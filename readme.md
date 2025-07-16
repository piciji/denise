
[![Build status](https://ci.appveyor.com/api/projects/status/5xq83txi0tfv222a?svg=true)](https://ci.appveyor.com/project/piciji/denise)

![Denise Logo](https://deniseemu.sourceforge.io/logo.png)

[Downloads](https://sourceforge.net/projects/deniseemu/files/) - [IssueTracker](https://sourceforge.net/p/deniseemu/tickets) - [Nightlies Windows | macOS](https://ci.appveyor.com/project/piciji/denise/history) - [Nightlies Linux](https://sourceforge.net/projects/deniseemu/files/nightlies/) - [Build info](https://bitbucket.org/piciji/denise/src/master/data/buildinfo) - [License](https://bitbucket.org/piciji/denise/src/master/licence.md)

# changelog

## 2.6
* Amiga: added harddisk emulation
    * built-In HD Controller
    * M-Tec AT 500
    * VHD support for dynamic HDFs
* Amiga: added ECS Denise emulation (A500+/A600)
* Amiga: added option to overclock CPU
* Amiga: support for floppy disks in SCP format
* Amiga: added CD32 gamepad support
* Amiga: added option to show Caps Lock LED in status bar
* C64: added file dialog option to prevent speeder for next load
    * the speeder + firmware does not have to be switched time-consumingly for certain programs
* C64: added file dialog preview for cartridges
* C64: improved PRG injection (rebuild linked list)
    * Note: a PRG can also be loaded as a D64
* Windows 10/11: added dark UI mode    
* added LHA support
    * added option to convert LHA archives to Amiga native filesystem (ADF, HD-ADF or HDF)
* APP can be used fully or partially portable
* added file history to make it easier to re-insert files
* improved Shader management
    * fixed an error when parsing shaders (Bezel: koko-aio works again)
    * added option to automatically download and unpack shaders

## 2.5
* added 1581 floppy drive
* added SuperCPU
    * support SuperCPU + REU (e.g. Sonic without slowdowns)
    * added option to boost DRAM accesses (speedup Wolf3D)
    * support Metal Dust burst loader (new CIA's only and in UI check CIA Burst modification)
* added Final Chesscard
    * support options to overclock (up to 115 MHz)
    * hint: latest BROM v3.6 from IBM PC ISA-cartridge can be used too    
* added MagicDesk 2 (SNK vs CAPCOM Stronger Edition)
* added Easycalc, Hyperbasic, Businessbasic cartridges
* added StarDOS, SuperCard+
* added 4 and 8 player adapter for C64
* added option to toggle 1541C track zero sensor
* Linux: built flatpak
* Linux: improve detection of Bluetooth controller
* Windows: support USB adapter for DB9 retro joysticks
* additional sound profile for C64 added to differentiate between 5 1/4" and 3.5" drives
* configurations are shown in a tree view for subfolder support
* updated reSID code
* hide mouse cursor after 2 seconds of inactivity and becomes visible again when moved
* fix VIC-II sprite collision bug
* fix A1000 blitter busy bug
* fix A1000 WOM lock during reset instruction

## 2.4
* support macOS Metal graphics driver with shaders
* added Cocoa as a keyboard driver for macOS
    * avoid the system dialog to allow input
* added BSD UHID joypad driver
    * supported on FreeBSD, NetBSD and OpenBSD
* reworked on-screen status messages
    * adjust position, font size, font type, foreground/background color, transparency, box padding/margin
    * download TTF/OTF/TTC fonts or add OS installed fonts for win c:/Windows/Fonts, mac /Library/Fonts, linux /usr/share/fonts
* added esperanto translation (thanks to Diego)
* separated(C64/Amiga) file dialog preview options and moved to Software UI
* improved openGL shader cache
* improved handling of shader UI
* added possibility to assign a different floppy sound profile for external drives
* Amiga: added new floppy sound profile
* Amiga: emulate Blitter<>Copper conflict
* C64: fixed a bug in G64 emulation
* C64: added exponential function to simulate drive motor acc/deceleration
* C64: improved D64 track alignment

## 2.3
* added RetroArch Shader support
    * targets: openGL, D3D11
    * converted existing GLSL/HLSL Shaders to SLANG
    * fast switching of shaders from a favourite list (shader cache and compilation without blocking App)
    * for shader development, compilation errors and translated native code are displayed in the UI
    * looking for Shader authors, read more in the next paragraph
* Amiga: improved emulation, especially the disk drive
    * thanks to amilo3438 for testing a huge number of games and demos
* Amiga: emulated Bitplane <> Refresh conflicts
* Amiga: Final Fight Enhanced supported by temporary option to fake ECS Denise  
* Amiga: written floppy disks are now saved in a second file
    * this does not apply to uncompressed ADF (don't worry, emulator asks if ADF can be changed)
    * permanent writes are now possible for IPF, compressed images, EXT ADF, DMS
    * the folder for the save files can be customized under Amiga > Software > Paths
* Amiga: added 4-player adapter
* Amiga: added copy protection dongle support
* C64: besides PRG, P00 and T64 are loadable as D64
    * Drag'n'Drop PRG on Software TAB -> Disk file slot OR
    * use filedialog and change file type to "all files"
* C64: fixed Hucky Cart emulation (thanks to Claus)
* fixed a bug not finishing Drag'n'Drop in Linux GTK
* fixed some OSX bugs/crashes [looking for tester]
    * ARM builds can now handle IPF capsimg library
    * fix UI file dialog crashes
* improved mouse capture handling for all OSes
* UI changes
    * many global options are distributed among the individual emulator options
    * volume slider has moved to status bar
    * Shader controls have been reworked
    * all hotkey assignments are now in the same place
    * added option to capture pointer by left mouse button
* changed BuildSystem to CMAKE
* added Linux/BSD XCB input driver for keyboard/mouse
* improved GTK window resizing in some desktop environments 

## looking for Shader authors
* interested in shaders for
    * [Luma/Chroma crosstalk, probably last unsupported C64 effect](https://bitbucket.org/piciji/denise/issues/1/video-old-vic2-chroma-effect-on-odd-chars)
    * wrap image with 1702/1084S/... monitor and C64/Amiga floppy drive
* new option for SLANGP files: luma_chroma
    * Luma Chroma input to prevent lossy conversion from RGB
    * Alpha Channel is used for Pixel meta data: C64 VIC2 AEC and BA signal (affect luminance)
    * add a few pixels around the image, which will be cropped later between shader passes (e.g., calculate FIR Filter in border or first PAL delay line)
* there are parameters that are not visible or can be operated by user
    * the emulator sets them automatically and can be used by the shader author
    * autoEmu_driveLED, autoEmu_cropTop, autoEmu_cropLeft, autoEmu_lace, autoEmu_hires, autoEmu_pal, autoEmu_subRegion

## 2.2.1
* fixed spanish translation

## 2.2
* added D3D11 driver
    * openGL has bad VSYNC/VRR behavior on Windows 11 (replace it with D3D11: Options->Video->Driver)
    * ported internal and external openGL shader to D3D11 
* added italian translation (thanks to Luigi)
* added an option to rotate the screen (90°/180°/270°)
* added an option to force a single instance when opening another instance from frontend loader or file explorer
* fixed Amiga basic volume (louder)
* improved 1571 emulation
* added C64 cartridges: Comal-80, Silverrock, RGCD
* added a more easy way to set stereo separation (Audio -> DSP)
* added tapes to swapper (mixed disc/tape swapper)
* added option to decide if manual warping stops automatic warping until next restart
* added more options to control the screen aspect ratio
* added button/hotkey to crop the outer frame after resizing or changing "aspect ratio" or emulated border

## 2.1
* add drag'n'drop overlay
    * files can be inserted into additional drives faster
    * files can either only be inserted or inserted and restarted
    * multi file support
* fix: Wasapi didn't work on some audio hardware  
* thanks to all translators: Ben, Ulgon, Ferenc, Muzza
* [Amiga]
* greatly improved accuracy, thanks to amilo3438, vAmigaTS, CPU Tester, AW182
* ~15% speed up
* fix: Drive LED sometimes does not turn off
* display tracks more clearly
* add IPF, DMS, EXE and encrypted kick rom support
* add RTC
* add multi file support for any-loader (fill DF0-3 in one go)
* show power LED in status line
    * colors for power and drive LEDs depend on the model
    * click on power LED to select audio filter
* [C64]
* fix: multi SID UI not updated correctly from config
* fix: sometimes crashes when changing speeder
* add cartridges: Dinamic, Diashow Maker, Super Snapshot V5
* add option to intensify pseudo stereo (now also works with same SID models)

## 2.0
* add Amiga emulation
    * A1000 (OSC/ICS), A500 (Full OCS), A500 (ECS Agnus, OCS Denise)
    * RGB or S-Video with PAL/NTSC color encoding, border cropping
    * support all global features like: runAhead, savestates, G-Sync, configs, Warp, JIT polling, shader
    * up to 4 disk drives with acceleration option
    * Chipram, Slowram, Fastram
    * list disk content in UI
    * AROS firmware is preinstalled
    * motor controlled auto warp
    * drive sounds
    * support custom frequencies like B.C. Kid (56.4 Hz)
    * if you only want to use one of the two emulators, Amiga or C64 core can be hidden
* emulator now boots automatically after power on
    * Splash screen can be disabled
    * emulator remembers which core was used last
* disk swapper can be filled faster due to multiple selection of files
    * files from archives can also be added quickly
    * if requested disk swap position is not prepared, emulator guesses the disk based on requested position and the currently inserted disk
* windows: prevent App minification when focus loss in fullscreen
* C64: add option to combine Virtual Device Traps (instant load first file) with floppy speeder
* C64: improve drive motor controlled auto warp
    * add option to stop auto warp when software requests input, because
    * some games stop drive motor too late and warp runs too long, what could lead to the loss of a life
* C64: support magicdesk CRT's with broken header
* C64: fix runahead slowdown with REU/GeoRAM when using a lot of memory, e.g. REU 16 MB
* Mice and other mouse-controlled devices, such as Lightguns, can now be configured with one click

## NOTES
* Mouse Capturing
    * In order to operate the Amiga/C64 mouse, it must first be captured. The middle mouse button is designed for this purpose by default. Of course, another key or key combination can also be used for this.
    * Press the "Hotkeys" button under Control / Amiga Configuration. A double click on the first entry "capture mouse" allows you to assign a new key(s).
* Dutch Translation
    * someone created a ticket and offered a Dutch translation for Denise. Unfortunately, I reacted too late
    * and the creator certainly never read the answer. If you're reading this. Yes, I am interested and in other languages too, of course.
* VRR (G-Sync, FreeSync)
    * [Windows] keep in mind that the old DirectX9 video driver can apply VRR only in "exclusive" fullscreen (not in windows, not in normal fullscreen)
    * [Windows] for VRR you should select openGL driver in Denise. So it works in window and fullscreeen mode
    * [Linux,macOS] keep in mind that VRR works only in fullscreen mode [not an emulator limitation]
    

## 1.1.3.1
* thanks for the countless tests and suggestions [thanks to AW182]
* thanks for spanish translation [thanks to Muzza]
* tape loading sounds (playing as audio cassette)
* fixed a bug for windows UI not listing all addresses for multi SID's in drop-down list
* faster loading of floppy drive profile with a lot of stepper WAV's for each track
    * newtronics drive sound profile no longer blocks emulation on first start
* added option to hold autofire a little bit longer
    * a few games benefit from it, applies to all active Turbo functions
    * hint: some autofire compatible joysticks do this
* added option to prioritise Controlport over keyboard and vice versa
    * double assignments are temporarily inactivated
    * e.g. using cursor keys as joystick replacement

## 1.1.3
* added newtronics drive sound profile [thanks to enigma]
* precise FPS counter and speed selection menu
    * define a custom speed
    * define decimal place and refresh interval for FPS counter
* added option to render in a separate thread [threaded renderer]
    * improves VSync for non standard refresh rates, like 50.125 Hz
    * new default setting, because most reliable
* added option for adaptive sync (not to be confused with VRR)
    * automatically selects the best sync option for requested emulation speed
    * outperforms threaded renderer, but it needs more attention
    * first it needs threaded renderer to be disabled
    * need to detect correct refresh rate (use new sync hotkey to confirm if it fits)
    * refresh rate detection works for primary screen only (at the moment)
    * threaded renderer will be enabled automatically if display refresh rate differs from user requested emu speed
    * skips frames when user requested emu speed is a multiple of display refresh rate
* fixed jerky scrolling for Linux and macOS
* added option to change the size of the window in the correct aspect ratio
* added option to emulate in a separate thread besides User Interface
    * unblocked audio/video when browsing the file dialog or moving/sizing window
* fix support for VRR displays (G-Sync/FreeSync)
    * reduce input lag by 20 ms (PAL)
    * don't forget to enabale VRR in your monitor OSD or display software
    * set display refresh rate to maximum
    * don't forget to disable VSync in emulator
    * Linux and macOS don't support VRR in window mode (not an emulator problem)
    * old direct3D9 driver in Denise support it only in exclusive fullscreen
    * use openGL in Windows (supports VRR in window and fullscreen mode)
    * when changing the emu speed, within the VRR interval, the scrolling remains soft
    * without adjusting the display refresh rate you can switch between 50.125 Hz (PAL) and 59.826 Hz (NTSC)
* using revision 1/2 kernals when selecting old VIC's
    * only happens if standard firmware is selected
* show tape content in User Interface
    * start files without manual fast-forward
    * fast-forward to selected file without autostartig it
    * Virtual Device Traps: load tape files (PRG or Turbo Tapes) instantly
* added autofire
    * typical or self-firing continuous fire (superimposed by a press on the fire button)
    * turbo fire for left/right directions (shaking games)
    * adjustable autofire frequency 
* added option to allow opposite directions
    * by default, simultaneous pressing of opposite directions (left<>right, top<>down) is ignored
* added support for second fire button (Cheetah Annihilator)
* added tape drive sounds
    * like floppies, you can add your own sound profiles
    * you can send me new profiles and I will integrate them permanently with the next release
* FreeBSD support
* macOS M1 support
* a lot of fixes

## 1.1.2
* P64 support
* 1541, 1541C support
* 1570/1571 support
    * support of D71 / G71 / P71 formats
    * Burst Modification
    * MFM support
        * P64
        * G64 (U-II+/U64 compatibility mode)
* Floppy RAM expansions and parallel cable
* Floppy Fastloader
    * SpeedDOS
    * DolphinDOS v2, v3, Ultimate compatible
    * ProfDOS v1, R3, R4, R5, R6
    * PrologicDos Original and Classic
    * Turbo Trans with 512k Ram Disk ( includes Turbo Access )
    * ProSpeed 1571 GTI v2.0
    * Expansionsport cart for ProfDOS, PrologicDOS, Turbo Trans
    * Userport plug-in for DolphinDOS, ProfDOS, PrologicDOS, ProSpeed 1571
* bugfix: switch to custom resolution in fullscreen
* auto insert newly created disks or tapes
* decrease App initial loading time
* add drive sounds
    * samples provided by [thanks to enigma and Trackers World]
    * support for multiple profiles
    * simply create own WAV folders and select it
* support command line start of disk entries, besides Load "*"
    * Frontends like Assembly64 support this
* Virtual Device Traps
    * fast load first file
    * can be combined with Warp
* add hungarian translation [thanks to Ferenc]
* add "Just in time" polling for faster input recognition
* Catalina and above prompts to allow keyboard monitoring if not already so
* display CPU JAM in status bar

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

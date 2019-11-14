
license -> in source: data/txt/licence  
build info -> in source: data/txt/buildinfo  
todo -> in source: data/txt/todo  
usage hints -> scroll down this file  

# changelog

## 1.0.6
* polished OS X UI ... looks ok now for Mojave dark theme
* Windows command line support is now independant from working directory of caller
* added option to manually save settings
* reworked expansion port emulation
    * added REU support with additional 8k rom
    * added Action Replay MK2, MK3, MK4, V4.1 and higher
    * support Cartridge bin format
* GIT repo is public now
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

---

# Usage Hints

### savestates are configured here: system/state manager
you can save/load direct within a file dialog or save/load/change slot by hotkeys.
hotkeys can be assigned to any input device.
you can specifiy a folder for hotkey driven states, otherwise the states will be saved
to a subfolder in your system application folder.
furthermore you can specify a file ident, which is used for hotkey driven state filenames.
this way you can distinguish between different games.
the 'find' button lists all states, which include the inputed file ident.
a double click on a state loads the state.  
for each portable state file exists a non portable image file. this file contains
the inserted images and firmware paths. by loading a state the emulator tries to load these images.
when deleting this file or image paths are outdated you have to insert the images by yourself
before loading a state.  
if not you will get a problem when the game is loading additional content.
Note: a loaded state insert the control port devices which were active when generating the state.
the mapping of these devices is not touched.  
when emulation core changes, older savestates can not be loaded anymore. you get an error message.

### keyboard layouts are configured here: input/configuration/
first select 'keyboard' in dropdown list.  
in the bottom part of configuration window select your prefered keyboard layout and click 'Assign'.
you can overmap single keys by your own.  
virtual keys are key combinations of the emulated system. i.e. c64 don't have keys for cursor up/left.
using a real c64, you need to press 2 keys same time to trigger cursor up/left.
with virtual keys you are able to map cursor up/left to a single key.  
'generic' option is usefull only if your layout is not supported and the emulated system
was produced with different keyboard layouts, like amiga.
this way you get a description of the physical position of some keys in order to map them by yourself.  
there is an alternate mapping for each input element. (i.e. shift left/right and plus) 

### control port devices
light guns/pens are controlled by OS screen cursor or by emulated screen cursor.

* OS cursor:
    * moves more smooth than emulated cursor
    * cursor speed adjustment is controlled within the OS
    * doesn't stop at screen border
    * cursor has always the same size, screen window size doesn't matter.
* emulated cursor:
    * cursor speed can be adjusted in global input configuration [hotkey section]
    * stops at screen border
    * cursor size depends on screen window size

keep in mind that c64 shares control lines internally between keyboard and devices connected to control ports.
some devices, like neos mouse or gunsticks, cause problems when c64 expects keyboard input.
it's better to connect them after inputing the loading command.

### c64 prg is configured here: system/drives/memory
after loading a memory image you can select it in the list view below.
a double click or return causes a power cycle, memory injection and execution of 'run' command.
use the 'injection' button to inject memory in the running system without executing 'run'.
you can save a loaded or self coded program as 'prg' file by selecting 'Create' Tab.  
a p64 image can not be mounted as a disk drive at the moment, so each included prg is on its own.

### disk is configured here: system/drives/disk drives
set the amount of connected disk drives in system menu.
you can insert a disk writable or not. this can be switched later without reinserting the disk.
you will see the directory of the disk in a list view below. 
double click an entry causes a power cycle and loads the directory entry.
create an empty disk by selecting 'Create' Tab.
you can prepare up to 15 diskimages in system/disk swapper.
assign disk swapper slots to hotkeys. during emulation you can insert them fast. (more usefull for amiga)

### c64 tape is configured here: system/drives/tape drives
powering the emulator adds a tape control menu in main menu bar.
you can map the control options as hotkeys too.
set the amount of connected tape drives in system menu.
you can insert a tape writable or not. this can be switched later without reinserting tape.
create an empty tape by selecting 'Create' Tab.
pressing 'record' with activated write protection informs you.
loading and writing the tape will be displayed with counter position in status bar.  
when software has stopped the tape, control menu 'play' / 'record' keeps hilighted
because tape deck button is still pressed and software can resume later (i.e. load next level)

### c64 cartdrige is configured here: system/drives/modules
following custom mappers are supported: ocean, system 3, funplay, supergames, zaxxon  
ultimax memory mode is supported

### c64 sid is configured here: system/management/feature
for 6581 emulation you should adjust 'filer bias' to simulate different center frequencies.  
you can map sid control to hotkeys, so you can switch between the sid models with a joypad button press.  
'Sid Hazard' is experimental modeling of all incomming voltages in Filter/Mixer
as individual transistors instead of one virtual. (runs in own thread)

### shader support (opengl / direct3d)  
first you need shaders or write your own shaders.
higan shader library is mostly supported.
set shader folder and graphic driver in 'settings/video'.  
activate shaders in 'C64/shader/'.
errors will be displayed in a message box.
you can combine multiple shaders.

### border cropping
when removing complete border in combination with the option 'maintain screen ratio'
a short border will remain.  
there are some games which use sprites as background elements in border area.
in this case you shouldn't remove the complete border but use free cropping instead.  
for a realistic result you should use 'Monitor' option.

### compressed archives
zip, gz, tar, tar.gz archives are supported.  
for archives with more than one file a treeview widget will open.
selected files in archives will be memorized when restarting the emulator.  
a compressed emulator image can not be inserted as writable.

---

## c64 creating turbo tapes

### method 1 (slow way)
* create empty tape_1 for non 'turbo taped' games
* create another empty tape_2 for turbo taped games
* power on c64	
* inject game prg, it's a simulated basic load
* insert tape_1
* save "game"
    * Note: basic can not handle huge 202 block games. (prints: out of memory)
    * restart and load the game for nostalgic reasons only.
* restart
* inject turbo tape prg
* insert tape_2
* save "turbo tape"
* run
* insert tape_1
* load "game", i like it to waste my lifetime this way
* reinsert tape_2 and fast forward to end of tape
* now we save the game in turbo format: <-S "game"
* restart and test it: load, run, <-L (compare by yourself how fast it loads), run

### method 2 (fast way)
by the first way you can not save too big games because of the basic memory limit
and you have to save and load the game in non turbo tape mode before.

* create empty tape for turbo taped games
* power on c64	
* inject turbo tape prg
* use mr.z turbo 250 for huge games
    * NOTE: some games doesn't work, like Masters of the universe (the movie, he-man 2)
      because of turbo tape resides in memory too.
    * use 61-K turbo tape. it solves the problem of he-man and works with huge games too.
* insert empty tape
* save "turbo tape"
* run
* inject game prg, it's a simulated basic load
* now we save the game in turbo format: <-S "game"
* restart and test it: load, run, <-L, run

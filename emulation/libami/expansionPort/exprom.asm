;
; Can be compiled with vasm: vasmm68k_mot exprom.asm -Fbin -phxass
;
; KS1.2 magic based on http://eab.abime.net/showpost.php?p=1045113&postcount=3
;
    ORG $40

RomStart=0

VERSION=0
REVISION=5

DEBUG=0

OP_INIT=$fedf
OP_IOREQ=$fede
OP_SETFSRES=$fee0
OP_FSINFO=$fee1
OP_FSINITSEG=$fee2
OP_VOLUME_GET_ID=$fee3
OP_VOLUME_INIT=$fee4
OP_VOLUME_PACKET=$fee5

RT_MATCHWORD=$00		; UWORD word to match on (ILLEGAL)
RT_MATCHTAG=$02			; APTR  pointer to the above (RT_MATCHWORD)
RT_ENDSKIP=$06			; APTR  address to continue scan
RT_FLAGS=$0A			; UBYTE various tag flags
RT_VERSION=$0B			; UBYTE release version number
RT_TYPE=$0C			; UBYTE type of module (NT_XXXXXX)
RT_PRI=$0D			; BYTE  initialization priority
RT_NAME=$0E			; APTR  pointer to node name
RT_IDSTRING=$12			; APTR  pointer to identification string
RT_INIT=$16			; APTR  pointer to init code
RT_SIZE=$1A

RTC_MATCHWORD=$4afc
RTF_AUTOINIT=$80	; rt_Init points to data structure
RTF_COLDSTART=$01

NT_UNKNOWN=0
NT_TASK=1
NT_INTERRUPT=2
NT_DEVICE=3
NT_MSGPORT=4
NT_MESSAGE=5
NT_FREEMSG=6
NT_REPLYMSG=7
NT_RESOURCE=8
NT_BOOTNODE=16

; exec.library
_LVOFindResident=-96
_LVOInitResident=-102
_LVOAlert=-108
_LVOForbid=-132
_LVOPermit=-138
_LVOAllocMem=-198
_LVOFreeMem=-210
_LVOAllocEntry=-222
_LVOAddHead=-240
_LVOAddTail=-246
_LVOEnqueue=-270
_LVOAddTask=-282
_LVOSetTaskPri=-300
_LVOWait=-318
_LVOPutMsg=-366
_LVOGetMsg=-372
_LVOReplyMsg=-378
_LVOWaitPort=-384
_LVOOldOpenLibrary=-408
_LVOCloseLibrary=-414
_LVODoIO=-456
_LVOAddResource=-486
_LVOOpenResource=-498
_LVOOpenLibrary=-552

ThisTask=$114
ResourceList=$150

; expansion.library
_LVOAddBootNode=-36
_LVOObtainConfigBinding=-120
_LVOReleaseConfigBinding=-126
_LVOSetCurrentBinding=-132
_LVOGetCurrentBinding=-138
_LVOMakeDosNode=-144
_LVOAddDosNode=-150

eb_MountList=74

; dos.library
_LVOCreateProc=-138
_LVODeviceProc=-174
_LVODateStamp=-192
_LVODelay=-198
 ; rev 36+
_LVOLockDosList=-654
_LVOUnLockDosList=-660
_LVOAttemptLockDosList=-666
_LVOAddDosEntry=-678

dl_Root=$0022

; struct RootNode
rn_Info=$0018

; struct DosInfo
di_DevInfo=$0004

pr_MsgPort=$5c
pr_FileSystemTask=$a8
pr_WindowPtr=$b8

INITBYTE=$e000
INITWORD=$d000
INITLONG=$c000

LN_SUCC=$00
LN_PRED=$04
LN_TYPE=$08
LN_PRI=$09
LN_NAME=$0A
LN_SIZE=$0E

LH_HEAD=$00         	; APTR	
LH_TAIL=$04         	; APTR	
LH_TAILPRED=$08     	; APTR	
LH_TYPE=$0C         	; UBYTE	
LH_SIZE=$0E

LIB_FLAGS=$0E ; LN_SIZE
LIB_NEGSIZE=$10
LIB_POSSIZE=$12
LIB_VERSION=$14
LIB_REVISION=$16
LIB_IDSTRING=$18
LIB_SUM=$1C
LIB_OPENCNT=$20
LIB_SIZE=$22

LIBF_SUMMING=$01	; we are currently checksumming
LIBF_CHANGED=$02	; we have just changed the lib
LIBF_SUMUSED=$04	; set if we should bother to sum
LIBF_DELEXP=$08         ; delayed expunge

; ConfigDev
cd_Node=$00
cd_Flags=$0E
cd_Rom=$10 ; struct ExpansionRom 16 bytes
cd_BoardAddr=$20

CDB_CONFIGME=1
ADNF_STARTPROC=1

; starts at LN_SIZE
MN_REPLYPORT=$0E ; APTR   message reply port
MN_LENGTH=$12    ; UWORD  total message length in bytes
MN_SIZE=$14
		
IOERR_OPENFAIL=-1	; device/unit failed to open
IOERR_ABORTED=-2	; request terminated early [after AbortIO()]
IOERR_NOCMD=-3	        ; command not supported by device
IOERR_BADLENGTH=-4	; not a valid length (usually IO_LENGTH)
IOERR_BADADDRESS=-5	; invalid address (misaligned or bad range)
IOERR_UNITBUSY=-6	; device opens ok, but requested unit is busy
IOERR_SELFTEST=-7	; hardware failed self-test 

IOB_QUICK=0

; starts at MN_SIZE
IO_DEVICE=$14			; APTR    device node pointer
IO_UNIT=$18			; APTR    unit (driver private)
IO_COMMAND=$1C                  ; UWORD   device command
IO_FLAGS=$1E                    ; UBYTE   special flags
IO_ERROR=$1F	                ; BYTE    error or warning code
IO_SIZE=$20

IO_ACTUAL=$20			; ULONG   actual # of bytes transfered
IO_LENGTH=$24			; ULONG   requested # of bytes transfered
IO_DATA=$28			; APTR    pointer to data area
IO_OFFSET=$2C			; ULONG   offset for seeking devices
IOSTD_SIZE=$30

CMD_READ=$2
TD_MOTOR=$9
TD_CHANGENUM=$D
TD_CHANGESTATE=$E


PA_SIGNAL=0	; Signal task in mp_SigTask
PA_SOFTINT=1	; Signal SoftInt in mp_SoftInt/mp_SigTask
PA_IGNORE=2	; Ignore arrival

; starts at LN_SIZE
MP_FLAGS=$0E                    ; UBYTE
MP_SIGBIT=$0F		        ; UBYTE    signal bit number
MP_SIGTASK=$10		        ; APTR     object to be signalled
MP_MSGLIST=$14	                ; STRUCT   message linked list
MP_SIZE=$22

; starts at MP_SIZE
UNIT_FLAGS=$22                  ; UBYTE
UNIT_OPENCNT=$24                ; UWORD
UNIT_SIZE=$26

; starts at LN_SIZE
bn_Flags=$0E                    ; UWORD
bn_DeviceNode=$10               ; APTR
BootNode_SIZEOF=$14

MEMF_PUBLIC=$1
MEMF_CLEAR=$10000

; struct MemEntry
me_Addr=0
me_Length=4
me_Sizeof=8

; struct MemList
ml_Node=0
ml_NumEntries=$e ; UWORD
ml_ME=$10        ; struct MemEntry[]

; struct Task
tc_Node        = $0000
tc_Flags       = $000e
tc_State       = $000f
tc_IDNestCnt   = $0010
tc_TDNestCnt   = $0011
tc_SigAlloc    = $0012
tc_SigWait     = $0016
tc_SigRecvd    = $001a
tc_SigExcept   = $001e
tc_TrapAlloc   = $0022
tc_TrapAble    = $0024
tc_ExceptData  = $0026
tc_ExceptCode  = $002a
tc_TrapData    = $002e
tc_TrapCode    = $0032
tc_SPReg       = $0036
tc_SPLower     = $003a
tc_SPUpper     = $003e
tc_Switch      = $0042
tc_Launch      = $0046
tc_MemEntry    = $004a
tc_UserData    = $0058
tc_Sizeof      = $005c

; Internal device structure
dev_SysLib=$22  ; LIB_SIZE
dev_SegList=$26
dev_Base=$2A
dev_FileSysRes=$2e
dev_Sizeof=$32

; struct DeviceNode
dn_Next=$0000
dn_Type=$0004
dn_Task=$0008
dn_Lock=$000c
dn_Handler=$0010
dn_StackSize=$0014
dn_Priority=$0018
dn_Startup=$001c
dn_SegList=$0020
dn_GlobalVec=$0024
dn_Name=$0028
dn_Sizeof=$002c

; struct CurrentBinding
cb_ConfigDev=$0000
cb_FileName=$0004
cb_ProductString=$0008
cb_ToolTypes=$000c
cb_Sizeof=$0010

; Starts at UNIT_SIZE
devunit_Device=$26              ; APTR
devunit_UnitNum=$2A             ; ULONG NOTE used in emulator, offset must be kept in sync
devunit_Sizeof=$2E

; struct FileSysResource
fsr_Node=$0000                  ; struct Node
fsr_Creator=$000e               ; char*
fsr_FileSysEntries=$0012        ; struct List
fsr_Sizeof=$0020

; struct FileSysEntry
fse_Node=$0000                  ; struct Node
fse_DosType=$000e               ; ULONG
fse_Version=$0012               ; ULONG
fse_PatchFlags=$0016            ; ULONG
fse_Type=$001a                  ; ULONG
fse_Task=$001e                  ; CPTR
fse_Lock=$0022                  ; BPTR
fse_Handler=$0026               ; BSTR
fse_StackSize=$002a             ; ULONG
fse_Priority=$002e              ; LONG
fse_Startup=$0032               ; BPTR
fse_SegList=$0036               ; BPTR
fse_GlobalVec=$003a             ; BPTR
fse_Sizeof=$003e

; Parameter packet for MakeDosNode
devn_dosName=$00        ; APTR  Pointer to DOS file handler name
devn_execName=$04       ; APTR  Pointer to device driver name
devn_unit=$08           ; ULONG Unit number
devn_flags=$0C          ; ULONG OpenDevice flags
devn_tableSize=$10      ; ULONG Environment size
devn_sizeBlock=$14      ; ULONG # longwords in a block
devn_secOrg=$18         ; ULONG sector origin -- unused
devn_numHeads=$1C       ; ULONG number of surfaces
devn_secsPerBlk=$20     ; ULONG secs per logical block -- unused
devn_blkTrack=$24       ; ULONG secs per track
devn_resBlks=$28        ; ULONG reserved blocks -- MUST be at least 1!
devn_prefac=$2C	        ; ULONG unused
devn_interleave=$30     ; ULONG interleave
devn_lowCyl=$34	        ; ULONG lower cylinder
devn_upperCyl=$38       ; ULONG upper cylinder
devn_numBuffers=$3C     ; ULONG number of buffers
devn_memBufType=$40     ; ULONG type of memory for AmigaDOS buffers
devn_transferSize=$44   ; LONG  largest transfer size (largest signed #)
devn_addMask=$48        ; ULONG address mask
devn_bootPrio=$4c       ; ULONG boot priority
devn_dName=$50 	        ; char[4] DOS file handler name
devn_bootFlags=$54	; ULONG boot flags (filled in by host)
devn_segList=$58        ; BPTR segList (filled in by host)
devn_Sizeof=$5c	        ; Size of this structure

; struct FileSysStartupMsg
fssm_Unit     = $0000
fssm_Device   = $0004
fssm_Environ  = $0008
fssm_Flags    = $000c
fssm_Sizeof   = $0010

; struct DosPacket
dp_Link=$0000
dp_Port=$0004
dp_Type=$0008
dp_Res1=$000c
dp_Res2=$0010
dp_Arg1=$0014
dp_Arg2=$0018
dp_Arg3=$001c
dp_Arg4=$0020
dp_Arg5=$0024
dp_Arg6=$0028
dp_Arg7=$002c
dp_Sizeof=$0030

ACTION_STARTUP=0
ACTION_LOCATE_OBJECT=8

DOSTRUE=-1
DOSFALSE=0

; struct DosList
dol_Next=$0
dol_Type=$4
dol_Task=$8

; Volume (dol_misc.dol_volume)
dol_VolumeDate=$10
dol_DiskType=$20
; Handler (dol_misc.dol_handler) -- for devices
dol_Startup=$1c

dol_Name=$28
dol_Sizeof=$2c

ID_DOS_DISK='DOS\0'
DLT_DEVICE=0
DLT_VOLUME=2

LDF_WRITE=2
LDF_DEVICES=4
LDF_VOLUMES=8
LDF_ASSIGNS=16
LDF_ALL=LDF_DEVICES+LDF_VOLUMES+LDF_ASSIGNS


; Filesystem info
fsinfo_num=$00          ; UWORD Filesystem number (filled in by expansion ROM)
fsinfo_dosType=$02      ; ULONG dos type (filled by host)
fsinfo_version=$06      ; ULONG dos type (filled by host)
fsinfo_numHunks=$0a     ; ULONG number of hunks (filled by host)
fsinfo_hunk1=$0e        ; ULONG mem flags for hunk1 (filled by host)
fsinfo_hunk2=$12        ; ULONG mem flags for hunk2 (filled by host)
fsinfo_hunk3=$16        ; ULONG mem flags for hunk3 (filled by host)
fsinfo_Sizeof=$1a

DiagStart:
            dc.b $90                 ; da_Config = DAC_WORDWIDE|DAC_CONFIGTIME
            dc.b $00                 ; da_Flags = 0
            dc.w EndCopy-DiagStart   ; da_Size (in bytes)
            dc.w DiagEntry-DiagStart ; da_DiagPoint (0 = none)
            dc.w BootEntry-DiagStart ; da_BootPoint
            dc.w DevName-DiagStart   ; da_Name (offset of ID string)
            dc.w $0000               ; da_Reserved01
            dc.w $0000               ; da_Reserved02

RomTag:
            dc.w    RTC_MATCHWORD      ; UWORD RT_MATCHWORD
rt_Match:   dc.l    RomTag-DiagStart   ; APTR  RT_MATCHTAG
rt_End:     dc.l    EndCopy-DiagStart  ; APTR  RT_ENDSKIP
            dc.b    RTF_AUTOINIT+RTF_COLDSTART ; UBYTE RT_FLAGS
            dc.b    VERSION            ; UBYTE RT_VERSION
            dc.b    NT_DEVICE          ; UBYTE RT_TYPE
            dc.b    20                 ; BYTE  RT_PRI
rt_NamePtr: dc.l    DevName-DiagStart  ; APTR  RT_NAME
rt_Id:      dc.l    IdString-DiagStart ; APTR  RT_IDSTRING
rt_InitPtr: dc.l    Init-DiagStart     ; APTR  RT_INIT


; Padding to allow change to longer name
DevName:    dc.b 'virtualhd.device', 0, 0, 0, 0, 0, 0, 0, 0, 0
IdString:   dc.b 'virtualhd ',VERSION+48,'.',REVISION+48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
DosLibName: dc.b 'dos.library', 0
    even

Init:
            dc.l    dev_Sizeof ; data space size
            dc.l    funcTable-DiagStart
            dc.l    dataTable-DiagStart
            dc.l    initRoutine-RomStart

funcTable:
            dc.l   Open-RomStart
            dc.l   Close-RomStart
            dc.l   Expunge-RomStart
            dc.l   Null-RomStart	    ;Reserved for future use!
            dc.l   BeginIO-RomStart
            dc.l   AbortIO-RomStart
            dc.l   -1

dataTable:
            DC.W INITBYTE, LN_TYPE
            DC.B NT_DEVICE, 0
            DC.W INITLONG, LN_NAME
dt_Name:    DC.L DevName-DiagStart
            DC.W INITBYTE, LIB_FLAGS
            DC.B LIBF_SUMUSED+LIBF_CHANGED, 0
            DC.W INITWORD, LIB_VERSION, VERSION
            DC.W INITWORD, LIB_REVISION, REVISION
            DC.W INITLONG, LIB_IDSTRING
dt_Id:      DC.L IdString-DiagStart
            DC.W 0   ; terminate list

BootEntry:
        lea     DosLibName(pc), a1
        jsr     _LVOFindResident(a6)    ; find the DOS resident tag
        move.l  d0, a0
        move.l  RT_INIT(a0), a0         ; set vector to DOS INIT
        jmp     (a0)                    ; initialize DOS

; a0=BoardBase, a2=DiagCopy, a3=configDev
DiagEntry:
        lea     patchTable-RomStart(a0), a1
        move.l  a2, d1
dloop:
        move.w  (a1)+, d0
        bmi.b   bpatches
        add.l   d1, 0(a2,d0.w)
        bra     dloop
bpatches:
        move.l  a0, d1
bloop:
        move.w  (a1)+, d0
        bmi.b   endpatches
        add.l   d1, 0(a2,d0.w)
        bra     bloop
endpatches:

        ; KS1.2?
        move.l  $4.w, a1
        cmp.w   #34, LIB_VERSION(a1)
        bcc.b   de_Out

        move.l  a0, d0 ; Save board base
        lea     BoardBase(pc), a0
        move.l  d0, (a0)
        lea     ConfigDev(pc), a0
        move.l  a3, (a0)
        ; Patch PutMsg
        move.l  _LVOPutMsg+2(a1), OldPutMsg-DiagStart(a2)
        lea     NewPutMsg(pc), a0
        move.l  a0, _LVOPutMsg+2(a1)

        ; Autoboot disabled?
        move.l  BoardBase(pc), a0
        tst.w   RomCodeEnd+4(a0)
        bne.b   de_Out

        ; and DoIO
        move.l  _LVODoIO+2(a1), OldDoIO-DiagStart(a2)
        lea     NewDoIO(pc), a0
        move.l  a0, _LVODoIO+2(a1)

de_Out:
        moveq   #1,d0 ; success
        rts

DOSSTACKSIZE=2048
NewPutMsg:
        movem.l d0/a1, -(sp)
        ; Check if DOS packet
        ; ln_Name must be non-zero
        move.l  LN_NAME(a1), d0
        beq.b   npm_Cont
        move.l  d0, a1
        ; ln_Name must be longword aligned
        and.b   #3, d0
        bne.b   npm_Cont
        ; Packet we're waiting for?
        cmp.l   #ACTION_LOCATE_OBJECT, dp_Type(a1)
        bne.b   npm_Cont

        ; Remove patch
        move.l  OldPutMsg(pc), _LVOPutMsg+2(a6)
        ; Now do what romboot.library handles in KS1.3
        movem.l d1-d7/a0-a6, -(sp)
        ; Allocate temporary stack
        move.l  #DOSSTACKSIZE, d0
        moveq   #0, d1
        jsr     _LVOAllocMem(a6)
        ; Assume this doesn't fail..
        add.l   #DOSSTACKSIZE, d0
        move.l  d0, a0
        move.l  sp, -(a0)
        move.l  a0, sp
        lea     RomTag(pc), a0
        move.l  BoardBase(pc), a1
        move.l  ConfigDev(pc), a3
        jsr     HandleResInit-RomStart(a1)
        move.l  d0, d7 ; store return value
        move.l  (sp)+, a1 ; Old stack
        exg.l   a1, a7
        move.l  #DOSSTACKSIZE, d0
        sub.l   d0, a1
        move.l  $4.w, a6
        jsr     _LVOFreeMem(a6)
        move.l  d7, d0
        movem.l (sp)+, d1-d7/a0-a6

        ; d0 is either 0 or DeviceProc() return value for boot drive
        tst.l   d0
        beq     npm_Cont

        ; Check if autoboot is disabled
        move.l  BoardBase(pc), a1
        tst.w   RomCodeEnd+4(a1)
        bne.b   npm_Cont

        ; Replace pr_FileSystemTask
        move.l  d0, a0 ; Replace msg port
        move.l  ThisTask(a6), a1
        move.l  a0, pr_FileSystemTask(a1) ; And filesystask

npm_Cont:
        movem.l (sp)+, d0/a1
        dc.w    $4ef9 ; JMP ABS.L
OldPutMsg:
        dc.l    0
BoardBase:
        dc.l    0
ConfigDev:
        dc.l    0

NewDoIO:
        movem.l a2/a3, -(sp)
        move.l  BoardBase(pc), a2
        lea     OldDoIO(pc), a3
        jsr     HandleDoIO-RomStart(a2)
        ; Preserve flags
        movem.l (sp)+, a2/a3
        beq.b   ndi_NotHandled
        rts
ndi_NotHandled:
        dc.w    $4ef9 ; JMP ABS.L
OldDoIO:
        dc.l    0
MotorState:
        dc.l    0

EndCopy:

patchTable:
; Word offsets into Diag area where pointers need Diag copy address added
        dc.w   rt_Match-DiagStart
        dc.w   rt_End-DiagStart
        dc.w   rt_NamePtr-DiagStart
        dc.w   rt_Id-DiagStart
        dc.w   rt_InitPtr-DiagStart
        dc.w   Init-DiagStart+$4
        dc.w   Init-DiagStart+$8
        dc.w   dt_Name-DiagStart
        dc.w   dt_Id-DiagStart
        dc.w   -1
; Word offsets into Diag area where pointers need boardbase+ROMOFFS added
        dc.w   Init-DiagStart+$c
        dc.w   funcTable-DiagStart+$00
        dc.w   funcTable-DiagStart+$04
        dc.w   funcTable-DiagStart+$08
        dc.w   funcTable-DiagStart+$0C
        dc.w   funcTable-DiagStart+$10
        dc.w   funcTable-DiagStart+$14
        dc.w   -1


; registers preserved in NewPutMsg
; a0 = romtag, a1 = boardbase, a3 = configdev, a6 = execbase
; returns d0 = DeviceProc() for bootable parition (or 0)
HandleResInit:
        move.l  a0, d6 ; d6=romtag
        move.l  a1, d7 ; d7=boardbase

        moveq   #0, d4 ; DeviceProc() of bootable parition

        lea     ExLibName(pc), a1
        moveq   #0, d0
        jsr     _LVOOpenLibrary(a6)
        tst.l   d0
        beq     hri_Out
        move.l  d0, a5  ; a5 = expansion library

        exg.l   a5, a6 ; a6=expansion library, a5=execbase

        jsr     _LVOObtainConfigBinding(a6)

        sub.l   #cb_Sizeof, a7

        moveq   #0, d0
        move.l  d0, cb_FileName(a7)
        move.l  d0, cb_ProductString(a7)
        move.l  d0, cb_ToolTypes(a7)
        move.l  a3, cb_ConfigDev(a7)
        moveq   #cb_Sizeof, d0
        move.l  a7, a0
        jsr     _LVOSetCurrentBinding(a6)

        exg.l   a5, a6 ; a6=execbase, a5=expansion

        move.l  d6, a1
        moveq   #0, d1
        jsr     _LVOInitResident(a6)

        exg.l   a5, a6 ; a6=expansion library, a5=execbase
        add.l   #cb_Sizeof, a7

        jsr     _LVOReleaseConfigBinding(a6)

        ; Close expansion.library
        move.l  a6, a1
        move.l  a5, a6
        jsr     _LVOCloseLibrary(a6)

        lea     DosLibName(pc), a1
        jsr     _LVOOldOpenLibrary(a6)
        tst.l   d0
        beq     hri_Out
        move.l  d0, a5
        exg.l   a5, a6 ; a6 = dos.library, a5 = exec.library

        ;
        ; Now call DeviceProc on all partitions
        ; This seems to be necessary even if they're
        ; not bootable?? FIXME
        ;

        moveq   #0, d5 ; Unit counter
hri_DevInit:
        cmp.w   RomCodeEnd(pc), d5 ; Done?
        beq     hri_DevsDone

        ; Room for dos packet
        sub.l   #devn_Sizeof, a7
        move.l  a7, a3

        ; Room for name and ':'
        sub.l   #34, a7

        ; Have emulator fill out dos packet (again)
        move.l  a7, devn_dosName(a3)
        lea     DevName(pc), a0
        move.l  d5, devn_unit(a3)

        lea     RomCodeEnd(pc), a0
        move.l  a3, (a0)
        move.w  #OP_INIT, 4(a0)

        ; Add ':' to name
        move.l  a7, a0
        moveq   #0, d0
hri_FindNul:
        cmp.b   (a0)+, d0
        bne.b   hri_FindNul
        move.b  d0, (a0)
        move.b  #':', -1(a0)

        move.l  a7, d1
        jsr     _LVODeviceProc(a6)
        ; Ignore failure here (for now)

        ; Do we already have a bootable partition?
        tst.l   d4
        bne.b   hri_Next

        ;  Is this parition bootable?
        btst.b  #0, devn_bootFlags+3(a3)
        beq.b   hri_Next

        move.l  d0, d4 ; Store device process

hri_Next:
        add.l   #devn_Sizeof+34, a7 ; free dos packet + name
        addq.w  #1, d5
        bra     hri_DevInit
hri_DevsDone:


        ; Any shared folders?
        lea     RomCodeEnd(pc), a3
        move.w  6(a3), d3
        moveq   #0, d2
.checkshare:
        cmp.w   d2, d3
        beq     .sharedone

        ; Get volume name
        move.l  d2, -(sp)
        move.l  a7, a2
        move.l  a2, (a3)
        move.w  #OP_VOLUME_GET_ID, 4(a3)
        move.l  (sp)+, a1 ; a1 = bptr to volume name

        add.l   a1, a1
        add.l   a1, a1  ; to cptr

        sub.l   #34, a7 ; room for name
        move.l  a7, a0
        moveq   #0, d0
        move.b  (a1)+, d0
.copy:
        subq.w  #1, d0
        bmi.b   .copyend
        move.b  (a1)+, (a0)+
        bra.b   .copy
.copyend:
        move.b  #':', (a0)+
        move.b  #0, (a0)+

        ifne    DEBUG
        move.l  a7, a0
        bsr     SerPutMsg
        bsr     SerPutCrLf
        endc

        move.l  a7, d1
        jsr     _LVODeviceProc(a6)
        tst.l   d4
        bne.b   .gotboot
        move.l  d0, d4 ; store device proc (if first)

.gotboot:
        add.l   #34, a7

        addq.w  #1, d2
        bra     .checkshare
.sharedone:

        exg.l   a5, a6 ; a6 = dos.library, a5 = exec.library
        move.l  a5, a1
        jsr     _LVOCloseLibrary(a6)
hri_Out:
        move.l  d4, d0 ; return DeviceProc() result
        rts

; a1 = ioRequest (ok to trash a2), a3=OldDoIO
; Returns whether the command was handled in Z-flag (NZ=handled)
HandleDoIO:
        movem.l d0-d2/a0-a1, -(sp)
        moveq   #0, d2 ; Assume unhandled command
        move.l  IO_DEVICE(a1), a2
        move.l  LN_NAME(a2), a2
        lea     trackdiskName(pc), a0
hdi_Compare:
        move.b  (a0)+, d0
        cmp.b   (a2)+, d0
        bne     hdi_Out
        tst.b   d0
        beq.b   hdi_Match
        bra.b   hdi_Compare
hdi_Match:
        move.w  IO_COMMAND(a1), d0
        cmp.w   #CMD_READ, d0
        beq     hdi_Read
        cmp.w   #TD_MOTOR, d0
        beq     hdi_Motor
        cmp.w   #TD_CHANGENUM, d0
        beq     hdi_ChangeNum
        cmp.w   #TD_CHANGESTATE, d0
        beq     hdi_ChangeState
hdi_Out:
        tst.l   d2 ; Return handled in zero
        ; DON'T TOUCH FLAGS ANYMORE!!
        movem.l (sp)+, d0-d2/a0-a1
        rts
hdi_Read:
        ; Remove patch
        move.l  (a3), _LVODoIO+2(a6)

        ; And provide OFS boot block as result
        lea     ofsBootBlock(pc), a0
        move.l  IO_DATA(a1), a2
        move.l  #ofsBootBlockEnd-ofsBootBlock-1, d0
hdi_CopyLoop:
        move.b  (a0)+, (a2)+
        dbf     d0, hdi_CopyLoop
        move.l  IO_LENGTH(a1), d0
        move.l  d0, IO_ACTUAL(a1)
        sub.l   #ofsBootBlockEnd-ofsBootBlock, d0
        moveq   #0, d1
hdi_ClearLoop:
        move.b  d1, (a2)+
        dbf     d0, hdi_ClearLoop
        move.b  #0, IO_ERROR(a1)
        moveq   #1, d2 ; Handled
        bra     hdi_Out
hdi_Motor:
        lea     MotorState-OldDoIO(a3), a2
        move.l  (a2), IO_ACTUAL(a1)
        move.b  #0, IO_ERROR(a1)
        move.l  IO_LENGTH(a1), (a2)
        move.b  IO_FLAGS(a1), d0
        moveq   #1, d2 ; Handled
        bra     hdi_Out
hdi_ChangeNum:
        ; Simulate disk inserted (IO_ACTUAL=0)
        move.l  #1, IO_ACTUAL(a1)
        move.b  #0, IO_ERROR(a1)
        moveq   #1, d2 ; Handled
        bra     hdi_Out
hdi_ChangeState:
        ; Simulate disk inserted (IO_ACTUAL=0)
        moveq   #0, d0
        move.l  d0, IO_ACTUAL(a1)
        move.b  d0, IO_ERROR(a1)
        moveq   #1, d2 ; Handled
        bra     hdi_Out

trackdiskName:
        dc.b 'trackdisk.device', 0
        even

ofsBootBlock:
        dc.w $444f, $5300, $c020, $0f19, $0000, $0370, $43fa, $0018
        dc.w $4eae, $ffa0, $4a80, $670a, $2040, $2068, $0016, $7000
        dc.w $4e75, $70ff, $60fa, $646f, $732e, $6c69, $6272, $6172
        dc.w $7900
ofsBootBlockEnd:

; d0 = device pointer, a0 = segment list
; a6 = exec base
initRoutine:
        movem.l d1-d7/a0-a6,-(sp)
        move.l  d0, a5   ; a5=device pointer
        move.l  a6, dev_SysLib(a5)
        move.l  a0, dev_SegList(a5)
        sub.l   a4, a4

        ; Open expansion library
        lea     ExLibName(pc), a1
        jsr     _LVOOldOpenLibrary(a6)
        tst.l   d0
        beq     irError
        move.l  d0, a4    ; a4=expansion library

        ; Check if we need to install any filesystems
        lea     RomCodeEnd(pc), a0
        move.w  2(a0), d6       ; d6 = number of filesystems to install
        beq     irFsOK

        ; Open FileSystem.resource
        lea     FsrName(pc), a1
        jsr     _LVOOpenResource(a6)
        tst.l   d0
        beq     irNoFsResource

        ; Inform host about FileSystem.resource
        lea     RomCodeEnd(pc), a0
        move.l  d0, (a0)
        move.w  #OP_SETFSRES, 4(a0)
        ; re-read number of file systems to install after host has had chance to prune list
        move.w  2(a0), d6
        bne     irHasFsResource
        bra     irFsOK

irNoFsResource:
        bsr     CreateFileSysResource
        tst.l   d0
        beq     irError

irHasFsResource:
        move.l  d0, dev_FileSysRes(a5)
        sub.l   #fsinfo_Sizeof, a7
        moveq   #0, d5 ; Fs counter
irFsLoop:
        move.l  a7, a1
        move.w  d5, fsinfo_num(a1)
        lea     RomCodeEnd(pc), a0
        move.l  a1, (a0)
        move.w  #OP_FSINFO, 4(a0)

        sub.l   #16, a7 ; fsseginfo
        move.l  a7, a3
        move.l  fsinfo_numHunks(a1), d3
        lea     fsinfo_hunk1(a1), a2
        moveq   #0, d2
irAllocHunkLoop:
        move.l  (a2)+, d0       ; Hunk size + flags
        move.l  d0, d1
        clr.w   d1
        swap    d1
        lsr.l   #8, d1
        lsr.l   #5, d1
        or.l    #MEMF_CLEAR, d1
        and.l   #$3FFFFFFF, d0
        lsl.l   #2, d0
        addq.l  #8, d0
        jsr     _LVOAllocMem(a6)
        move.l  d2, d1
        lsl.l   #2, d1
        move.l  d0, 0(a3,d1.w)
        addq.l  #1, d2
        cmp.l   d2, d3
        bne     irAllocHunkLoop

        move.l  d5, 12(a3)
        lea     RomCodeEnd(pc), a0
        move.l  a3, (a0)
        move.w  #OP_FSINITSEG, 4(a0)

        move.l  0(a3), d0 ; First hunk
        addq.l  #4, d0    ; Point to link
        lsr.l   #2, d0    ; BPTR
        move.l  d0, a3

        add.l   #16, a7

        move.l  #fse_Sizeof, d0
        move.l  #MEMF_CLEAR+MEMF_PUBLIC, d1
        jsr     _LVOAllocMem(a6)
        tst.l   d0
        beq     irError ; TODO: Need clean up...
        move.l  d0, a0

        move.l  a7, a1
        ; a0 = FileSysEntry
        ; a1 = fsinfo
        ; a3 = seglist
        move.l  fsinfo_dosType(a1), fse_DosType(a0)
        move.l  fsinfo_version(a1), fse_Version(a0)
        move.l  #$180, fse_PatchFlags(a0) ; Needed?
        move.l  #-1, fse_GlobalVec(a0)
        move.l  a3, fse_SegList(a0)
        lea.l   IdString(pc), a3
        move.l  a3, LN_NAME(a0)
        move.l  a0, a1
        move.l  dev_FileSysRes(a5), a0
        lea.l   fsr_FileSysEntries(a0), a0
        jsr     _LVOAddHead(a6)

        addq.w  #1, d5
        cmp.w   d5, d6
        bne     irFsLoop
        add.l   #fsinfo_Sizeof, a7

irFsOK:
        move.l  a4, a6 ; a6 = expansion library
        sub.l   #cb_Sizeof, a7
        move.l  a7, a0
        moveq   #cb_Sizeof, d0 ; Don't pass in less than sizeof(CurrentBinding) for KS1.3!
        jsr     _LVOGetCurrentBinding(a6)
        move.l  cb_ConfigDev(a7), d6         ; d6 = ConfigNode
        add.l   #cb_Sizeof, a7
        tst.l   d6
        beq     irError
        move.l  d6, a0
        move.l  cd_BoardAddr(a0), dev_Base(a5) ; Save base address
        bclr.b  #CDB_CONFIGME, cd_Flags(a0)    ; Mark as configured

        moveq   #0, d5 ; Unit counter
        cmp.w   RomCodeEnd(pc), d5 ; No units?
        beq     irNoUnits
irUnitLoop:
        ; Room for dos packet
        sub.l   #devn_Sizeof, a7
        move.l  a7, a3

        ; Room for name
        sub.l   #32, a7

        move.l  a7, devn_dosName(a3)
        lea     DevName(pc), a0
        move.l  a0, devn_execName(a3)
        move.l  d5, devn_unit(a3)
        move.l  #16, devn_tableSize(a3)

        lea     RomCodeEnd(pc), a0
        move.l  a3, (a0)
        move.w  #OP_INIT, 4(a0)

        move.l  a3, a0
        jsr     _LVOMakeDosNode(a6)
        tst.l   d0
        beq     irUnitError

        move.l  d0, a0 ; a0 = DosNode

        move.l  #-1, dn_GlobalVec(a0)
        move.l  devn_segList(a3), dn_SegList(a0) ; Seglist from host

        ; KS1.2?
        cmp.w   #34, LIB_VERSION(a6)
        bcs.b   irPrev34

        move.l  devn_bootFlags(a3), d0
        btst.l  #0, d0 ; Auto boot?
        bne.b   irBootNode

        moveq   #ADNF_STARTPROC, d1 ; Flags
irAddDosNode:
        ; a0 = dosNode
        moveq   #-128, d0 ; Boot priority (-128 = not bootable)
        jsr     _LVOAddDosNode(a6)
        tst.l   d0
        beq     irUnitError

        bra     irNextUnit

irPrev34:
        moveq   #0, d1 ; Flags for AddDosNode (don't use ADNF_STARTPROC here to avoid requester for "DEVS", DeviceProc starts it later)
        ; For OFS the global vector MUST be zero (BCPL linking rules)
        tst.l   dn_SegList(a0)
        bne.b   irAddDosNode
        move.l  #0, dn_GlobalVec(a0)
        bra.b   irAddDosNode

irBootNode:
        ; a0 = dosNode
        move.l  d6, a1 ; configDev
        move.l  devn_bootPrio(a3), d0 ; Boot priority
        moveq   #ADNF_STARTPROC, d1 ; flags
        bsr     AddBootNode
        tst.l   d0
        beq     irUnitError

        bra     irNextUnit

irUnitError:
        add.l   #devn_Sizeof+32, a7 ; Free name + dos packet
        bra     irError

irNextUnit:
        add.l   #devn_Sizeof+32, a7 ; Free name and dos packet
        addq.w  #1, d5 ; next unit
        cmp.w   RomCodeEnd(pc), d5 ; Done?
        bne     irUnitLoop

irNoUnits:

        ; Any shared folders?
        lea     RomCodeEnd(pc), a3
        move.w  6(a3), d3
        moveq   #0, d2
.addshare:
        cmp.w   d2, d3
        beq     .done

        ; Get volume name
        move.l  d2, -(sp)
        move.l  a7, a2
        move.l  a2, (a3)
        move.w  #OP_VOLUME_GET_ID, 4(a3)
        move.l  (sp)+, d1 ; d1 = bptr to volume name

        ifne DEBUG
        lea     irAddShareMsg1(pc), a0
        bsr     SerPutMsg
        move.l  d2, d0
        bsr     SerPutNum
        bsr     SerPutSpace
        move.l  d1, d0
        add.l   d0, d0
        add.l   d0, d0
        addq.l  #1, d0
        move.l  d0, a0
        bsr     SerPutMsg
        bsr     SerPutCrLf
        endc ;DEBUG

        move.l  d1, a0
        bsr     MakeDeviceNode
        tst.l   d0
        beq     irError

        ifne DEBUG
        movem.l a0/d0, -(sp)
        lea     irAddShareMsg2(pc), a0
        bsr     SerPutMsg
        bsr     SerPutNum
        lea     irAddShareMsg3(pc), a0
        bsr     SerPutMsg
        move.l  d0, a0
        move.l  dn_Startup(a0), d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        movem.l (sp)+, a0/d0
        endc

        move.l  d0, a0 ; deviceNode
        move.l  d6, a1 ; configDev
        moveq   #0, d0 ; Boot priority
        moveq   #ADNF_STARTPROC, d1 ; flags
        bsr     AddBootNode
        tst.l   d0
        beq     irError

        addq.w  #1, d2
        bra     .addshare
.done:

        move.l  a5, d0
        bra     irExit
irError:
        moveq   #0, d0
irExit:
        move.l  dev_SysLib(a5), a6 ; restore exec base

        cmp.l   #0, a4
        beq     irNoClose
        move.l  d0, -(sp)
        ; Close expansion library
        move.l  a4, a1
        jsr     _LVOCloseLibrary(a6)
        move.l  (sp)+, d0
        beq.b   irNoClose
        ; FIXME
        ; Avoid expunge from now on (AROS doesn't keep device open)
        addq.w  #1, LIB_OPENCNT(a5)
irNoClose:
        movem.l (sp)+, d1-d7/a0-a6
        rts

ExLibName:  dc.b 'expansion.library', 0
FsrName:    dc.b 'FileSystem.resource', 0
            even
        ifne DEBUG
irAddShareMsg1: dc.b 'Adding share ', 0
irAddShareMsg2: dc.b 'Device node: $',0
irAddShareMsg3: dc.b ' Startup packet: $',0
        even
        endc ; DEBUG

; ok = AddBootNode( bootPri, flags, deviceNode, configDev )
; D0                D0       D1     A0          A1
AddBootNode:
        cmp.w   #34, LIB_VERSION(a6)
        bcs.b   .prev34
        cmp.w   #36, LIB_VERSION(a6)
        bcs.b   .prev36
        jmp     _LVOAddBootNode(a6)
.prev34:
        moveq   #0, d1 ; Don't start process yet
        jmp     _LVOAddDosNode(a6)
.prev36:
        movem.l a2/a6, -(sp)

        movem.l a0-a1/d0-d1, -(sp)
        move.l  dev_SysLib(a5), a6
        moveq   #BootNode_SIZEOF, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        jsr     _LVOAllocMem(a6)
        move.l  d0, a2
        movem.l (sp)+, a0-a1/d0-d1
        cmp.l   #0, a2
        beq     .out
        move.b  #NT_BOOTNODE, LN_TYPE(a2)
        move.l  a1, LN_NAME(a2)
        move.w  d1, bn_Flags(a2)
        move.l  a0, bn_DeviceNode(a2)
        jsr     _LVOForbid(a6)
        lea     eb_MountList(a4), a0
        move.l  a2, a1
        jsr     _LVOEnqueue(a6)
        jsr     _LVOPermit(a6)
        moveq   #1, d0 ; success
.out:
        movem.l (sp)+, a2/a6
        rts

        cnop 0,4
DevNameBstr:
        dc.b 16, 'virtualhd.device',0
        cnop 0,4
; DE_BOOTBLOCKS=19
FakeEnviron:
        dc.l 0, 0, 0, 0, 0, 0, 0, 0
        dc.l 0, 0, 0, 0, 0, 0, 0, 0
        dc.l 0, 0, 0

; In: a0 = BPTR to volume name, d2 = share number
; Out: d0 = DeviceNode* (NULL on error)
MakeDeviceNode:
        movem.l d1/a0-a3/a6, -(sp)
        move.l  a0, a2

        move.l  $4.w, a6

        moveq   #fssm_Sizeof, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        jsr     _LVOAllocMem(a6)
        tst.l   d0
        beq.b   .out
        move.l  d0, a3
        move.l  d2, fssm_Unit(a3)
        lea     DevNameBstr(pc), a0
        move.l  a0, d0
        lsr.l   #2, d0 ; To BSTR
        move.l  d0, fssm_Device(a3)
        lea     FakeEnviron(pc), a0
        move.l  a0, d0
        lsr.l   #2, d0 ; To BPTR
        move.l  d0, fssm_Environ(a3)

        moveq   #dn_Sizeof, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        jsr     _LVOAllocMem(a6)
        tst.l   d0
        beq.b   .out

        move.l  d0, a0
        move.l  a2, dn_Name(a0)
        moveq   #-1, d1
        move.l  d1, dn_GlobalVec(a0)
        move.l  a3, d1
        lsr.l   #2, d1 ; To BPTR
        move.l  d1, dn_Startup(a0)
        lea     FakeSegList(pc), a1
        move.l  a1, d1
        lsr.l   #2, d1 ; to BPTR
        move.l  d1, dn_SegList(a0)
        move.l  #2048, dn_StackSize(a0)
.out:
        movem.l (sp)+, d1/a0-a3/a6
        rts

        ifne DEBUG
        ; a0 = List
PrintList:
        movem.l d0/a0/a1, -(sp)
        move.l  a0, a1
prLoop:
        tst.l   LN_SUCC(a1)
        beq     prDone
        move.l  a1, d0
        bsr     SerPutNum
        move.b  #$20, d0
        bsr     SerPutchar
        move.l  LN_NAME(a1), a0
        bsr     SerPutMsg
        bsr     SerPutCrLf
        move.l  LN_SUCC(a1), a1
        bra     prLoop
prDone:
        movem.l (sp)+, d0/a0/a1
        rts

SerPutchar:
        move.l  d0, -(sp)
        and.w   #$ff, d0
        or.w    #$100, d0       ; stop bit
        move.w  d0, $dff030
        move.l  (sp)+, d0
        rts

SerPutMsg:
        movem.l  d0/a0, -(sp)
spLoop:
        move.b  (a0)+, d0
        beq     spDone
        bsr     SerPutchar
        bra     spLoop
spDone:
        movem.l  (sp)+, d0/a0
        rts

; Print BSTR in d0
SerPutBstr:
        movem.l  d0-d1/a0, -(sp)
        add.l   d0, d0
        add.l   d0, d0
        move.l  d0, a0
        moveq   #0, d1
        move.b  (a0)+, d1 ; Length
.loop:
        subq.w  #1, d1
        bmi.b   .done
        move.b  (a0)+, d0
        bsr     SerPutchar
        bra.b   .loop
.done:
        movem.l  (sp)+, d0-d1/a0
        rts


SerPutCrLf:
        move.l  d0, -(sp)
        moveq   #'\r', d0
        bsr     SerPutchar
        moveq   #'\n', d0
        bsr     SerPutchar
        move.l  (sp)+, d0
        rts

SerPutSpace:
        move.l  d0, -(sp)
        moveq   #' ', d0
        bsr     SerPutChar
        move.l  (sp)+, d0
        rts

SerPutNum:
        movem.l d0-d2, -(sp)
        move.l  d0, d1
        moveq   #7, d2
spnLoop:
        rol.l   #4, d1
        move.w  d1, d0
        and.b   #$f, d0
        add.b   #$30, d0
        cmp.b   #$39, d0
        ble.b   spnPrint
        add.b   #39, d0
spnPrint:
        bsr     SerPutchar
        dbf     d2, spnLoop
        movem.l (sp)+, d0-d2
        rts
        endc ; DEBUG

        ifne DEBUG
OpenMsg: dc.b 'Device open unit=$',0
        even
        endc ;DEBUG

Open:   ; ( device:a6, iob:a1, unitnum:d0, flags:d1 )
        movem.l d0-d7/a0-a6, -(sp)

        ifne DEBUG
        movem.l d0/a0, -(sp)
        lea     OpenMsg(pc), a0
        bsr     SerPutMsg
        bsr     SerPutNum
        bsr     SerPutCrLf
        movem.l (sp)+, d0/a0
        endc ; DEBUG

        addq.w  #1, LIB_OPENCNT(a6) ; Avoid expunge during open
        move.l  a1, a2 ; IOB in a2

        moveq   #0, d2
        move.w  RomCodeEnd(pc), d2 ; Number of units
        cmp.l   d2, d0
        bge     OpenError ; Invalid unit

        move.l  d0, d4 ; UnitNum in d4

        ; Allocate memory for unit
        move.l  #devunit_Sizeof, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        move.l  a6, a3 ; device in a3
        move.l  dev_SysLib(a3), a6
        jsr     _LVOAllocMem(a6)
        move.l  a3, a6 ; restore device pointer
        tst.l   d0
        beq     OpenError
        move.l  d0, a3 ; unit=a3

        ; Initialize it
        move.l  a6, devunit_Device(a3)
        move.l  d4, devunit_UnitNum(a3)
        move.b  #PA_IGNORE, MP_FLAGS(a3)
        move.b  #NT_MSGPORT, LN_TYPE(a3)
        lea     DevName(pc), a0
        move.l  a0, LN_NAME(a3)

        move.l  a3, IO_UNIT(a2)
        addq.w  #1, LIB_OPENCNT(a6)
        addq.w  #1, UNIT_OPENCNT(a3)

        moveq   #0, d0
        move.b  d0, IO_ERROR(a2)
        move.b  #NT_REPLYMSG, LN_TYPE(a2) ; Mark IORqeust as complete

        ifne DEBUG
        lea     .openmsg(pc), a0
        bsr     SerPutMsg
        move.l  a3, d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        bra.b   .cont
.openmsg:
        dc.b    'Open done res=$', 0
        even
.cont:
        endc ; DEBUG

OpenDone:
        subq.w  #1, LIB_OPENCNT(a6) ; remove temp count
        movem.l (sp)+, d0-d7/a0-a6
        rts
OpenError:
        moveq   #IOERR_OPENFAIL, d0
        move.b  d0, IO_ERROR(a2)
        move.l  d0, IO_DEVICE(a2)
        bra     OpenDone

Close:  ; ( device:a6, iob:a1 )
        movem.l d1-d7/a0-a6, -(sp)

        ifne DEBUG
        lea     .closemsg(pc), a0
        bsr     SerPutMsg
        move.l  a1, d0
        bsr     SerPutNum
        lea     .closemsg2(pc), a0
        bsr     SerPutMsg
        move.l  IO_UNIT(a1), d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        bra.b   .closecont
.closemsg:
        dc.b 'Close unit=$', 0
.closemsg2:
        dc.b ' device=$', 0
        even
.closecont:
        endc ; DEBUG

        move.l  IO_UNIT(a1), a3
        moveq   #0, d0
        move.l  d0, IO_UNIT(a1)
        move.l  d0, IO_DEVICE(a1)

        subq.w  #1, UNIT_OPENCNT(a3)
        bne     CloseDevice

        ; Free unit
        move.l  a6, a5
        move.l  a3, a1
        move.l  #devunit_Sizeof, d0
        move.l  dev_SysLib(a6), a6
        jsr     _LVOFreeMem(a6)
        move.l  a5, a6

CloseDevice:
        moveq   #0, d0
        subq.w  #1, LIB_OPENCNT(a6)
        bne     CloseEnd
        bsr     Expunge
CloseEnd:
        movem.l (sp)+, d1-d7/a0-a6
        ; Important!! Return D0=NULL here to avoid segment list being unloaded
        ; AROS doesn't call CloseLibrary with D0=0! (Unlike AmigaOS)
        rts

Expunge:
        ; TODO: Support this (maybe)
        ; Just leak memory for now
        ifne    DEBUG
        lea     .expmsg(pc), a0
        bsr     SerPutMsg
        bra.s   .cont
.expmsg:
        dc.b 'Expunge!\r\n',0
        even
.cont:
        endc ; DEBUG

        moveq   #0, d0
        rts

Null:
        moveq   #0, d0
        rts

BeginIO: ; ( iob: a1, device:a6 )
        movem.l d0-d7/a0-a6, -(sp)

        move.b  #NT_MESSAGE, LN_TYPE(a1)

        lea     RomCodeEnd(pc), a0
        move.l  a1, (a0)
        move.w  #OP_IOREQ, 4(a0)

        btst    #IOB_QUICK, IO_FLAGS(a1)
        bne     BeginIO_End

        ; Note: "trash" a6
        move.l  dev_SysLib(a6), a6
        ; a1=message
        jsr     _LVOReplyMsg(a6)

BeginIO_End:
        movem.l (sp)+, d0-d7/a0-a6
        rts

AbortIO:
        ; Shouldn't be called, but fake success anyway
        move.b  #0, IO_ERROR(a1)
        rts

; In: a6=SysBase Out: d0=Filesystem resource
CreateFileSysResource:
        move.l  #fsr_Sizeof, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        jsr     _LVOAllocMem(a6)
        tst.l   d0
        beq     cfsrOut
        move.l  d0, -(sp)
        move.l  d0, a0
        move.b  #NT_RESOURCE, LN_TYPE(a0)
        lea     FsrName(pc), a1
        move.l  a1, LN_NAME(a0)
        lea     IdString(pc), a1
        move.l  a1, fsr_Creator(a0)
        ; Init the list
        lea     fsr_FileSysEntries(a0), a0
        move.l  a0, LH_HEAD(a0)
        addq.l  #4, LH_HEAD(a0) ; Point head to tail
        clr.l   LH_TAIL(a0)     ; Clear tail
        move.l  a0, LH_TAILPRED(a0) ; TailPred points to head
        ; Add it to the system
        lea     ResourceList(a6), a0
        move.l  d0, a1
        jsr     _LVOAddTail(a6)
        move.l  (sp)+, d0
cfsrOut:
        rts

        ; Structure shared with emulator
        ; Keep in sync
        rsreset
handler_SysBase rs.l 1
handler_DosBase rs.l 1
handler_MsgPort rs.l 1
handler_DosList rs.l 1
handler_Id      rs.l 1
handler_Sizeof  rs.b 0

        cnop    0,4 ; Must be long word aligned
        dc.l    16 ; fake length
FakeSegList:
        dc.l    0  ; next segment

        sub.l   #handler_Sizeof, sp
        move.l  sp, a4

        move.l  $4.w, a6
        move.l  a6, handler_SysBase(a4)
        move.l  ThisTask(a6), a1
        lea.l   pr_MsgPort(a1), a3
        move.l  a3, handler_MsgPort(a4)

        ifne DEBUG
        movem.l d0/a0/a1, -(sp)
        lea     .text_Starting(pc), a0
        bsr     SerPutMsg
        move.l  ThisTask(a6), a1
        move.l  LN_NAME(a1), a0
        bsr     SerPutMsg
        bsr     SerPutSpace
        move.l  a0, d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        movem.l (sp)+, d0/a0/a1
        endc

        ; Get startup packet
        move.l  a3, a0
        jsr     _LVOWaitPort(a6)
        move.l  a3, a0
        jsr     _LVOGetMsg(a6)
        move.l  d0, a2
        move.l  LN_NAME(a2), a2 ; a2 = DosPacket*

        lea     DosLibName(pc), a1
        jsr     _LVOOldOpenLibrary(a6)
        move.l  d0, handler_DosBase(a4)
        beq     .err

        ; KS1.3 does not initialize arg1/arg3
        tst.l   dp_Arg3(a2)
        bne.b   .nofix
        bsr     FixStartupMessage

        ; arg1 = BSTR to mount name
        ; arg2 = BPTR to startup packet
        ; arg3 = BPTR to device node

.nofix:

        ifne DEBUG
        movem.l a0/d0, -(sp)
        lea     .text_Startup1(pc), a0
        bsr     SerPutMsg
        move.l  dp_Arg1(a2), d0
        bsr     SerPutBstr
        lea     .text_Startup2(pc), a0
        bsr     SerPutMsg
        move.l  dp_Arg2(a2), d0
        add.l   d0, d0
        add.l   d0, d0
        bsr     SerPutNum
        lea     .text_Startup3(pc), a0
        bsr     SerPutMsg
        move.l  dp_Arg3(a2), d0
        add.l   d0, d0
        add.l   d0, d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        movem.l (sp)+, a0/d0
        endc

        move.l  dp_Arg2(a2), d0
        add.l   d0, d0
        add.l   d0, d0
        move.l  d0, a0
        move.l  fssm_Unit(a0), handler_Id(a4)
        move.l  dp_Arg3(a2), d0
        add.l   d0, d0
        add.l   d0, d0
        move.l  d0, handler_DosList(a4)
        move.l  d0, a0
        move.l  a3, dol_Task(a0) ; Set task in device node

        ; Reply to startup packet
        move.l  #DOSTRUE, dp_Res1(a2)
        move.l  #0, dp_Res2(a2)
        bsr     .dosreply

        ; Add volume
        move.l  #dol_Sizeof, d0
        move.l  #MEMF_PUBLIC+MEMF_CLEAR, d1
        jsr     _LVOAllocMem(a6)
        tst.l   d0
        beq     .err
        move.l  d0, a2

        move.l  handler_DosBase(a4), a6

        move.l  handler_MsgPort(a4), dol_Task(a2)
        move.l  handler_DosList(a4), a0
        move.l  dol_Name(a0), dol_Name(a2)
        move.l  #DLT_VOLUME, dol_Type(a2)
        move.l  #ID_DOS_DISK, dol_DiskType(a2)
        lea     dol_VolumeDate(a2), a0
        move.l  a0, d1
        jsr     _LVODateStamp(a6)
        bsr     AddDosEntry
        tst.l   d0
        beq     .err

        ; Tell host we're ready
        lea     RomCodeEnd(pc), a5 ; a5 = comm area
        move.l  a4, (a5)
        move.w  #OP_VOLUME_INIT, 4(a5)

        move.l  handler_SysBase(a4), a6
.waitmsg:
        ; In this loop:
        ; a4 = handler data, a5 = comm area, a6 = ExecBase

        move.l  handler_MsgPort(a4), a0
        jsr     _LVOWaitPort(a6)

.msgloop:
        move.l  handler_MsgPort(a4), a0
        jsr     _LVOGetMsg(a6)
        tst.l   d0
        beq     .waitmsg
        move.l  d0, a2
        move.l  LN_NAME(a2), a2 ; a2 = DosPacket*

        ifne DEBUG
        lea     .text_GotMsg(pc), a0
        bsr     SerPutMsg
        move.l  a2, d0
        bsr     SerPutNum
        move.b  #$20, d0
        bsr     SerPutChar
        move.l  dp_Type(a2), d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        endc ; DEBUG

        ; Put the handler ID in Res1 for the host
        move.l  handler_Id(a4), dp_Res1(a2)

        move.l  a2, (a5)
        move.w  #OP_VOLUME_PACKET, 4(a5)

        bsr     .dosreply

        bra     .msgloop
.err:
        ; TODO: Cleanup
        ifne DEBUG
        lea     .text_Failed(pc), a0
        bsr     SerPutMsg
        endc
.out:
        add.l   #handler_Sizeof, a7
        rts

; Reply to dos packet in a2
; transhes a0-a1
.dosreply:
        move.l  dp_Port(a2), a0 ; grab reply port for PutMsg
        move.l  handler_MsgPort(a4), dp_Port(a2) ; our reply port
        move.l  dp_Link(a2), a1 ; message
        jmp     _LVOPutMsg(a6)

        ifne DEBUG
.text_Starting: dc.b 'FS handler starting: ',0
.text_Startup1: dc.b 'Startup msg: "', 0
.text_Startup2: dc.b '" dn_Startup=$', 0
.text_Startup3: dc.b ' deviceNode=$', 0
.text_GotMsg: dc.b 'Got message ', 0
.text_Failed: dc.b 'Failed to add device', 13, 10, 0
        even
        endc ; DEBUG

; input: a2=entry, a6=dosbase
; output: D0=result
AddDosEntry:
        ; KS1.x?
        cmp.w   #36, LIB_VERSION(a6)
        bcc.b   .waitlock

        move.l  a6, -(sp)
        move.l  handler_SysBase(a4), a6
        jsr     _LVOForbid(a6)
        bsr     GetDosList
        move.l  (a0), dol_Next(a2)
        move.l  a2, d0
        lsr.l   #2, d0 ; to bptr
        move.l  d0, (a0)
        jsr     _LVOPermit(a6)
        move.l  (sp)+, a6
        moveq   #1, d0 ; success
        rts
.waitlock:
        moveq   #LDF_WRITE+LDF_ALL, d1
        jsr     _LVOAttemptLockDosList(a6)
        subq.l  #2, d0 ; AttemptLockDosList can return 0 or 1 for failure (!)
        bmi.s   .waitlock

        move.l  a2, d1
        jsr     _LVOAddDosEntry(a6)
        move.l  d0, -(sp)

        moveq   #LDF_WRITE+LDF_ALL, d1
        jsr     _LVOUnLockDosList(a6)
        move.l   (sp)+, d0
        rts

; input a2=message, a6=sysbase
FixStartupMessage:
        movem.l a0-a1/d0-d1, -(sp)
        jsr     _LVOForbid(a6)
        bsr     GetDosList
        move.l  dp_Arg2(a2), d1
.loop:
        move.l  dol_Next(a0), d0 ; d0 = BPTR to dos list entry
        beq.b   .done
        move.l  d0, a0
        add.l   a0, a0
        add.l   a0, a0 ; From BPTR
        tst.l   dol_Type(a0) ; == DLT_DEVICE?
        bne.b   .loop

        ifne DEBUG
        movem.l d0/a0, -(sp)
        move.l  dol_Name(a0), d0
        bsr     SerPutBstr
        bsr     SerPutSpace
        move.l  dol_Startup(a0), d0
        bsr     SerPutNum
        bsr     SerPutCrLf
        movem.l (sp)+, d0/a0
        endc ; DEBUG

        cmp.l   dol_Startup(a0), d1
        bne.b   .loop
        move.l  dol_Name(a0), dp_Arg1(a2)
        move.l  d0, dp_Arg3(a2)
.done:
        jsr     _LVOPermit(a6)
        movem.l (sp)+, a0-a1/d0-d1
        rts

; Return dos list in a0 (Must be in forbid state)
GetDosList:
        move.l  handler_DosBase(a4), a0
        move.l  dl_Root(a0), a0
        move.l  rn_Info(a0), a0
        add.l   a0, a0
        add.l   a0, a0  ; From BPTR
        lea.l   di_DevInfo(a0), a0
        rts

RomCodeEnd:

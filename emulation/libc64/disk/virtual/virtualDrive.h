
/**
 * used vdrive code from VICE, stripped all not needed for loading first file on Disk.
 * this high level emulation is used only for speed reasons and is disabled after first file is loaded
 */

/*
 * vdrive.c - Virtual disk-drive implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  pottendo <pottendo@gmx.net>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#pragma once

#include "../../traps/baseDevice.h"

#define CBMDOS_SLOT_NAME_LENGTH 16

namespace LIBC64 {

struct DiskStructure;
struct System;

struct VirtualDrive : BaseDevice {
    VirtualDrive(System* system, DiskStructure* structure);

    System* system;

    struct vdrive_dir_context_t {
        uint8_t buffer[256];      /* Current directory sector. */
        int find_length;       /* -1 allowed.  */
        uint8_t find_nslot[CBMDOS_SLOT_NAME_LENGTH];
        unsigned int find_type;
        unsigned int slot;
        unsigned int track;
        unsigned int sector;
        unsigned int time_low;
        unsigned int time_high;
     //   struct vdrive_s *vdrive;
    };

    struct bufferinfo_t {
        unsigned int mode;     /* Mode on this buffer */
        unsigned int readmode; /* Is this channel for reading or writing */
        uint8_t *buffer;          /* Use this to save data */
        uint8_t *slot;            /* Save data for directory-slot */
        unsigned int bufptr;   /* Use this to save/read data to disk */
        unsigned int track;    /* which track is allocated for this sector */
        unsigned int sector;   /*   (for write files only) */
        unsigned int length;   /* Directory-read length */
        unsigned int record;   /* Current record */
        unsigned int partition;/* Current partition */
        unsigned int partstart;  /* for 1581's: partition (and BAM) can be different for open files */
        unsigned int partend;
        int small;               /* flag to enable small buffer reads on seq type files */
        int timemode;            /* flag to enable time output on dir listings */

        vdrive_dir_context_t dir; /* directory listing context or directory entry */

        /* REL file information stored in buffers since we can have more than
            one open */
        uint8_t *side_sector;
        /* location of the side sectors */
        uint8_t *side_sector_track;
        uint8_t *side_sector_sector;

        uint8_t *super_side_sector;
        /* location of the super side sector */
        uint8_t super_side_sector_track;
        uint8_t super_side_sector_sector;

        uint8_t *buffer_next;          /* next buffer for rel file */
        unsigned int track_next;    /* track for the next sector */
        unsigned int sector_next;   /* sector for the next sector */

        unsigned int record_max;  /* Max rel file record, inclusive */
        unsigned int record_next; /* Buffer pointer to beginning of next record */
        uint8_t *side_sector_needsupdate;
        uint8_t needsupdate;         /* true if the current sector needs to be
                                  written (from REL write) */
        uint8_t super_side_sector_needsupdate; /* similar to above */

    };

    struct {
        unsigned int unit;         /* IEC bus device number */

        struct disk_image_s *images[2]; /* for dual drives */

        /* Current image file */
        struct disk_image_s *image;
        int image_mode;            /* -1 no disk, 0 is write/read, 1 is read */
        unsigned int image_format; /* 1541/71/81 */

        unsigned int Bam_Track;
        unsigned int Bam_Sector;
        unsigned int bam_name;     /* Offset from start of BAM to disk name.   */
        unsigned int bam_id;       /* Offset from start of BAM to disk ID.  */
     //   int bam_state[VDRIVE_BAM_MAX_STATES];
       // int bam_tracks[VDRIVE_BAM_MAX_STATES];
        //int bam_sectors[VDRIVE_BAM_MAX_STATES];
        /* bam cache dirty (1 char per sector)  */
        /* -1 = not in mem, 0 = in mem, 1 = dirty */
        unsigned int Header_Track = 18; /* Directory header location */
        unsigned int Header_Sector = 0;
        unsigned int Dir_Track = 18;    /* First directory sector location */
        unsigned int Dir_Sector = 1;
        unsigned int num_tracks;
        /* CBM partition first and last track (1581)
         * Part_Start is 1, Part_End = num_tracks if no partition is used
         */
        unsigned int Part_Start, Part_End;
        /* CMD paritions */
        unsigned int current_offset;
        unsigned int sys_offset;
        signed int current_part; /* current part; one matching the bam loaded */
        signed int selected_part; /* the "currently selected" partition, with CP command */
        signed int default_part; /* the default one that is to be used on startup */
        uint8_t ptype[256];
        uint32_t poffset[256];
        uint32_t psize[256];
        unsigned int cheadertrack[256];    /* current header track and sector for each partition */
        unsigned int cheadersector[256];
        unsigned int cdirtrack[256];    /* current directory track and sector for each partition */
        unsigned int cdirsector[256];
        unsigned int cpartstart[256];   /* current partition start end and for each partition for 1581 */
        unsigned int cpartend[256];

        unsigned int d90toggle;    /* D9090/60 new sector toggle */
        int haspt;                 /* whether the connected disk supports partitions (not drives) */
        int dir_part;              /* which drive to show first when doing group dirs */
        int dir_count;             /* how many drives are left when doing group dirs */
        int last_code;             /* for command channel status string */

        unsigned int bam_size;
        uint8_t *bam;              /* Disk header blk (if any) followed by BAM blocks */
        bufferinfo_t buffers[16];

        /* Memory read command buffer.  */
        uint8_t mem_buf[256];
        unsigned int mem_length;

        /* removed side sector data and placed it in buffer structure */
        /* BYTE *side_sector; */

        uint8_t ram[0x8000];
    } vdrive;

    struct disk_addr_t {
        unsigned int track;
        unsigned int sector;
    };

    struct cbmdos_cmd_parse_plus_t {
        const uint8_t *full; /* input: full dos-command string */
        unsigned int fulllength; /* input: length of string */
        unsigned int secondary; /* input */
        int mode; /* input: decode type, 0=file, 1=command */

        int drive; /* output: drive number */
        uint8_t *command; /* output: parsed command */
        unsigned int commandlength; /* command length */
        uint8_t *abbrv; /* output: parsed abbreviated command */
        unsigned int abbrvlength; /* abbreviated command length */
        uint8_t *path; /* output: parsed path */
        unsigned int pathlength; /* path length */
        uint8_t *file; /* output: parsed file */
        unsigned int filelength; /* file length */
        uint8_t *more; /* output: command to further process */
        unsigned int morelength; /* length of command to further process */

        /* for file mode */
        unsigned int readmode; /* output */
        unsigned int filetype; /* output */
        unsigned int recordlength; /* output */
    };

    DiskStructure* structure;

    unsigned int last_read_track, last_read_sector;
    uint8_t last_read_buffer[256];
    bool x81 = false;

    using Callback = std::function<void ()>;
    Callback finishEvent;

    auto vdrive_command_set_error(int code, unsigned int track, unsigned int sector) -> int;
    auto cbmdos_command_parse_plus(cbmdos_cmd_parse_plus_t *cmd_parse) -> unsigned int;
    auto iec_open_read_directory(unsigned int secondary, cbmdos_cmd_parse_plus_t *cmd_parse) -> int;
    auto iec_open_read_sequential(unsigned int secondary, unsigned int track, unsigned int sector) -> int;
    auto vdrive_alloc_buffer(bufferinfo_t *p, int mode) -> void;
    auto vdrive_read_sector(uint8_t *buf, unsigned int track, unsigned int sector) -> int;
    auto vdrive_set_last_read(unsigned int track, unsigned int sector, uint8_t *buffer) -> void;
    auto iec_read_sequential(uint8_t *data, unsigned int secondary) -> int;
    auto vdrive_command_switchtraverse(cbmdos_cmd_parse_plus_t *cmd) -> int;
    auto vdrive_dir_find_first_slot(const uint8_t *name, int length, unsigned int type, vdrive_dir_context_t *dir) -> void;
    auto cbmdos_dir_slot_create(const char *name, unsigned int len) -> uint8_t*;
    auto vdrive_dir_find_next_slot(vdrive_dir_context_t *dir) -> uint8_t*;
    auto iec_open_read(unsigned int secondary) -> int;

    auto vdrive_dir_name_match(uint8_t *slot, uint8_t *nslot, int length, int type) -> unsigned int;
    auto cbmdos_parse_wildcard_compare(const uint8_t *name1, const uint8_t *name2) -> unsigned int;

    auto vdrive_command_execute(const uint8_t *buf, unsigned int length) -> int;
    auto vdrive_command_memory(uint8_t *buffer, unsigned int length) -> int;
    auto vdrive_command_memory_read(const uint8_t *buf, uint16_t addr, unsigned int length) -> int;
    auto vdrive_command_memory_exec(const uint8_t *buf, uint16_t addr, unsigned int length) -> int;

    auto flush(unsigned int secondary) -> void;
    auto get(uint8_t* data, unsigned int secondary) -> int;
    auto put(uint8_t data, unsigned int secondary) -> int;
    auto listen(unsigned int secondary) -> void;
    auto open(const uint8_t* name, unsigned int length, unsigned int secondary) -> int;
    auto close(unsigned int secondary) -> int;

    auto finish(bool sendFinishEvent) -> void;
    auto reset() -> void;
};

}

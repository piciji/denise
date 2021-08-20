
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

#include "../structure/structure.h"
#include "../drive/drive1541.h"
#include "virtualDrive.h"
#include "../../system/system.h"
#include <cstring>

#define SERIAL_OK               0
#define SERIAL_WRITE_TIMEOUT    1
#define SERIAL_READ_TIMEOUT     2
#define SERIAL_FILE_NOT_FOUND   64      /* EOF */
#define SERIAL_NO_DEVICE        128

#define SERIAL_LISTEN 0x20
#define SERIAL_TALK 0x40

#define ISOPEN_CLOSED           0
#define ISOPEN_AWAITING_NAME    1
#define ISOPEN_OPEN             2

#define BUFFER_COMMAND_CHANNEL 5

#define CBMDOS_IPE_OK                      0
#define CBMDOS_IPE_DELETED                 1
#define CBMDOS_IPE_SEL_PARTN               2   /* 1581 */
#define CBMDOS_IPE_UNIMPL                  3
#define CBMDOS_IPE_MEMORY_READ             4

#define CBMDOS_IPE_READ_ERROR_BNF          20
#define CBMDOS_IPE_READ_ERROR_SYNC         21
#define CBMDOS_IPE_READ_ERROR_DATA         22
#define CBMDOS_IPE_READ_ERROR_CHK          23
#define CBMDOS_IPE_READ_ERROR_GCR          24
#define CBMDOS_IPE_WRITE_ERROR_VER         25
#define CBMDOS_IPE_WRITE_PROTECT_ON        26
#define CBMDOS_IPE_READ_ERROR_BCHK         27
#define CBMDOS_IPE_WRITE_ERROR_BIG         28
#define CBMDOS_IPE_DISK_ID_MISMATCH        29
#define CBMDOS_IPE_SYNTAX                  30
#define CBMDOS_IPE_INVAL                   31
#define CBMDOS_IPE_LONG_LINE               32
#define CBMDOS_IPE_BAD_NAME                33
#define CBMDOS_IPE_NO_NAME                 34
#define CBMDOS_IPE_PATH_NOT_FOUND          39

#define CBMDOS_IPE_NO_RECORD               50
#define CBMDOS_IPE_OVERFLOW                51
#define CBMDOS_IPE_TOOLARGE                52  /* 1581 */

#define CBMDOS_IPE_NOT_WRITE               60
#define CBMDOS_IPE_NOT_OPEN                61
#define CBMDOS_IPE_NOT_FOUND               62
#define CBMDOS_IPE_FILE_EXISTS             63
#define CBMDOS_IPE_BAD_TYPE                64
#define CBMDOS_IPE_NO_BLOCK                65
#define CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR 66
#define CBMDOS_IPE_ILLEGAL_SYSTEM_T_OR_S   67  /* 1581 */

#define CBMDOS_IPE_NO_CHANNEL              70
#define CBMDOS_IPE_DIRECTORY_ERROR         71
#define CBMDOS_IPE_DISK_FULL               72
#define CBMDOS_IPE_DOS_VERSION             73
#define CBMDOS_IPE_NOT_READY               74
#define CBMDOS_IPE_FORMAT_ERROR            75
#define CBMDOS_IPE_BAD_PARTN               77  /* 1581 */

#define CBMDOS_FAM_READ   0
#define CBMDOS_FAM_WRITE  1
#define CBMDOS_FAM_APPEND 2
#define CBMDOS_FAM_EOF    4

#define BUFFER_NOT_IN_USE      0
#define BUFFER_DIRECTORY_READ  1
#define BUFFER_SEQUENTIAL      2
#define BUFFER_MEMORY_BUFFER   3
#define BUFFER_RELATIVE        4
#define BUFFER_COMMAND_CHANNEL 5
#define BUFFER_PARTITION_READ  6
#define BUFFER_DIRECTORY_MORE_READ  7

/* CBM DOS File Types */
#define CBMDOS_FT_DEL         0       /* should match FILEIO_TYPE_xxx */
#define CBMDOS_FT_SEQ         1
#define CBMDOS_FT_PRG         2
#define CBMDOS_FT_USR         3
#define CBMDOS_FT_REL         4
#define CBMDOS_FT_CBM         5       /* 1581 partition */
#define CBMDOS_FT_DIR         6
#define CBMDOS_FT_REPLACEMENT 0x20
#define CBMDOS_FT_LOCKED      0x40
#define CBMDOS_FT_CLOSED      0x80

#define SERIAL_ERROR                    (2)
#define SERIAL_EOF                      (0x40)
#define SERIAL_DEVICE_NOT_PRESENT       (0x80)

#define CBMDOS_IPE_NOT_WRITE               60
#define CBMDOS_IPE_NOT_OPEN                61
#define CBMDOS_IPE_NOT_FOUND               62
#define CBMDOS_IPE_FILE_EXISTS             63
#define CBMDOS_IPE_BAD_TYPE                64
#define CBMDOS_IPE_NO_BLOCK                65
#define CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR 66

#define SLOT_TYPE_OFFSET      2
#define SLOT_FIRST_TRACK      3
#define SLOT_FIRST_SECTOR     4
#define SLOT_NAME_OFFSET      5
#define SLOT_SIDE_TRACK       21
#define SLOT_SIDE_SECTOR      22
#define SLOT_RECORD_LENGTH    23 /* for relative files */
#define SLOT_REPLACE_TRACK    28
#define SLOT_REPLACE_SECTOR   29
#define SLOT_NR_BLOCKS        30

#define SLOT_GEOS_ITRACK      21
#define SLOT_GEOS_ISECTOR     22
#define SLOT_GEOS_STRUCT      23
#define SLOT_GEOS_TYPE        24
#define SLOT_GEOS_YEAR        25 /* same on CMD (year to minute) */
#define SLOT_GEOS_MONTH       26
#define SLOT_GEOS_DATE        27
#define SLOT_GEOS_HOUR        28
#define SLOT_GEOS_MINUTE      29

#define SLOT_SIZE             32

namespace LIBC64 {

VirtualDrive::VirtualDrive(Structure1541* structure) : structure(structure) {
    for (unsigned i = 0; i < 16; i++) {
        vdrive.buffers[i].mode = BUFFER_NOT_IN_USE;
        vdrive.buffers[i].buffer = nullptr;
    }
}

auto VirtualDrive::reset() -> void {
    for (unsigned i = 0; i < 15; i++) {
        vdrive.buffers[i].mode = BUFFER_NOT_IN_USE;
        if (vdrive.buffers[i].buffer) {
            std::free(vdrive.buffers[i].buffer);
            vdrive.buffers[i].buffer = nullptr;
        }
    }

    /* init command channel */
    vdrive_alloc_buffer(&(vdrive.buffers[15]), BUFFER_COMMAND_CHANNEL);
    vdrive_command_set_error(CBMDOS_IPE_DOS_VERSION, 0, 0);

    vdrive.dir_part = 0;
    vdrive.last_code = CBMDOS_IPE_OK;
}

auto VirtualDrive::open(const uint8_t* name, unsigned int length, unsigned int secondary) -> int {
    bufferinfo_t* p = &(vdrive.buffers[secondary]);
    uint8_t *slot; /* Current directory entry */
    int rc, status = SERIAL_OK;
    unsigned int opentype;
    cbmdos_cmd_parse_plus_t cmd_parse_stat;
    cbmdos_cmd_parse_plus_t *cmd_parse = &cmd_parse_stat;


    if ( (!name || !*name) && p->mode != BUFFER_COMMAND_CHANNEL) {
        return SERIAL_NO_DEVICE;
    }

    p->small = 0;

    /*
     * If channel is command channel, name will be used as write. Return only
     * status of last write ...
     */
    if (p->mode == BUFFER_COMMAND_CHANNEL) {
        unsigned int n;

        /* make static analysis happy */
        if ( !name || !*name ) {
            length = 0;
        }

        /* partition will be handled inside command code */
        for (n = 0; n < length; n++) {
            status = put(name[n], secondary);
        }

        if (length) {
            p->readmode = CBMDOS_FAM_WRITE;
        } else {
            p->readmode = CBMDOS_FAM_READ;
        }
        return status;
    }

    /*
     * Clear error flag
     */
    vdrive_command_set_error(CBMDOS_IPE_OK, 0, 0);

    /*
     * In use ?
     */
    if (p->mode != BUFFER_NOT_IN_USE) {
        vdrive_command_set_error(CBMDOS_IPE_NO_CHANNEL, 0, 0);
        return SERIAL_ERROR;
    }

    /* mode 0 without setting drive */
    cmd_parse->mode = 2;
    cmd_parse->drive = -1;


    cmd_parse->full = (uint8_t *)name;
    cmd_parse->fulllength = length;
    cmd_parse->secondary = secondary;

    rc = cbmdos_command_parse_plus(cmd_parse);

    if (rc != CBMDOS_IPE_OK) {
        status = SERIAL_ERROR;
        goto out;
    }

    /* remember whether the partition was specified or not (for dual dir) */
    vdrive.dir_count = 1;
    if (cmd_parse->command && cmd_parse->command[0] == '$') {
        if (cmd_parse->drive < 0) {

            /* setup multi drive list */
            cmd_parse->drive = vdrive.dir_part;
            vdrive.dir_count = 2;

        } else {
            /* otherwise remember part for next multi list */
            vdrive.dir_part = cmd_parse->drive;
        }
    } else {
        if (cmd_parse->drive < 0) {
            cmd_parse->drive = 0;
        }
    }

    /* make sure they can't use the system partition for anything */
    if (cmd_parse->drive == 255) {
        status = SERIAL_ERROR;
        goto out;
    }

    /* check if part is allocated to something */
//    if (!vdrive_ispartvalid(vdrive, cmd_parse->drive)) {
//        vdrive_command_set_error(CBMDOS_IPE_NOT_READY, vdrive->Header_Track, vdrive->Header_Sector);
//        status = SERIAL_ERROR;
//        goto out;
//    }
  //  p->partition = vdrive_realpart(vdrive, cmd_parse->drive);

    /*
     * Internal buffer ?
     */
    if (cmd_parse->command && cmd_parse->command[0] == '#') {
/* FIXME: "file" has requested buffer */
        vdrive_alloc_buffer(p, BUFFER_MEMORY_BUFFER);

        /* the pointer is actually 1 on the real drives. */
        /* this probably relates to the B-R and B-W commands. */
        /* 1541 firmware: $cb84 - open channel, $cc0f bp = 1 */
        p->bufptr = 1;
        /* we need a length to support the original B-R and B-W
           commands. */
        p->length = 256;
        status = SERIAL_OK;
        goto out;
    }

    /* Clear update flag */
    p->needsupdate = 0;

    /* switch partition and sub directory */
    status = vdrive_command_switchtraverse(cmd_parse);
    if (status) {
        status = SERIAL_ERROR;
        goto out;
    }

    /* for 1581's */
//    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) {
//        p->partstart = vdrive->Part_Start;
//        p->partend = vdrive->Part_End;
//    }

    /*
     * Directory read
     * A little-known feature of the 1541: open 1,8,2,"$" (SA 1 or >)
     * It gives you the BAM+DIR as a sequential file, containing the data
     * just as it appears on disk.  -Olaf Seibert
     * SA of 1 on CMD-ROMs appears to return a constant stream of $47
     */

    if (cmd_parse->command && cmd_parse->command[0] == '$') {
        system->interface->log("todo open read dir");
        p->readmode = CBMDOS_FAM_READ;
        //status = iec_open_read_directory(vdrive, secondary, cmd_parse);
        goto out;
    }

    /* Limit file name to 16 chars.  */
    if (cmd_parse->filelength > 16) {
        cmd_parse->filelength = 16;
    }

    /*
     * Check that there is room on directory.
     */
    if (cmd_parse->readmode == CBMDOS_FAM_READ
        || cmd_parse->readmode == CBMDOS_FAM_APPEND) {
        opentype = cmd_parse->filetype;
    } else {
        opentype = CBMDOS_FT_DEL;
    }

    vdrive_dir_find_first_slot(cmd_parse->file, cmd_parse->filelength, opentype, &p->dir);

    /*
     * Find the first non-DEL entry in the directory (if it exists).
     */
    do {
        slot = vdrive_dir_find_next_slot(&p->dir);
    } while (slot && ((slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_DEL));

    p->readmode = cmd_parse->readmode;
    p->slot = slot;

    /* Call REL function if we are creating OR opening one */
//    if (cmd_parse->filetype == CBMDOS_FT_REL ||
//        ( slot && (slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_REL)) {
//        /* Make sure the record length of the opening command is the same as
//           the record length in the directory slot, if not DOS ERROR 50 */
//        if (slot && cmd_parse->recordlength > 0 &&
//            slot[SLOT_RECORD_LENGTH] != cmd_parse->recordlength) {
//            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_RECORD, 0, 0);
//            status = SERIAL_ERROR;
//            goto out;
//        }
//        /* At this point the record lengths are the same (or will be), so set
//            them equal. */
//        if (slot) {
//            cmd_parse->recordlength = slot[SLOT_RECORD_LENGTH];
//        }
//        status = vdrive_rel_open(vdrive, secondary, cmd_parse);
//        goto out;
//    }

    if (cmd_parse->readmode == CBMDOS_FAM_READ) {
        status = iec_open_read(secondary);
    } else {
        //status = iec_open_write(vdrive, secondary, cmd_parse);
    }

    out:

    if (cmd_parse_stat.abbrv) {
        std::free(cmd_parse_stat.abbrv);
    }
    if (cmd_parse_stat.path) {
        std::free(cmd_parse_stat.path);
    }
    if (cmd_parse_stat.file) {
        std::free(cmd_parse_stat.file);
    }
    if (cmd_parse_stat.more) {
        std::free(cmd_parse_stat.more);
    }
    if (cmd_parse_stat.command) {
        std::free(cmd_parse_stat.command);
    }

    return status;
}

auto VirtualDrive::close(unsigned int secondary) -> int {
    bufferinfo_t* p = &(vdrive.buffers[secondary]);
    int status = SERIAL_OK;

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            return SERIAL_OK; /* FIXME: Is this correct? */

        case BUFFER_MEMORY_BUFFER:
        case BUFFER_DIRECTORY_READ:
        case BUFFER_PARTITION_READ:
        case BUFFER_DIRECTORY_MORE_READ:
            p->mode = BUFFER_NOT_IN_USE;
            p->slot = NULL;
            break;
        case BUFFER_SEQUENTIAL:
            if (p->readmode & (CBMDOS_FAM_WRITE | CBMDOS_FAM_APPEND)) {
                system->interface->log("todo iec close sequentiel");
            }
            p->mode = BUFFER_NOT_IN_USE;
            status = SERIAL_OK;
            break;
        case BUFFER_RELATIVE:
            system->interface->log("todo rel close");
            status = SERIAL_OK;
            break;
        case BUFFER_COMMAND_CHANNEL:
            vdrive_command_set_error(CBMDOS_IPE_OK, 0, 0);
            break;
        default:
            system->interface->log("Fatal: unknown floppy-close-mode");
    }

    return status;
}

auto VirtualDrive::get(uint8_t *data, unsigned int secondary) -> int {
    bufferinfo_t* p = &(vdrive.buffers[secondary]);
    int status = SERIAL_OK;

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            /* real drives just return $42 */
            return SERIAL_ERROR | SERIAL_EOF;

        case BUFFER_MEMORY_BUFFER:
            *data = p->buffer[p->bufptr];
            p->bufptr++;
            if (p->bufptr >= p->length) {
                /* Buffer pointer resets to 1, not 0. */
                p->bufptr = 1;
                status = SERIAL_EOF;
            }
            break;

        case BUFFER_DIRECTORY_READ:
        case BUFFER_PARTITION_READ:
        case BUFFER_DIRECTORY_MORE_READ:
        case BUFFER_SEQUENTIAL:
            status = iec_read_sequential(data, secondary);
            break;

        case BUFFER_COMMAND_CHANNEL:
            if (p->bufptr > p->length) {
                vdrive_command_set_error(CBMDOS_IPE_OK, 0, 0);
            }
            *data = p->buffer[p->bufptr];
            p->bufptr++;
            if (p->bufptr > p->length) {
                status = SERIAL_EOF;
            }
            break;

        case BUFFER_RELATIVE:
            system->interface->log("todo read rel mode");
            break;

        default:
            system->interface->log("Fatal: unknown buffermode on floppy-read");
    }

    return status;
}

auto VirtualDrive::put( uint8_t data, unsigned int secondary) -> int {
    bufferinfo_t* p = &(vdrive.buffers[secondary]);

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            /* real drives return 128 and don't set command error */
            return SERIAL_DEVICE_NOT_PRESENT;
        case BUFFER_DIRECTORY_READ:
        case BUFFER_PARTITION_READ:
        case BUFFER_DIRECTORY_MORE_READ:
            vdrive_command_set_error(CBMDOS_IPE_NOT_WRITE, 0, 0);
            return SERIAL_ERROR;
        case BUFFER_MEMORY_BUFFER:
            p->buffer[p->bufptr] = data;
            p->bufptr++;
            if (p->bufptr >= p->length) {
                /* On writes, buffer pointer resets to 0. */
                p->bufptr = 0;
            }
            return SERIAL_OK;
        case BUFFER_SEQUENTIAL:
            if (p->readmode == CBMDOS_FAM_READ) {
                return SERIAL_ERROR;
            }
            system->interface->log("todo buffer seq");
            break;
        case BUFFER_COMMAND_CHANNEL:
            if (p->readmode == CBMDOS_FAM_READ) {
                p->bufptr = 0;
                p->readmode = CBMDOS_FAM_WRITE;
            }
            if (p->bufptr >= 256) { /* Limits checked later */
                return SERIAL_ERROR;
            }
            p->buffer[p->bufptr] = data;
            p->bufptr++;
            break;
        case BUFFER_RELATIVE:
            system->interface->log("todo rel mode");
        default:
            system->interface->log("fatal unknown write mode");
            break;
    }
    return SERIAL_OK;
}

auto VirtualDrive::listen(unsigned int secondary) -> void {
    bufferinfo_t* p = &(vdrive.buffers[secondary]);

    if (p->mode == BUFFER_RELATIVE) {
        system->interface->log("todo listen rel mode");
    }
}

auto VirtualDrive::flush(unsigned int secondary) -> void {

    bufferinfo_t* p = &(vdrive.buffers[secondary]);

    if (p->mode != BUFFER_COMMAND_CHANNEL) {
        return;
    }

    if (p->readmode == CBMDOS_FAM_READ) {
        return;
    }

    if (p->length) {
        system->interface->log("todo vdrive command execute");
        p->bufptr = 0;
    }
}

// helper
auto VirtualDrive::vdrive_command_set_error(int code, unsigned int track, unsigned int sector) -> int {
    bufferinfo_t* p = &(vdrive.buffers[15]);

    vdrive.last_code = code;

    /* Length points to the last byte, and doesn't give the length.  */
    p->length = (unsigned int)strlen((char *)p->buffer) - 1;


    p->bufptr = 0;
    p->readmode = CBMDOS_FAM_READ;

    return code;
}

auto VirtualDrive::vdrive_alloc_buffer(bufferinfo_t *p, int mode) -> void {
    size_t size = 256;

    if (p->buffer == nullptr) {
        /* first time actually allocate memory, and clear it */
        p->buffer = new uint8_t[size];
        std::memset(p->buffer, 0, size);
    } else {
        /* any other time, just adjust the size of the buffer */
        p->buffer = (uint8_t*)std::realloc(p->buffer, size);
    }
    p->mode = mode;
}

auto VirtualDrive::iec_open_read_sequential(unsigned int secondary, unsigned int track, unsigned int sector) -> int {
    int status;
    bufferinfo_t* p = &(vdrive.buffers[secondary]);

    /* we should already be in the proper partition at this point */
    vdrive_alloc_buffer(p, BUFFER_SEQUENTIAL);
    p->bufptr = 2;
    p->record = 1;

    status = vdrive_read_sector(p->buffer, track, sector);
    p->length = p->buffer[0] ? 0 : p->buffer[1];

    vdrive_set_last_read(track, sector, p->buffer);

    if (status != 0) {
        close(secondary);
        return SERIAL_ERROR;
    }
    return SERIAL_OK;
}

auto VirtualDrive::vdrive_set_last_read(unsigned int track, unsigned int sector, uint8_t *buffer) -> void {
    last_read_track = track;
    last_read_sector = sector;
    memcpy(last_read_buffer, buffer, 256);
}

auto VirtualDrive::finish() -> void {
    uint8_t bam[256];
    uint8_t id[2];

    if (structure->readSector( &bam[0], 18, 0 )) {
        std::memcpy(id, bam + 162, 2);
        structure->drive->ram[0x12] = id[0];
        structure->drive->ram[0x13] = id[1];
        structure->drive->ram[0x16] = id[0];
        structure->drive->ram[0x17] = id[1];
        structure->drive->ram[0x18] = last_read_track;
        structure->drive->ram[0x19] = last_read_sector;
        structure->drive->ram[0x22] = last_read_track;

        std::memcpy(&(structure->drive->ram[0x400]), last_read_buffer, 256);

        structure->drive->currentHalftrack = last_read_track * 2 - 2;
        structure->drive->changeHalfTrack( 0 );
    }
}

auto VirtualDrive::vdrive_read_sector(uint8_t *buf, unsigned int track, unsigned int sector) -> int {
    disk_addr_t dadr;
    int ret;

    /* check image mode */
    if (!structure->media) {
        return CBMDOS_IPE_NOT_READY;
    }

    dadr.track = track;
    dadr.sector = sector;

  //  ret = disk_image_read_sector(vdrive->image, buf, &dadr);

    if (structure->readSector(buf, dadr.track, dadr.sector ))
        return CBMDOS_IPE_OK;

    return CBMDOS_IPE_OK;
}

auto VirtualDrive::iec_read_sequential(uint8_t *data, unsigned int secondary) -> int {
    bufferinfo_t *p = &(vdrive.buffers[secondary]);
    int status;
    unsigned int track, sector;

    if (p->readmode != CBMDOS_FAM_READ) {
        *data = 0xc7;
        return SERIAL_ERROR;
    }

    *data = p->buffer[p->bufptr];
    if (p->length != 0) {
        if (p->bufptr == p->length) {
            p->bufptr = 0xff;
        }
    }
    p->bufptr = (p->bufptr + 1) & 0xff;
    if (p->bufptr) {
        return SERIAL_OK;
    }
    /* do not signal EOF when p->small is 1; get new data */
    if (!p->small && p->length) {
        p->readmode = CBMDOS_FAM_EOF;
        return SERIAL_EOF;
    }

    switch (p->mode) {
        case BUFFER_SEQUENTIAL:

            track = (unsigned int)p->buffer[0];
            sector = (unsigned int)p->buffer[1];

            status = vdrive_read_sector(p->buffer, track, sector);
            p->length = p->buffer[0] ? 0 : p->buffer[1];
            vdrive_set_last_read(track, sector, p->buffer);

            if (status == 0) {
                p->bufptr = 2;
            } else {
                p->readmode = CBMDOS_FAM_EOF;
            }
            break;
        case BUFFER_DIRECTORY_READ:
            system->interface->log("todo dir read");
            p->bufptr = 0;
            break;
        case BUFFER_PARTITION_READ:
            system->interface->log("todo part read");
            p->bufptr = 0;
            break;
        case BUFFER_DIRECTORY_MORE_READ:
            system->interface->log("todo dir more read");
            p->mode = BUFFER_DIRECTORY_READ;
            /* p->small = 1 by now */
         //   p->partition = vdrive->dir_part;
            /* switch to the other drive if possible */
           // if (!vdrive_iec_switch(vdrive, p)) {
             //   p->length = vdrive_dir_first_directory(vdrive, NULL,0, CBMDOS_FT_DEL, p);
               // p->bufptr = 0;
            //} else {
                /* if not, the transfer is done */
                p->readmode = CBMDOS_FAM_EOF;
                return SERIAL_EOF;
            //}
            break;
    }

    return SERIAL_OK;
}

auto VirtualDrive::iec_open_read_directory(unsigned int secondary, cbmdos_cmd_parse_plus_t *cmd_parse) -> int {
    int retlen;
    bufferinfo_t* p = &(vdrive.buffers[secondary]);

    /* we should already be in the proper partition at this point */
    if (secondary > 0) {
        return iec_open_read_sequential(secondary, vdrive.Header_Track, vdrive.Header_Sector);
    }

    vdrive_alloc_buffer(p, BUFFER_DIRECTORY_READ);

    p->timemode = 0;
    if (cmd_parse->command && cmd_parse->commandlength > 2
        && cmd_parse->command[1] == '=') {
        if (cmd_parse->command[2] == 'T') {
            /* if this is CMD time listing, pass on information to
               vdrive_dir_first_directory thru "done" */
            p->timemode = 1;
        }
    }
    //retlen = vdrive_dir_first_directory(cmd_parse->file, cmd_parse->filelength, CBMDOS_FT_DEL, p);

    p->length = (unsigned int)retlen;
    p->bufptr = 0;

    return SERIAL_OK;
}

auto VirtualDrive::vdrive_command_switchtraverse(cbmdos_cmd_parse_plus_t *cmd) -> int {
    int status, rc;
    uint8_t* slot, * next;
    //vdrive_dir_context_t dir;
    int i, skip;
    uint8_t buffer[256];

    status = CBMDOS_IPE_NOT_READY;

   // if (vdrive_command_switch(vdrive, cmd->drive)) {
     //   goto out;
    //}

    /* traverse paths if on supported image, otherwise ignore it - that is what
        the CMD ROMs do */

    /* swap filename to pathname if command is CD to simplify coding */
    if (cmd->commandlength == 2 && cmd->command[0] == 'C'
        && cmd->command[1] == 'D' && cmd->pathlength == 0) {
        cmd->path = cmd->file;
        cmd->pathlength = cmd->filelength;
        cmd->file = NULL;
        cmd->filelength = 0;
    }

    status = CBMDOS_IPE_OK;
    out:
    return status;
}

auto VirtualDrive::vdrive_dir_find_first_slot(const uint8_t *name, int length, unsigned int type, vdrive_dir_context_t *dir) -> void {
    if (length > 0) {
        uint8_t *nslot;

        nslot = cbmdos_dir_slot_create((char*)name, length);
        memcpy(dir->find_nslot, nslot, CBMDOS_SLOT_NAME_LENGTH);
        std::free(nslot);
    }

    //dir->vdrive = vdrive;
    dir->find_length = length;
    dir->find_type = type;

    dir->track = vdrive.Header_Track;
    dir->sector = vdrive.Header_Sector;
    dir->slot = 7;

    /* date comparisons; show everything */
    dir->time_low = 0;
    dir->time_high = 0xffffffff;

    vdrive_read_sector(dir->buffer, dir->track, dir->sector);

    /* old drives may have needed this, but NP's keep their info correct */
   // if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_NP) {
        dir->buffer[0] = vdrive.Dir_Track;
        dir->buffer[1] = vdrive.Dir_Sector;
    //}
}

auto VirtualDrive::cbmdos_dir_slot_create(const char *name, unsigned int len) -> uint8_t* {
    uint8_t *slot;

    if (len > CBMDOS_SLOT_NAME_LENGTH) {
        len = CBMDOS_SLOT_NAME_LENGTH;
    }

    slot = (uint8_t*)std::malloc(CBMDOS_SLOT_NAME_LENGTH);
    memset(slot, 0xa0, CBMDOS_SLOT_NAME_LENGTH);

    memcpy(slot, name, (size_t)len);

    return slot;
}

auto VirtualDrive::vdrive_dir_find_next_slot(vdrive_dir_context_t *dir) -> uint8_t* {
    static uint8_t return_slot[32];
    //vdrive_t *vdrive = dir->vdrive;
    uint8_t *tmp;
    int j;
    unsigned int t, s, c;
    uint8_t *dirbuf = nullptr;


    /*
     * Loop all directory blocks starting from track 18, sector 1 (1541).
     */

    do {
        /*
         * Load next(first) directory block ?
         */

        dir->slot++;

        if (dir->slot >= 8) {
            int status;

            /* end of current directory? */
            if (dir->buffer[0] == 0) {
                break;
            }

            dir->slot = 0;
            dir->track = (unsigned int)dir->buffer[0];
            dir->sector = (unsigned int)dir->buffer[1];

            status = vdrive_read_sector(dir->buffer, dir->track, dir->sector);
            if (status != 0) {
                return NULL; /* error */
            }
        }

        if (vdrive_dir_name_match(&dir->buffer[dir->slot * 32],
                                  dir->find_nslot, dir->find_length,
                                  dir->find_type)) {
            memcpy(return_slot, &dir->buffer[dir->slot * 32], 32);
            /* check date range; for DIR listings */
//            t = date_to_int(return_slot[SLOT_GEOS_YEAR], return_slot[SLOT_GEOS_MONTH],
//                            return_slot[SLOT_GEOS_DATE], return_slot[SLOT_GEOS_HOUR],
//                            return_slot[SLOT_GEOS_MINUTE] );
//            /* time_low is initially 0, and time_high is initially largest,
//                so it should always match for most uses. */
//            if (t >= dir->time_low && t <= dir->time_high)
                return return_slot;
        }
    } while (1);

    /*
     * If length < 0, create new directory-entry if possible
     */
    if (dir->find_length < 0) {
        int i, h, h2;
        unsigned int sector, max_sector, max_sector_all;

        max_sector = Structure1541::countSectors(dir->track);
        max_sector_all = Structure1541::countSectors(dir->track);
        h = (dir->sector / max_sector) * max_sector;
        sector = dir->sector % max_sector;
        sector += 3;
        if (sector >= max_sector) {
            sector -= max_sector;
            if (sector != 0) {
                sector--;
            }
        }
        /* go through all groups, 1 round for most CBM drives */
        for (h2 = 0; h2 < max_sector_all; h2 += max_sector) {
            for (i = 0; i < max_sector; i++) {
              //  dirbuf = find_next_directory_sector(dir, dir->track, sector + h);
                dirbuf = nullptr;
                if (dirbuf != NULL) {
                    return dirbuf;
                }
                sector++;
                if (sector >= max_sector) {
                    sector = 0;
                }
            }

            h += max_sector;
            if (h >= max_sector_all) {
                h = 0;
            }
        }

    }
    return nullptr;
}

auto VirtualDrive::vdrive_dir_name_match(uint8_t *slot, uint8_t *nslot, int length, int type) -> unsigned int {
    if (length < 0) {
        if (slot[SLOT_TYPE_OFFSET]) {
            return 0;
        } else {
            return 1;
        }
    }

    if (!slot[SLOT_TYPE_OFFSET]) {
        return 0;
    }

    if (type != CBMDOS_FT_DEL && type != (slot[SLOT_TYPE_OFFSET] & 0x07)) {
        return 0;
    }

    return cbmdos_parse_wildcard_compare(nslot, &slot[SLOT_NAME_OFFSET]);
}

auto VirtualDrive::cbmdos_parse_wildcard_compare(const uint8_t *name1, const uint8_t *name2) -> unsigned int {
    unsigned int index;

    for (index = 0; index < CBMDOS_SLOT_NAME_LENGTH; index++) {

        switch (name1[index]) {
            case '*':
                return 1; /* rest is not interesting, it's a match */
            case '?':
                if (name2[index] == 0xa0) {
                    return 0; /* wildcard, but the other is too short */
                }
                break;
            case 0xa0: /* This one ends, let's see if the other as well */
                return (name2[index] == 0xa0);
            case ':':
                if (index == 0)
                    break;
            default:
                if (name1[index] != name2[index]) {
                    return 0; /* does not match */
                }
        }
    }

    return 1; /* matched completely */
}

auto VirtualDrive::iec_open_read( unsigned int secondary) -> int {
    int type;
    unsigned int track, sector;
    bufferinfo_t *p = &(vdrive.buffers[secondary]);
    uint8_t *slot = p->slot;

    /* we should already be in the proper partition at this point */
    if (!slot) {
        close( secondary);
        vdrive_command_set_error(CBMDOS_IPE_NOT_FOUND, 0, 0);
        //system->interface->log("no slot");
        return SERIAL_ERROR;
    }

    type = slot[SLOT_TYPE_OFFSET] & 0x07;
    track = (unsigned int)slot[SLOT_FIRST_TRACK];
    sector = (unsigned int)slot[SLOT_FIRST_SECTOR];

    /* we can not open files that were not properly closed ("splat files") */
    if (slot[SLOT_TYPE_OFFSET] & 0x80) {
        /* Del, Seq, Prg, Usr (Rel not supported here).  */
        if (type != CBMDOS_FT_REL) {
            return iec_open_read_sequential(secondary, track, sector);
        }
    }

    return SERIAL_ERROR;
}

// command parse

auto VirtualDrive::cbmdos_command_parse_plus(cbmdos_cmd_parse_plus_t *cmd_parse) -> unsigned int {
    const uint8_t *p, *limit, *p1, *p2;
    uint8_t temp[256];
    int special = 0;
    int i, templength = 0;

    cmd_parse->command = NULL;
    cmd_parse->commandlength = 0;
    cmd_parse->abbrv = NULL;
    cmd_parse->abbrvlength = 0;
    cmd_parse->path = NULL;
    cmd_parse->pathlength = 0;
    cmd_parse->file = NULL;
    cmd_parse->filelength = 0;
    cmd_parse->more = NULL;
    cmd_parse->morelength = 0;
    cmd_parse->recordlength = 0;
    cmd_parse->filetype = 0;
    cmd_parse->readmode = (cmd_parse->secondary == 1)
                          ? CBMDOS_FAM_WRITE : CBMDOS_FAM_READ;
    /* drive is not reset here */

    if (cmd_parse->full == NULL || cmd_parse->fulllength == 0) {
        return CBMDOS_IPE_NO_NAME;
    }

    p = cmd_parse->full;
    limit = p + cmd_parse->fulllength;

    /* in file mode */
    if ((cmd_parse->mode == 0 || cmd_parse->mode == 2) && p < limit) {
        if (cmd_parse->mode == 0) {
            cmd_parse->drive = 0;
        }
        /* look for a ':' to separate the unit/part-path from name */
        p2 = (uint8_t*)std::memchr(p, ':', limit - p);
        if (p2) {
            if (*(p2+1) == '*')
                p2 = NULL;
        }
        /* check for special commands without : */
        if (*p == '$' || *p == '#') {
            special = 1;
        }
        /* check for special commands with : */
        if (p2 && *p == '@') {
            special = 1;
        }
        if (p2 || special) {
            /* check for anything before unit/partition number (@,&), but not a '/' */
            if (*p != '/' && (*p < '0' || *p > '9')) {
                p1 = p;
                /* wait for numbers to appear */
                if (special) {
                    /* compensate for CMD $*=P and $*=T syntax */
                    if (*p == '$' && (p + 2) < limit && *(p + 1) == '='
                        && (*(p + 2) == 'P' || *(p + 2) == 'T') ) {
                        p += 2;
                    } else if (!p2 && *p == '$' && (p + 1) < limit
                               && *(p + 1) >= '0' && *(p + 1) <= '9') {
                        /* compensate for just $n */
                        p2 = limit;
                    }
                    p++;
                } else {
                    while (p < limit && (*p < '0' || *p > '9')) {
                        p++;
                    }
                }
                cmd_parse->commandlength = (unsigned int)(p - p1);
                cmd_parse->command = (uint8_t*)std::calloc(1, cmd_parse->commandlength + 1);
                memcpy(cmd_parse->command, p1, cmd_parse->commandlength);
                cmd_parse->command[cmd_parse->commandlength] = 0;
            }
            /* if in special mode, and no unit/path is provided, anything after first character is the filename */
            if (p2) {
                /* skip any more spaces */
                while (p < limit && *p == ' ') {
                    p++;
                }
                /* get unit/part number if not at ':' yet and next value is a digit*/
                if (p < p2 && (*p >= '0' || *p <= '9')) {
                    cmd_parse->drive = 0;
                    while (p < p2 && (*p >= '0' && *p <= '9')) {
                        /* unit/part should never be more than 256, if so, skip them */
                        i = cmd_parse->drive * 10 + (*p - '0');
                        if (i < 256) {
                            cmd_parse->drive = i;
                        }
                        p++;
                    }
                }
                /* skip any more spaces */
                while (p < limit && *p == ' ') {
                    p++;
                }
/* TODO: CMD limits paths by having them begin and end with a '/' */
                /* get path if not at ':' yet and next value is not a ':'; just use p2 limit here*/
                if (p < p2) {
                    p1 = p;
                    p = p2;
                    cmd_parse->pathlength = (unsigned int)(p - p1);
                    cmd_parse->path = (uint8_t*)std::calloc(1, cmd_parse->pathlength + 1);
                    memcpy(cmd_parse->path, p1, cmd_parse->pathlength);
                    cmd_parse->path[cmd_parse->pathlength] = 0;
                    p = p2;
                }
            }
        }
        /* skip first colon, others are allowed in the file name */
        if (*p == ':') {
            p++;
        }
        /* file to follow */
        p1 = p;
        p = p2 = (uint8_t*)std::memchr(p, ',', limit - p);
        /* find end of input or first ',' */
        /* compensate for CMD $*=P, and $*=T, syntax (coma) */
        if (p2 && cmd_parse->command && cmd_parse->command[0]=='$') {
            p = NULL;
        }
        if (!p) {
            p = limit;
        }
        cmd_parse->filelength = (unsigned int)(p - p1);
        cmd_parse->file = (uint8_t*)std::calloc(1, cmd_parse->filelength + 1);
        memcpy(cmd_parse->file, p1, cmd_parse->filelength);
        cmd_parse->file[cmd_parse->filelength] = 0;

        /* Preset the file-type if the LOAD/SAVE secondary addresses are used. */
        cmd_parse->filetype = (cmd_parse->secondary < 2) ? CBMDOS_FT_PRG : 0;

        if (p2) {
            /* type and mode next */
            while (p < limit) {
                p++;
                switch (*p) {
                    case 'S':
                        cmd_parse->filetype = CBMDOS_FT_SEQ;
                        break;
                    case 'P':
                        cmd_parse->filetype = CBMDOS_FT_PRG;
                        break;
                    case 'U':
                        cmd_parse->filetype = CBMDOS_FT_USR;
                        break;
                    case 'L'|0x80:
                    case 'L':                   /* L,(#record length)  max 254 */
                        if (p+2 < limit && p[1] == ',') {
                            cmd_parse->recordlength = p[2]; /* Changing RL causes error */
                            /* Don't allow REL file record lengths less than 2 or
                               greater than 254.  The 1541/71/81 lets you create a
                               REL file of record length 0, but it locks up the CPU
                               on the drive - nice. */
                            if (cmd_parse->recordlength < 2 || cmd_parse->recordlength > 254) {
                                return CBMDOS_IPE_OVERFLOW;
                            }
                            /* skip the REL length */
                            p += 2;
                            cmd_parse->filetype = CBMDOS_FT_REL;
                        } else {
                            return CBMDOS_IPE_OVERFLOW;
                        }
                        break;
                    case 'R':
                        cmd_parse->readmode = CBMDOS_FAM_READ;
                        break;
                    case 'W':
                        cmd_parse->readmode = CBMDOS_FAM_WRITE;
                        break;
                    case 'A':
                        cmd_parse->readmode = CBMDOS_FAM_APPEND;
                        break;
                    default:
                        return CBMDOS_IPE_INVAL;
                }
                p++;
                /* skip extra characters after first ','; ",sequential,write" is allowed for example */
                p = (uint8_t*)std::memchr(p, ',', limit - p);
                if (!p) {
                    break;
                }
            }
            /* Override read mode if secondary is 0 or 1.  */
            if (cmd_parse->secondary == 0) {
                cmd_parse->readmode = CBMDOS_FAM_READ;
            }
            if (cmd_parse->secondary == 1) {
                cmd_parse->readmode = CBMDOS_FAM_WRITE;
            }
        }
    } else
        /* in standard (flexible) command mode; too hard to merge everything
            - too many special cases */
    if (cmd_parse->mode == 1 && p < limit) {
        cmd_parse->drive = 0;
        /* any command beginning with P, or M do not require any parsing,
            copy all information into command */
        /* only process U1, U2, UA, UB */
        special = 0;
        if (*p == 'U' || *p == 'M') {
            special++;
            if (p + 1 < limit) {
                if (p[0] == 'U'
                    && (p[1] == '1' || p[1] == 'A' || p[1] == '2' || p[1] == 'B')) {
                    special = 0;
                }
                if (p[0] == 'M' && p[1] == 'D') {
                    special = 0;
                }
            } else {
                return CBMDOS_IPE_INVAL;
            }
        }
        if (special || *p == 'P') {
            cmd_parse->commandlength = cmd_parse->fulllength;
            cmd_parse->command = (uint8_t*)std::calloc(1, cmd_parse->commandlength);
            memcpy(cmd_parse->command, cmd_parse->full, cmd_parse->commandlength);
            return CBMDOS_IPE_OK;
        }
        /* look for a ':' to separate the unit/part-path from name */
        p2 = (uint8_t*)std::memchr(p, ':', limit - p);

        special = (*p == 'U');
        /* check for command before unit/partition number (I,V,C,etc) */
        if (*p < '0' || *p > '9') {
            p1 = p;
            /* compensate for "BLOCK-ALLOCATE", for example, which will
                become "B-A" , but not for UA, etc*/
            while (p < limit) {
                /* alpha */
                temp[templength++] = *p;
                p++;
                if(p >= limit) {
                    break;
                }
                /* if first character of command is not alpha, then it is only 1 character */
                if(*p1 < 'A') {
                    break;
                }
                /* if command is CP or C(shift P) */
                if (*p1 == 'C') {
                    if (*p == 'P' || *p == 'D') {
                        /* for CP, partition # is the file name */
                        /* for CD, "<-" is the file name, or the unit and part
                            go in the right place */
                        p++;
                        break;
                    } else if (*p == ('P' | 0x80)) {
                        /* for C{shift}P, copy the command */
                        cmd_parse->commandlength = 3;
                        cmd_parse->command = (uint8_t*)std::calloc(1, cmd_parse->commandlength);
                        memcpy(cmd_parse->command, cmd_parse->full, cmd_parse->commandlength);
                        return CBMDOS_IPE_OK;
                    }
                } else if (*p1 == 'M' && *p == 'D') {
                    /* MD */
                    p++;
                    break;
                }
                /* wait for non-alpha to appear or next character after U */
                if (!special) {
                    while (p < limit
                           && ((*p >= 'A' && *p <= 'Z')
                               || (*p >= ('A' | 0x80) && *p <= ('Z' | 0x80)))) {
                        p++;
                    }
                }
                if (p >= limit) {
                    break;
                }
                /* get out the moment we see a delimiter */
                if (*p == ':' || *p == ' ' || *p == 29 ) {
                    break;
                }
                if (!special) {
                    if (*p == '/' || (*p >= '0' && *p <= '9')) {
                        break;
                    }
                }
                /* non-alpha */
                temp[templength++] = *p;
                p++;
                if (p >= limit) {
                    break;
                }
                /* get out the moment we see a delimiter */
                if (*p == ':' || *p == ' ' || *p == 29 ) {
                    break;
                }
                if (!special) {
                    if (*p == '/' || (*p >= '0' && *p <= '9')) {
                        break;
                    }
                }
            }

            cmd_parse->commandlength = (unsigned int)(p - p1);
            cmd_parse->command = (uint8_t*)std::calloc(1, cmd_parse->commandlength + 1);
            memcpy(cmd_parse->command, p1, cmd_parse->commandlength);
            cmd_parse->command[cmd_parse->commandlength] = 0;
            cmd_parse->abbrvlength = templength;
            cmd_parse->abbrv = (uint8_t*)std::calloc(1, cmd_parse->abbrvlength + 1);
            memcpy(cmd_parse->abbrv, temp, cmd_parse->abbrvlength);
            cmd_parse->abbrv[cmd_parse->abbrvlength] = 0;
        }

        if (p >= limit) {
            return CBMDOS_IPE_OK;
        }

        /* skip any spaces or cursor-lefts */
        while (p < limit && (*p == ' ' || *p == 29) ) {
            p++;
        }

        /* get unit/part number if not at ':' yet and next value is a digit*/
        if (p < p2 && (*p >= '0' || *p <= '9')) {
            while (p < p2 && (*p >= '0' && *p <= '9')) {
                /* unit/part should never be more than 256, if so, skip them */
                i = cmd_parse->drive * 10 + (*p - '0');
                if (i < 256) {
                    cmd_parse->drive = i;
                }
                p++;
            }
        }

        /* skip any more spaces */
        while (p < limit && *p ==' ') {
            p++;
        }

        /* get path if not at ':' yet and next value is not a ':'; just use p2 limit here*/
        if (p < p2) {
            p1 = p;
            p = p2;
            cmd_parse->pathlength = (unsigned int)(p - p1);
            cmd_parse->path = (uint8_t*)std::calloc(1, cmd_parse->pathlength + 1);
            memcpy(cmd_parse->path, p1, cmd_parse->pathlength);
            cmd_parse->path[cmd_parse->pathlength] = 0;
            p = p2;
        }

        /* skip first colon, others are allowed in the file name */
        if (*p == ':') {
            p++;
        }
        /* file to follow */
        p1 = p;
        p = (uint8_t*)std::memchr(p1, '=', limit - p1);
        p2 = (uint8_t*)std::memchr(p1, ',', limit - p1);
        if (p && p2 && (p2 < p)) {
            p = p2;
        } else if (!p && p2) {
            p = p2;
        }
        /* find end of input or first ',' */
        if (!p) {
            p = limit;
        }
        cmd_parse->filelength = (unsigned int)(p - p1);
        cmd_parse->file = (uint8_t*)std::calloc(1, cmd_parse->filelength + 1);
        memcpy(cmd_parse->file, p1, cmd_parse->filelength);
        cmd_parse->file[cmd_parse->filelength] = 0;

        if (p < limit) {
            cmd_parse->morelength = (unsigned int)(limit - p);
            cmd_parse->more = (uint8_t*)std::calloc(1, cmd_parse->morelength + 1);
            memcpy(cmd_parse->more, p, cmd_parse->morelength);
            cmd_parse->more[cmd_parse->morelength] = 0;
        }

    }

    return CBMDOS_IPE_OK;
}

}

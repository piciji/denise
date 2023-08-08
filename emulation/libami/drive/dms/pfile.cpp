
/*
*     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
*     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
*
*     Handles the processing of a single DMS archive
*     modifications: put all in a single compilation unit, make it portable
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace AMI_DMS {


#define HEADLEN 56
#define THLEN 20
#define TRACK_BUFFER_LEN 32000
#define TEMP_BUFFER_LEN 32000
#define MAX_OUT_SIZE (1760 * 512)


#include "cdata.h"
#include "pfile.h"
#include "tables.cpp"
#include "maketbl.cpp"
#include "getbits.cpp"
#include "u_init.cpp"
#include "u_rle.cpp"
#include "u_quick.cpp"
#include "u_medium.cpp"
#include "u_deep.cpp"
#include "u_heavy.cpp"
#include "crc_csum.cpp"


#define DMSFLAG_ENCRYPTED 2
#define DMSFLAG_HD 16

static USHORT Process_Track(unsigned char **, unsigned&, unsigned, unsigned char **, unsigned&, UCHAR *, UCHAR *, USHORT, int);
static USHORT Unpack_Track(UCHAR *, UCHAR *, USHORT, USHORT, UCHAR, UCHAR, USHORT, USHORT, USHORT, int);

static int passfound, passretries;

static char modes[7][7]={"NOCOMP","SIMPLE","QUICK ","MEDIUM","DEEP  ","HEAVY1","HEAVY2"};
static USHORT PWDCRC;


USHORT Process_File(unsigned char *fi, unsigned fiSize, unsigned char **fo, unsigned& foSize)
{
	USHORT from, to, geninfo, c_version, cmode, hcrc, disktype, pv, ret;
	ULONG pkfsize, unpkfsize;
	UCHAR *b1, *b2;
	//time_t date;
    USHORT cmd = CMD_UNPACK;
    USHORT PCRC = 0;
    USHORT pwd = 0;
    unsigned fiPos = 0;

	passfound = 0;
	passretries = 2;

    if (fiSize < HEADLEN) {
        return ERR_SREAD;
    }

    if ( (fi[0] != 'D') || (fi[1] != 'M') || (fi[2] != 'S') || (fi[3] != '!') ) {
        /*  Check the first 4 bytes of file to see if it is "DMS!"  */
        return ERR_NOTDMS;
    }

    b1 = new unsigned char[TRACK_BUFFER_LEN];
    b2 = new unsigned char[TRACK_BUFFER_LEN];
    dms_text = new unsigned char[TEMP_BUFFER_LEN];

    memcpy(b1, fi, HEADLEN);
    fi += HEADLEN;
    fiPos += HEADLEN;

	hcrc = (USHORT)((b1[HEADLEN-2]<<8) | b1[HEADLEN-1]);
	/* Header CRC */

	if (hcrc != dms_CreateCRC(b1+4,(ULONG)(HEADLEN-6))) {
		free(b1);
		free(b2);
		free(dms_text);
		return ERR_HCRC;
	}

    *fo = new unsigned char[MAX_OUT_SIZE];
	geninfo = (USHORT) ((b1[10]<<8) | b1[11]);	/* General info about archive */
	//date = (time_t) ((((ULONG)b1[12])<<24) | (((ULONG)b1[13])<<16) | (((ULONG)b1[14])<<8) | (ULONG)b1[15]);	/* date in standard UNIX/ANSI format */
	from = (USHORT) ((b1[16]<<8) | b1[17]);		/*  Lowest track in archive. May be incorrect if archive is "appended" */
	to = (USHORT) ((b1[18]<<8) | b1[19]);		/*  Highest track in archive. May be incorrect if archive is "appended" */

	pkfsize = (ULONG) ((((ULONG)b1[21])<<16) | (((ULONG)b1[22])<<8) | (ULONG)b1[23]);	/*  Length of total packed data as in archive   */
	unpkfsize = (ULONG) ((((ULONG)b1[25])<<16) | (((ULONG)b1[26])<<8) | (ULONG)b1[27]);	/*  Length of unpacked data. Usually 901120 bytes  */

	c_version = (USHORT) ((b1[46]<<8) | b1[47]);	/*  version of DMS used to generate it  */
	disktype = (USHORT) ((b1[50]<<8) | b1[51]);		/*  Type of compressed disk  */
	cmode = (USHORT) ((b1[52]<<8) | b1[53]);        /*  Compression mode mostly used in this archive  */

	PWDCRC = PCRC;

	if (disktype == 7) {
		/*  It's not a DMS compressed disk image, but a FMS archive  */
		free(b1);
		free(b2);
		free(dms_text);
		return ERR_FMS;
	}

	ret=NO_PROBLEM;

	Init_Decrunchers();

	if (cmd != CMD_VIEW) {
		if (cmd == CMD_SHOWBANNER) /*  Banner is in the first track  */
			ret = Process_Track(&fi, fiPos, fiSize, NULL, foSize, b1,b2,cmd,geninfo);
		else {
			Init_Decrunchers();
			for (;;) {
				ret = Process_Track(&fi, fiPos, fiSize, fo, foSize, b1,b2,cmd,geninfo);
				if (ret == DMS_FILE_END)
					break;
				if (ret == NO_PROBLEM)
					continue;
				// ignore posible extra data at the end of archive if output file is already complete
				if ((ret == ERR_SREAD || ret == ERR_NOTTRACK || ret == ERR_THCRC || ret == ERR_BIGTRACK) && foSize >= unpkfsize) {
					ret = DMS_FILE_END;
					break;
				}
				break;
			}
		}
	}

	if (ret == DMS_FILE_END) ret = NO_PROBLEM;


	/*  Used to give an error message, but I have seen some DMS  */
	/*  files with texts or zeros at the end of the valid data   */
	/*  So, when we find something that is not a track header,   */
	/*  we suppose that the valid data is over. And say it's ok. */
	if (ret == ERR_NOTTRACK) ret = NO_PROBLEM;

	free(b1);
	free(b2);
	free(dms_text);

	return ret;
}

static USHORT Process_Track(unsigned char **fi, unsigned& fiPos, unsigned fiSize, unsigned char **fo, unsigned& foSize, UCHAR *b1, UCHAR *b2, USHORT cmd, int dmsflags){
	USHORT hcrc, dcrc, usum, number, pklen1, pklen2, unpklen, l;
	UCHAR cmode, flags;
	int crcerr = 0;
	bool normaltrack;

    if (fiPos >= fiSize)
        return DMS_FILE_END;

    if ((fiPos + THLEN) > fiSize)
        return ERR_SREAD;

    memcpy(b1, *fi, THLEN);
    fiPos += THLEN;
    *fi += THLEN;

	/*  "TR" identifies a Track Header  */
	if ((b1[0] != 'T')||(b1[1] != 'R'))
		return ERR_NOTTRACK;

	/*  Track Header CRC  */
	hcrc = (USHORT)((b1[THLEN-2] << 8) | b1[THLEN-1]);

	if (dms_CreateCRC(b1,(ULONG)(THLEN-2)) != hcrc)
		return ERR_THCRC;

	number = (USHORT)((b1[2] << 8) | b1[3]);	/*  Number of track  */
	pklen1 = (USHORT)((b1[6] << 8) | b1[7]);	/*  Length of packed track data as in archive  */
	pklen2 = (USHORT)((b1[8] << 8) | b1[9]);	/*  Length of data after first unpacking  */
	unpklen = (USHORT)((b1[10] << 8) | b1[11]);	/*  Length of data after subsequent rle unpacking */
	flags = b1[12];		/*  control flags  */
	cmode = b1[13];		/*  compression mode used  */
	usum = (USHORT)((b1[14] << 8) | b1[15]);	/*  Track Data CheckSum AFTER unpacking  */
	dcrc = (USHORT)((b1[16] << 8) | b1[17]);	/*  Track Data CRC BEFORE unpacking  */

	if ((pklen1 > TRACK_BUFFER_LEN) || (pklen2 >TRACK_BUFFER_LEN) || (unpklen > TRACK_BUFFER_LEN))
		return ERR_BIGTRACK;

    if ((fiPos + pklen1) > fiSize)
        return ERR_SREAD;

    memcpy(b1, *fi, pklen1);
    fiPos += pklen1;
    *fi += pklen1;

	if (dms_CreateCRC(b1,(ULONG)pklen1) != dcrc) {
		crcerr = 1;
	}
	/*  track 80 is FILEID.DIZ, track 0xffff (-1) is Banner  */
	/*  and track 0 with 1024 bytes only is a fake boot block with more advertising */
	/*  FILE_ID.DIZ is never encrypted  */

	normaltrack = false;
	if ((cmd == CMD_UNPACK) && (number<80) && (unpklen>2048)) {
		memset(b2, 0, unpklen);
		if (!crcerr) {
			Unpack_Track(b1, b2, pklen2, unpklen, cmode, flags, number, pklen1, usum, dmsflags & DMSFLAG_ENCRYPTED);
		}

        unsigned outPos = number * 512 * 22 * ((dmsflags & DMSFLAG_HD) ? 2 : 1);
        unsigned char* out = *fo + outPos;

        if ((outPos + unpklen) > MAX_OUT_SIZE)
            return ERR_CANTWRITE;

        memcpy(out, b2, unpklen);
        foSize += unpklen;

		normaltrack = true;
	} else if (number == 0 && unpklen == 1024) {
		memset(b2, 0, unpklen);
		if (!crcerr)
			Unpack_Track(b1, b2, pklen2, unpklen, cmode, flags, number, pklen1, usum, dmsflags & DMSFLAG_ENCRYPTED);
	}

	if (crcerr)
		return NO_PROBLEM;


	if (!normaltrack)
		Init_Decrunchers();

	return NO_PROBLEM;

}


static USHORT Unpack_Track_2(UCHAR *b1, UCHAR *b2, USHORT pklen2, USHORT unpklen, UCHAR cmode, UCHAR flags){
	switch (cmode){
		case 0:
			/*   No Compression   */
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 1:
			/*   Simple Compression   */
			if (Unpack_RLE(b1,b2,unpklen)) return ERR_BADDECR;
			break;
		case 2:
			/*   Quick Compression   */
			if (Unpack_QUICK(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 3:
			/*   Medium Compression   */
			if (Unpack_MEDIUM(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 4:
			/*   Deep Compression   */
			if (Unpack_DEEP(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 5:
		case 6:
			/*   Heavy Compression   */
			if (cmode==5) {
				/*   Heavy 1   */
				if (Unpack_HEAVY(b1,b2,flags & 7,pklen2)) return ERR_BADDECR;
			} else {
				/*   Heavy 2   */
				if (Unpack_HEAVY(b1,b2,flags | 8,pklen2)) return ERR_BADDECR;
			}
			if (flags & 4) {
				memset(b1,0,unpklen);
				/*  Unpack with RLE only if this flag is set  */
				if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
				memcpy(b2,b1,(size_t)unpklen);
			}
			break;
		default:
			return ERR_UNKNMODE;
	}

	if (!(flags & 1)) Init_Decrunchers();

	return NO_PROBLEM;

}

/*  DMS uses a lame encryption  */
static void dms_decrypt(UCHAR *p, USHORT len, UCHAR *src){
	USHORT t;

	while (len--){
		t = (USHORT) *src++;
		*p++ = t ^ (UCHAR)PWDCRC;
		PWDCRC = (USHORT)((PWDCRC >> 1) + t);
	}
}

static USHORT Unpack_Track(UCHAR *b1, UCHAR *b2, USHORT pklen2, USHORT unpklen, UCHAR cmode, UCHAR flags, USHORT number, USHORT pklen1, USHORT usum1, int enc)
{
	USHORT r, err = NO_PROBLEM;
	static USHORT pass;
	int maybeencrypted;
	int pwrounds;
	UCHAR *tmp;
	USHORT prevpass = 0;

	if (passfound) {
		if (number != 80)
			dms_decrypt(b1, pklen1, b1);
		r = Unpack_Track_2(b1, b2, pklen2, unpklen, cmode, flags);
		if (r == NO_PROBLEM) {
			if (usum1 == dms_Calc_CheckSum(b2,(ULONG)unpklen))
				return NO_PROBLEM;
		}

		if (passretries <= 0)
			return ERR_CSUM;
	}

	passretries--;
	pwrounds = 0;
	maybeencrypted = 0;
	tmp = (unsigned char*)malloc (pklen1);
	memcpy (tmp, b1, pklen1);
	memset(b2, 0, unpklen);
	for (;;) {
		r = Unpack_Track_2(b1, b2, pklen2, unpklen, cmode, flags);
		if (r == NO_PROBLEM) {
			if (usum1 == dms_Calc_CheckSum(b2,(ULONG)unpklen)) {
				passfound = maybeencrypted;

				err = NO_PROBLEM;
				pass = prevpass;
				break;
			}
		}
		if (number == 80 || !enc) {
			err = ERR_CSUM;
			break;
		}
		maybeencrypted = 1;
		prevpass = pass;
		PWDCRC = pass;
		pass++;
		dms_decrypt(b1, pklen1, tmp);
		pwrounds++;
		if (pwrounds == 65536) {
			err = ERR_CSUM;
			passfound = 0;
			break;
		}
	}
	free (tmp);
	return err;
}


}

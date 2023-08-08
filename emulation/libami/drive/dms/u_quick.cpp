
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *
 */

#define QBITMASK 0xff

USHORT Unpack_QUICK(UCHAR *in, UCHAR *out, USHORT origsize){
	USHORT i, j;
	UCHAR *outend;

	initbitbuf(in);

	outend = out+origsize;
	while (out < outend) {
		if (GETBITS(1)!=0) {
			DROPBITS(1);
			*out++ = dms_text[dms_quick_text_loc++ & QBITMASK] = (UCHAR)GETBITS(8);  DROPBITS(8);
		} else {
			DROPBITS(1);
			j = (USHORT) (GETBITS(2)+2);  DROPBITS(2);
			i = (USHORT) (dms_quick_text_loc - GETBITS(8) - 1);  DROPBITS(8);
			while(j--) {
				*out++ = dms_text[dms_quick_text_loc++ & QBITMASK] = dms_text[i++ & QBITMASK];
			}
		}
	}
	dms_quick_text_loc = (USHORT)((dms_quick_text_loc+5) & QBITMASK);

	return 0;
}


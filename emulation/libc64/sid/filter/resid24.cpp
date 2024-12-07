
//  This code is the reSID engine from VICE 2.4
//  You can get a copy of the original here: https://sourceforge.net/projects/vice-emu/

//  ---------------------------------------------------------------------------
//  This file is part of VICE, the Versatile Commodore Emulator.
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2010  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

namespace LIBC64 {

auto Sid::Filter::clock24(int voice1, int voice2, int voice3) -> void {
    Calculated& ca = calculated[ Type::MOS_8580 ];
    v1 = (voice1 * ca.voiceScaleS14Old >> 18) + ca.voiceDCOld;
    v2 = (voice2 * ca.voiceScaleS14Old >> 18) + ca.voiceDCOld;
    v3 = (voice3 * ca.voiceScaleS14Old >> 18) + ca.voiceDCOld;

    int Vi;

    switch ( sum & 0xf ) {
        default:
        case 0x0: Vi = 0;                    break;
        case 0x1: Vi = v1;                    break;
        case 0x2: Vi = v2;                    break;
        case 0x3: Vi = v2 + v1;                break;
        case 0x4: Vi = v3;                    break;
        case 0x5: Vi = v3 + v1;                break;
        case 0x6: Vi = v3 + v2;                break;
        case 0x7: Vi = v3 + v2 + v1;        break;
        case 0x8: Vi = ve;                    break;
        case 0x9: Vi = ve + v1;                break;
        case 0xa: Vi = ve + v2;                break;
        case 0xb: Vi = ve + v2 + v1;        break;
        case 0xc: Vi = ve + v3;                break;
        case 0xd: Vi = ve + v3 + v1;        break;
        case 0xe: Vi = ve + v3 + v2;        break;
        case 0xf: Vi = ve + v3 + v2 + v1;    break;
    }

    int dVbp = w0 * (Vhp >> 4) >> 16;
    int dVlp = w0 * (Vbp >> 4) >> 16;
    Vbp -= dVbp;
    Vlp -= dVlp;
    Vhp = (Vbp * _1024_div_Q >> 10) - Vlp - Vi;
}

auto Sid::Filter::output24() -> short {
// <?php
//    $sOut = "";
//    $aInputs = ['v1', 'v2', 'v3', 've', 'Vlp', 'Vbp', 'Vhp'];
//
//    for ($i = 0; $i <= 0x7f; $i++) {
//        $sOut .= "case " . $i . ": ";
//        $sVi = "";
//
//        for ($j = 0; $j < 7; $j++) {
//            if ($i & (1 << $j))
//                $sVi .= $aInputs[ $j ] . " + ";
//        }
//
//        if ($sVi == "")
//            $sVi = "0;";
//        else
//            $sVi = substr($sVi, 0, -3) . ";";
//
//        $sOut .= " Vi = " . $sVi . " break;<br/>";
//    }
//    echo $sOut;

	int Vi = 0;

	switch( mix & 0x7f ) {
        case 0:  Vi = 0; break;
        case 1:  Vi = v1; break;
        case 2:  Vi = v2; break;
        case 3:  Vi = v1 + v2; break;
        case 4:  Vi = v3; break;
        case 5:  Vi = v1 + v3; break;
        case 6:  Vi = v2 + v3; break;
        case 7:  Vi = v1 + v2 + v3; break;
        case 8:  Vi = ve; break;
        case 9:  Vi = v1 + ve; break;
        case 10:  Vi = v2 + ve; break;
        case 11:  Vi = v1 + v2 + ve; break;
        case 12:  Vi = v3 + ve; break;
        case 13:  Vi = v1 + v3 + ve; break;
        case 14:  Vi = v2 + v3 + ve; break;
        case 15:  Vi = v1 + v2 + v3 + ve; break;
        case 16:  Vi = Vlp; break;
        case 17:  Vi = v1 + Vlp; break;
        case 18:  Vi = v2 + Vlp; break;
        case 19:  Vi = v1 + v2 + Vlp; break;
        case 20:  Vi = v3 + Vlp; break;
        case 21:  Vi = v1 + v3 + Vlp; break;
        case 22:  Vi = v2 + v3 + Vlp; break;
        case 23:  Vi = v1 + v2 + v3 + Vlp; break;
        case 24:  Vi = ve + Vlp; break;
        case 25:  Vi = v1 + ve + Vlp; break;
        case 26:  Vi = v2 + ve + Vlp; break;
        case 27:  Vi = v1 + v2 + ve + Vlp; break;
        case 28:  Vi = v3 + ve + Vlp; break;
        case 29:  Vi = v1 + v3 + ve + Vlp; break;
        case 30:  Vi = v2 + v3 + ve + Vlp; break;
        case 31:  Vi = v1 + v2 + v3 + ve + Vlp; break;
        case 32:  Vi = Vbp; break;
        case 33:  Vi = v1 + Vbp; break;
        case 34:  Vi = v2 + Vbp; break;
        case 35:  Vi = v1 + v2 + Vbp; break;
        case 36:  Vi = v3 + Vbp; break;
        case 37:  Vi = v1 + v3 + Vbp; break;
        case 38:  Vi = v2 + v3 + Vbp; break;
        case 39:  Vi = v1 + v2 + v3 + Vbp; break;
        case 40:  Vi = ve + Vbp; break;
        case 41:  Vi = v1 + ve + Vbp; break;
        case 42:  Vi = v2 + ve + Vbp; break;
        case 43:  Vi = v1 + v2 + ve + Vbp; break;
        case 44:  Vi = v3 + ve + Vbp; break;
        case 45:  Vi = v1 + v3 + ve + Vbp; break;
        case 46:  Vi = v2 + v3 + ve + Vbp; break;
        case 47:  Vi = v1 + v2 + v3 + ve + Vbp; break;
        case 48:  Vi = Vlp + Vbp; break;
        case 49:  Vi = v1 + Vlp + Vbp; break;
        case 50:  Vi = v2 + Vlp + Vbp; break;
        case 51:  Vi = v1 + v2 + Vlp + Vbp; break;
        case 52:  Vi = v3 + Vlp + Vbp; break;
        case 53:  Vi = v1 + v3 + Vlp + Vbp; break;
        case 54:  Vi = v2 + v3 + Vlp + Vbp; break;
        case 55:  Vi = v1 + v2 + v3 + Vlp + Vbp; break;
        case 56:  Vi = ve + Vlp + Vbp; break;
        case 57:  Vi = v1 + ve + Vlp + Vbp; break;
        case 58:  Vi = v2 + ve + Vlp + Vbp; break;
        case 59:  Vi = v1 + v2 + ve + Vlp + Vbp; break;
        case 60:  Vi = v3 + ve + Vlp + Vbp; break;
        case 61:  Vi = v1 + v3 + ve + Vlp + Vbp; break;
        case 62:  Vi = v2 + v3 + ve + Vlp + Vbp; break;
        case 63:  Vi = v1 + v2 + v3 + ve + Vlp + Vbp; break;
        case 64:  Vi = Vhp; break;
        case 65:  Vi = v1 + Vhp; break;
        case 66:  Vi = v2 + Vhp; break;
        case 67:  Vi = v1 + v2 + Vhp; break;
        case 68:  Vi = v3 + Vhp; break;
        case 69:  Vi = v1 + v3 + Vhp; break;
        case 70:  Vi = v2 + v3 + Vhp; break;
        case 71:  Vi = v1 + v2 + v3 + Vhp; break;
        case 72:  Vi = ve + Vhp; break;
        case 73:  Vi = v1 + ve + Vhp; break;
        case 74:  Vi = v2 + ve + Vhp; break;
        case 75:  Vi = v1 + v2 + ve + Vhp; break;
        case 76:  Vi = v3 + ve + Vhp; break;
        case 77:  Vi = v1 + v3 + ve + Vhp; break;
        case 78:  Vi = v2 + v3 + ve + Vhp; break;
        case 79:  Vi = v1 + v2 + v3 + ve + Vhp; break;
        case 80:  Vi = Vlp + Vhp; break;
        case 81:  Vi = v1 + Vlp + Vhp; break;
        case 82:  Vi = v2 + Vlp + Vhp; break;
        case 83:  Vi = v1 + v2 + Vlp + Vhp; break;
        case 84:  Vi = v3 + Vlp + Vhp; break;
        case 85:  Vi = v1 + v3 + Vlp + Vhp; break;
        case 86:  Vi = v2 + v3 + Vlp + Vhp; break;
        case 87:  Vi = v1 + v2 + v3 + Vlp + Vhp; break;
        case 88:  Vi = ve + Vlp + Vhp; break;
        case 89:  Vi = v1 + ve + Vlp + Vhp; break;
        case 90:  Vi = v2 + ve + Vlp + Vhp; break;
        case 91:  Vi = v1 + v2 + ve + Vlp + Vhp; break;
        case 92:  Vi = v3 + ve + Vlp + Vhp; break;
        case 93:  Vi = v1 + v3 + ve + Vlp + Vhp; break;
        case 94:  Vi = v2 + v3 + ve + Vlp + Vhp; break;
        case 95:  Vi = v1 + v2 + v3 + ve + Vlp + Vhp; break;
        case 96:  Vi = Vbp + Vhp; break;
        case 97:  Vi = v1 + Vbp + Vhp; break;
        case 98:  Vi = v2 + Vbp + Vhp; break;
        case 99:  Vi = v1 + v2 + Vbp + Vhp; break;
        case 100:  Vi = v3 + Vbp + Vhp; break;
        case 101:  Vi = v1 + v3 + Vbp + Vhp; break;
        case 102:  Vi = v2 + v3 + Vbp + Vhp; break;
        case 103:  Vi = v1 + v2 + v3 + Vbp + Vhp; break;
        case 104:  Vi = ve + Vbp + Vhp; break;
        case 105:  Vi = v1 + ve + Vbp + Vhp; break;
        case 106:  Vi = v2 + ve + Vbp + Vhp; break;
        case 107:  Vi = v1 + v2 + ve + Vbp + Vhp; break;
        case 108:  Vi = v3 + ve + Vbp + Vhp; break;
        case 109:  Vi = v1 + v3 + ve + Vbp + Vhp; break;
        case 110:  Vi = v2 + v3 + ve + Vbp + Vhp; break;
        case 111:  Vi = v1 + v2 + v3 + ve + Vbp + Vhp; break;
        case 112:  Vi = Vlp + Vbp + Vhp; break;
        case 113:  Vi = v1 + Vlp + Vbp + Vhp; break;
        case 114:  Vi = v2 + Vlp + Vbp + Vhp; break;
        case 115:  Vi = v1 + v2 + Vlp + Vbp + Vhp; break;
        case 116:  Vi = v3 + Vlp + Vbp + Vhp; break;
        case 117:  Vi = v1 + v3 + Vlp + Vbp + Vhp; break;
        case 118:  Vi = v2 + v3 + Vlp + Vbp + Vhp; break;
        case 119:  Vi = v1 + v2 + v3 + Vlp + Vbp + Vhp; break;
        case 120:  Vi = ve + Vlp + Vbp + Vhp; break;
        case 121:  Vi = v1 + ve + Vlp + Vbp + Vhp; break;
        case 122:  Vi = v2 + ve + Vlp + Vbp + Vhp; break;
        case 123:  Vi = v1 + v2 + ve + Vlp + Vbp + Vhp; break;
        case 124:  Vi = v3 + ve + Vlp + Vbp + Vhp; break;
        case 125:  Vi = v1 + v3 + ve + Vlp + Vbp + Vhp; break;
        case 126:  Vi = v2 + v3 + ve + Vlp + Vbp + Vhp; break;
        case 127:  Vi = v1 + v2 + v3 + ve + Vlp + Vbp + Vhp; break;
    }

    int tmp = Vi * (int) vol >> 4;
    if (tmp < -32768) tmp = -32768;
    if (tmp > 32767) tmp = 32767;
    return (short) tmp;
}

auto Sid::Filter::updateQ() -> void {
    static const int _1024_div_Q_table[] = {
        1448, 1328, 1218, 1117, 1024, 939, 861, 790,
        724, 664, 609, 558, 512, 470, 431, 395
    };

    _1024_div_Q = _1024_div_Q_table[res];
}

}

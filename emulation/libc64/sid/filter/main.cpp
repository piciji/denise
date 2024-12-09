
//  This code is a modification of the reSID engine in VICE
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

#include "../sid.h"
#include "build.cpp"
#include "opamp.cpp"
#include "separateInput.cpp"

namespace LIBC64 {

auto Sid::Filter::clock(int voice1, int voice2, int voice3) -> void {
    Calculated& ca = calculated[ type ];
    // Skalierung: 20 bit * 14 bit = 34 / 18 = 16 bit
    v1 = (voice1 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
    v2 = (voice2 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
    v3 = (voice3 * ca.voiceScaleS14 >> 18) + ca.voiceDC;

    int Vi = 0;
    int offset = 0;

	switch ( sum & 0xf ) {
		case 0x0: Vi = 0;					offset = 0;	break;
		case 0x1: Vi = v1;					offset = 2 << 16; break;
		case 0x2: Vi = v2;					offset = 2 << 16; break;
		case 0x3: Vi = v2 + v1;				offset = 5 << 16; break;
		case 0x4: Vi = v3;					offset = 2 << 16; break;
		case 0x5: Vi = v3 + v1;				offset = 5 << 16; break;
		case 0x6: Vi = v3 + v2;				offset = 5 << 16; break;
		case 0x7: Vi = v3 + v2 + v1;		offset = 9 << 16; break;
		case 0x8: Vi = ve;					offset = 2 << 16; break;
		case 0x9: Vi = ve + v1;				offset = 5 << 16; break;
		case 0xa: Vi = ve + v2;				offset = 5 << 16; break;
		case 0xb: Vi = ve + v2 + v1;		offset = 9 << 16; break;
		case 0xc: Vi = ve + v3;				offset = 5 << 16; break;
		case 0xd: Vi = ve + v3 + v1;		offset = 9 << 16; break;
		case 0xe: Vi = ve + v3 + v2;		offset = 9 << 16; break;
		case 0xf: Vi = ve + v3 + v2 + v1;	offset = 14 << 16; break;
	}

	// Die Berechnung der Filter geschieht rückwärts in der
	// Reihenfolge: tief pass, band pass, hoch pass
	// Es liegen Kreis Abhängigkeiten vor. Die Ausgänge von
	// band pass und tief pass sind wiederum Eingänge für
	// hoch pass.

	if  ( type == Type::MOS_6581 ) {
        // Eingang: band pass, Ausgang: tief pass
        Vlp = solveIntegrate6581( Vbp, Vlp_x, Vlp_vc, ca );
        // Eingang: hoch pass, Ausgang: band pass
		Vbp = solveIntegrate6581( Vhp, Vbp_x, Vbp_vc, ca );

	} else { // 8580
		Vlp = solveIntegrate8580( Vbp, Vlp_x, Vlp_vc, ca );
		Vbp = solveIntegrate8580( Vhp, Vbp_x, Vbp_vc, ca );
    }
	// Eingang: v1 + v2 + v3 + ve + tief pass + band pass, Ausgang: hoch pass
    Vhp = ca.summer[offset + ca.resonance[res][Vbp] + Vlp + Vi];
}

auto Sid::Filter::input(short sample) -> void {
    // Scale to three times the peak-to-peak for one voice and add the op-amp
    // "zero" DC level.
    // NB! Adding the op-amp "zero" DC level is a (wildly inaccurate)
    // approximation of feeding the input through an AC coupling capacitor.
    // This could be implemented as a separate filter circuit, however the
    // primary use of the emulator is not to process external signals.
    // The upside is that the MOS8580 "digi boost" works without a separate (DC)
    // input interface.
    Calculated& ca = calculated[ this->type ];

    ve = (sample * ca.voiceScaleS14 * 3 >> 14) + ca.mixer[0];
}

auto Sid::Filter::output() -> short {
	// Der Mixer hat 7 Eingänge, 4 voices und 3 Filter.

	// Mittels folgendem php Skript sind die 'case' labels generiert.
	// Der C Preprocessor ist hier wenig hilfreich, da er keine selbst
	// aus-rollenden Schleifen anbietet.

	 // <?php
	 // $sOut = "";
	 // $aInputs = ['v1', 'v2', 'v3', 've', 'Vlp', 'Vbp', 'Vhp'];
	 //
	 // for ($i = 0; $i <= 0x7f; $i++) {
	 //
	 //  $sOut .= "case " . $i . ": ";
	 //
	 //  $sVi = "";
	 //  $sVi1 = "";
	 //  $iSize = 1;
	 //  $iOffset = 0;
	 //  $iCount = 0;
	 //  $sSumFilt = "";
	 //  $sSumVoice = "";
	 //
	 //  for ($j = 0; $j < 7; $j++) {
	 //
	 //   if ($i & (1 << $j)) {
	 //    if ($j < 4)
	 //     $sSumVoice .= $aInputs[ $j ] . " + ";
	 //    else
	 //     $sSumFilt .= $aInputs[ $j ] . " + ";
	 //
	 //    $sVi1 .= $aInputs[ $j ] . " + ";
	 //    $iOffset += $iSize;
	 //    $iSize = ($iCount++ +1) << 16;
	 //   }
	 //  }
	 //
	 //  if (($sSumFilt == "") && ($sSumVoice == ""))
	 //   $sVi = "0;";
	 //  else if (($sSumFilt != "") && ($sSumVoice != ""))
	 //   $sVi = "(((" . substr($sSumFilt, 0, -3) . ") * _6581Gain) >> 12) + " . substr($sSumVoice, 0, -3) . ";";
	 //  else if (($sSumFilt == "") && ($sSumVoice != ""))
	 //   $sVi = substr($sSumVoice, 0, -3) . ";";
	 //  else
	 //   $sVi = "(((" . substr($sSumFilt, 0, -3) . ") * _6581Gain) >> 12);";
	 //
	 //  if ($sVi1 == "")
	 //   $sVi1 = "0;";
	 //  else
	 //   $sVi1 = substr($sVi1, 0, -3) . ";";
	 //
	 //  $sOut .= " if _t6581 Vi = " . $sVi . " else Vi = " . $sVi1 . " offset = ". $iOffset . "; break;\n";
	 // }
	 // echo $sOut;

	int Vi = 0;
	int offset = 0;

#define _t6581 (type == Type::MOS_6581)
#define _6581Gain (int)(0.93 * (1 << 12))

	switch( mix & 0x7f ) {
case 0:  if _t6581 Vi = 0; else Vi = 0; offset = 0; break;
case 1:  if _t6581 Vi = v1; else Vi = v1; offset = 1; break;
case 2:  if _t6581 Vi = v2; else Vi = v2; offset = 1; break;
case 3:  if _t6581 Vi = v1 + v2; else Vi = v1 + v2; offset = 65537; break;
case 4:  if _t6581 Vi = v3; else Vi = v3; offset = 1; break;
case 5:  if _t6581 Vi = v1 + v3; else Vi = v1 + v3; offset = 65537; break;
case 6:  if _t6581 Vi = v2 + v3; else Vi = v2 + v3; offset = 65537; break;
case 7:  if _t6581 Vi = v1 + v2 + v3; else Vi = v1 + v2 + v3; offset = 196609; break;
case 8:  if _t6581 Vi = ve; else Vi = ve; offset = 1; break;
case 9:  if _t6581 Vi = v1 + ve; else Vi = v1 + ve; offset = 65537; break;
case 10:  if _t6581 Vi = v2 + ve; else Vi = v2 + ve; offset = 65537; break;
case 11:  if _t6581 Vi = v1 + v2 + ve; else Vi = v1 + v2 + ve; offset = 196609; break;
case 12:  if _t6581 Vi = v3 + ve; else Vi = v3 + ve; offset = 65537; break;
case 13:  if _t6581 Vi = v1 + v3 + ve; else Vi = v1 + v3 + ve; offset = 196609; break;
case 14:  if _t6581 Vi = v2 + v3 + ve; else Vi = v2 + v3 + ve; offset = 196609; break;
case 15:  if _t6581 Vi = v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve; offset = 393217; break;
case 16:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12); else Vi = Vlp; offset = 1; break;
case 17:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vlp; offset = 65537; break;
case 18:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vlp; offset = 65537; break;
case 19:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vlp; offset = 196609; break;
case 20:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vlp; offset = 65537; break;
case 21:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vlp; offset = 196609; break;
case 22:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vlp; offset = 196609; break;
case 23:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vlp; offset = 393217; break;
case 24:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + ve; else Vi = ve + Vlp; offset = 65537; break;
case 25:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vlp; offset = 196609; break;
case 26:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vlp; offset = 196609; break;
case 27:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vlp; offset = 393217; break;
case 28:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vlp; offset = 196609; break;
case 29:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vlp; offset = 393217; break;
case 30:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vlp; offset = 393217; break;
case 31:  if _t6581 Vi = (((Vlp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vlp; offset = 655361; break;
case 32:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12); else Vi = Vbp; offset = 1; break;
case 33:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vbp; offset = 65537; break;
case 34:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vbp; offset = 65537; break;
case 35:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vbp; offset = 196609; break;
case 36:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vbp; offset = 65537; break;
case 37:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vbp; offset = 196609; break;
case 38:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vbp; offset = 196609; break;
case 39:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vbp; offset = 393217; break;
case 40:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + ve; else Vi = ve + Vbp; offset = 65537; break;
case 41:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vbp; offset = 196609; break;
case 42:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vbp; offset = 196609; break;
case 43:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vbp; offset = 393217; break;
case 44:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vbp; offset = 196609; break;
case 45:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vbp; offset = 393217; break;
case 46:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vbp; offset = 393217; break;
case 47:  if _t6581 Vi = (((Vbp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vbp; offset = 655361; break;
case 48:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12); else Vi = Vlp + Vbp; offset = 65537; break;
case 49:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vlp + Vbp; offset = 196609; break;
case 50:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vlp + Vbp; offset = 196609; break;
case 51:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vlp + Vbp; offset = 393217; break;
case 52:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vlp + Vbp; offset = 196609; break;
case 53:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vlp + Vbp; offset = 393217; break;
case 54:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vlp + Vbp; offset = 393217; break;
case 55:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vlp + Vbp; offset = 655361; break;
case 56:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + ve; else Vi = ve + Vlp + Vbp; offset = 196609; break;
case 57:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vlp + Vbp; offset = 393217; break;
case 58:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vlp + Vbp; offset = 393217; break;
case 59:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vlp + Vbp; offset = 655361; break;
case 60:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vlp + Vbp; offset = 393217; break;
case 61:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vlp + Vbp; offset = 655361; break;
case 62:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vlp + Vbp; offset = 655361; break;
case 63:  if _t6581 Vi = (((Vlp + Vbp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vlp + Vbp; offset = 983041; break;
case 64:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12); else Vi = Vhp; offset = 1; break;
case 65:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vhp; offset = 65537; break;
case 66:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vhp; offset = 65537; break;
case 67:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vhp; offset = 196609; break;
case 68:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vhp; offset = 65537; break;
case 69:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vhp; offset = 196609; break;
case 70:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vhp; offset = 196609; break;
case 71:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vhp; offset = 393217; break;
case 72:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + ve; else Vi = ve + Vhp; offset = 65537; break;
case 73:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vhp; offset = 196609; break;
case 74:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vhp; offset = 196609; break;
case 75:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vhp; offset = 393217; break;
case 76:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vhp; offset = 196609; break;
case 77:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vhp; offset = 393217; break;
case 78:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vhp; offset = 393217; break;
case 79:  if _t6581 Vi = (((Vhp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vhp; offset = 655361; break;
case 80:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12); else Vi = Vlp + Vhp; offset = 65537; break;
case 81:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vlp + Vhp; offset = 196609; break;
case 82:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vlp + Vhp; offset = 196609; break;
case 83:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vlp + Vhp; offset = 393217; break;
case 84:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vlp + Vhp; offset = 196609; break;
case 85:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vlp + Vhp; offset = 393217; break;
case 86:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vlp + Vhp; offset = 393217; break;
case 87:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vlp + Vhp; offset = 655361; break;
case 88:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + ve; else Vi = ve + Vlp + Vhp; offset = 196609; break;
case 89:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vlp + Vhp; offset = 393217; break;
case 90:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vlp + Vhp; offset = 393217; break;
case 91:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vlp + Vhp; offset = 655361; break;
case 92:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vlp + Vhp; offset = 393217; break;
case 93:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vlp + Vhp; offset = 655361; break;
case 94:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vlp + Vhp; offset = 655361; break;
case 95:  if _t6581 Vi = (((Vlp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vlp + Vhp; offset = 983041; break;
case 96:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12); else Vi = Vbp + Vhp; offset = 65537; break;
case 97:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vbp + Vhp; offset = 196609; break;
case 98:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vbp + Vhp; offset = 196609; break;
case 99:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vbp + Vhp; offset = 393217; break;
case 100:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vbp + Vhp; offset = 196609; break;
case 101:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vbp + Vhp; offset = 393217; break;
case 102:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vbp + Vhp; offset = 393217; break;
case 103:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vbp + Vhp; offset = 655361; break;
case 104:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + ve; else Vi = ve + Vbp + Vhp; offset = 196609; break;
case 105:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vbp + Vhp; offset = 393217; break;
case 106:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vbp + Vhp; offset = 393217; break;
case 107:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vbp + Vhp; offset = 655361; break;
case 108:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vbp + Vhp; offset = 393217; break;
case 109:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vbp + Vhp; offset = 655361; break;
case 110:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vbp + Vhp; offset = 655361; break;
case 111:  if _t6581 Vi = (((Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vbp + Vhp; offset = 983041; break;
case 112:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12); else Vi = Vlp + Vbp + Vhp; offset = 196609; break;
case 113:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1; else Vi = v1 + Vlp + Vbp + Vhp; offset = 393217; break;
case 114:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v2; else Vi = v2 + Vlp + Vbp + Vhp; offset = 393217; break;
case 115:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2; else Vi = v1 + v2 + Vlp + Vbp + Vhp; offset = 655361; break;
case 116:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v3; else Vi = v3 + Vlp + Vbp + Vhp; offset = 393217; break;
case 117:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v3; else Vi = v1 + v3 + Vlp + Vbp + Vhp; offset = 655361; break;
case 118:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v2 + v3; else Vi = v2 + v3 + Vlp + Vbp + Vhp; offset = 655361; break;
case 119:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3; else Vi = v1 + v2 + v3 + Vlp + Vbp + Vhp; offset = 983041; break;
case 120:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + ve; else Vi = ve + Vlp + Vbp + Vhp; offset = 393217; break;
case 121:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + ve; else Vi = v1 + ve + Vlp + Vbp + Vhp; offset = 655361; break;
case 122:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v2 + ve; else Vi = v2 + ve + Vlp + Vbp + Vhp; offset = 655361; break;
case 123:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + ve; else Vi = v1 + v2 + ve + Vlp + Vbp + Vhp; offset = 983041; break;
case 124:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v3 + ve; else Vi = v3 + ve + Vlp + Vbp + Vhp; offset = 655361; break;
case 125:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v3 + ve; else Vi = v1 + v3 + ve + Vlp + Vbp + Vhp; offset = 983041; break;
case 126:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v2 + v3 + ve; else Vi = v2 + v3 + ve + Vlp + Vbp + Vhp; offset = 983041; break;
case 127:  if _t6581 Vi = (((Vlp + Vbp + Vhp) * _6581Gain) >> 12) + v1 + v2 + v3 + ve; else Vi = v1 + v2 + v3 + ve + Vlp + Vbp + Vhp; offset = 1376257; break;
    }

    Calculated& ca = calculated[ type ];
    return (short)(ca.gain[vol][ ca.mixer[offset + Vi] ] - (1 << 15) );
}

}

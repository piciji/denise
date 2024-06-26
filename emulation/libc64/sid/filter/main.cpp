
//  This code is a modification of the resid engine in VICE
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

// my english isn't good enough so following detailed explanations are in german only
// my motivation is to understand the mathematical background 
// therefore i have analyzed this code line by line

#include "../sid.h"

// Um den roten Pfaden nicht zu verlieren ist es am Sinnvolsten 'build.cpp' zuerst zu lesen.
// Und an den entsprechenden Stellen in die Berechnungen siehe 'opamp.cpp' zu wechseln.
// Ebenso die notwendigen physikalischen Grundvoraussetzungen inkl. Formeln befinden sich
// in 'opamp.cpp'.
// Die Schaltkreise für jeweils beide Sids befinden sich in 'circuit'.

// Diese Datei sollte als letztes gelesen werden, da hier die Resultate in der 
// Emulations Schleife verwendet werden.
//
#include "build.cpp"
#include "opamp.cpp"

namespace LIBC64 {

auto Sid::Filter::clock(int voice1, int voice2, int voice3) -> void {
    
	Calculated& ca = calculated[ this->type ];
	// Skalierung: 20 bit * 14 bit = 34 / 18 = 16 bit
    // Skalierungs Faktor und Nullpunkt Spannung Berechnung
    // sind in "build.cpp" beschrieben.
	v1 = (voice1 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
	v2 = (voice2 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
	v3 = (voice3 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
	
	int Vi = 0;
	int offset = 0;
	
	// Hier passiert die eigentliche emulation. Ausgangspunkt
	// sind die 4 Eingänge der voices. Beginnen wir mit den voices,
	// welche in den Filter geroutet werden. 
	
	// 4 bit 'sum' sagt aus welche voices gefiltert werden sollen.
	// Eingehende Spannungen werden addiert. Jede Summe dient als
	// Position innerhalb der bereits vorberechneten Werte.
	// Zur Positionsbestimmung benötigen wir noch den 'offset'
	// Zur Erinnerung: Wir haben für jede Anzahl voices, welche dem
	// Filter zugewiesen sind ein separates Feld von Werten.
	// Diese Felder sind hintereinander gespeichert. 'offset' definiert
    // den Beginn eines Feldes.
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
	// hoch pass, obwohl hoch pass als erstes von den
	// voice Eingangsspannungen erreicht wird.
	// Also berechnen wir hoch pass nachdem tief pass 
	// und band pass berechnet sind.
	
    // band pass und tief pass werden in jedem Zyklus berechnet.
    // Aufgrund der frei definierbaren Grenzfrequenz können wir unmöglich
    // alle Werte vorberechnen.
    
	if ( this->type == Type::MOS_6581 ) {
        // Eingang: band pass, Ausgang: tief pass
        Vlp = solveIntegrate6581( Vbp, Vlp_x, Vlp_vc, ca, false );
        // Eingang: hoch pass, Ausgang: band pass
		Vbp = solveIntegrate6581( Vhp, Vbp_x, Vbp_vc, ca, false );
        // Eingang: v1 + v2 + v3 + ve + tief pass + band pass, Ausgang: hoch pass
        // Der band pass Ausgang geht durch den Resonanz Verstärker.
		// Resonanz wird mittels 4 bit im Register gesetzt und
        // ermöglicht eine Verstärkung um die Grenz Frequenz.
		// Für jeden dieser 16 Register Werte in Abhängikeit
		// der Spannung am band pass Eingang haben wir die
		// Eingangsspannung am hoch pass Filter bereits
		// vorberechnet. Der Ausgang von tief pass gelangt
		// direkt als Eingang in den hochpass.
		// Dazu kommen die 4 voice Eingänge.
		// Für jede Anzahl Eingänge zum hochpass, es spielt dabei
		// keine Rolle welche Eingänge nur wieviel, gibt es ein
		// Feld vorberechneter Werte beginnend bei 'offset'.
		// Alle Eingänge addiert ergeben die Position in diesem
		// Feld.
		Vhp = ca.summer[offset + ca.gain[_8_div_Q][Vbp] + Vlp + Vi];        
                
	} else { // 8580        
		// Nach dem gleichen Prinzip erfolgt die Berechnung für den 8580
		Vlp = solveIntegrate8580( Vbp, Vlp_x, Vlp_vc, ca );		
		Vbp = solveIntegrate8580( Vhp, Vbp_x, Vbp_vc, ca );
		Vhp = ca.summer[offset + resonance[res][Vbp] + Vlp + Vi];        
    }
}

// siehe 'clock' mit dem Unterschied das keine vorberechneten Werte zum Einsatz kommen.
// Das Newton Raphson Verfahren wird zur Laufzeit berechnet. Jedoch werden im Gegensatz
// zur vorberechneten Version alle eingehenden Spannungen als eigenständige Transistoren
// berechnet.
inline auto Sid::Filter::clockMulti(int voice1, int voice2, int voice3) -> void {
	
	Calculated& ca = calculated[ this->type ];
	// Skalierung: 20 bit * 14 bit = 34 / 18 = 16 bit
    // Skalierungs Faktor und Nullpunkt Spannung Berechnung
    // sind in "build.cpp" beschrieben.
	v1 = (voice1 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
	v2 = (voice2 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
	v3 = (voice3 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
    
    // Eingang 
	
	if ( this->type == Type::MOS_6581 ) {
        // Eingang: band pass, Ausgang: tief pass
        Vlp = solveIntegrate6581( Vbp, Vlp_x, Vlp_vc, ca, true );
        // Eingang: hoch pass, Ausgang: band pass
		Vbp = solveIntegrate6581( Vhp, Vbp_x, Vbp_vc, ca, true );

		VbpRes = ca.gain[_8_div_Q][Vbp];
        
	} else { // 8580
		// Nach dem gleichen Prinzip erfolgt die Berechnung für den 8580
		Vlp = solveIntegrate8580( Vbp, Vlp_x, Vlp_vc, ca );		
		Vbp = solveIntegrate8580( Vhp, Vbp_x, Vbp_vc, ca );
		
		VbpRes = resonance[res][Vbp];
	}
	
	int c = 0;
	
	for(int* vi : multiSum )		
        // n * (Vddt - vi)^2
        // n = 1 für filter
		c += ca.calcC[ *vi ];

	Vhp = solveOpampMulti( ca.opamp, multiSum.size() << 7, c, ca.nrXFilter, ca );
}

auto Sid::Filter::input(short sample) -> void {
    // Der Sid bietet die Möglichkeit eine 4. voice 've' von extern
    // einzuschleusen und diese mit den anderen zu mischen.
    // Im Gegensatz zu den 20 bit breiten eingehenden voices 1 - 3
    // ist die externe voice 16 bit breit.
    
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
// Nachdem die Filter Ausgänge berechnet sind, wenden wir uns dem
// Mixer zu.    
// Der Mixer hat 7 Eingänge, 4 voices und 3 Filter.
// Das ergiebt 128 Möglichkeiten für die Kombination aus aktiven Eingängen.
// Aus Effizienz Gründen sollten so wenig wie möglich Berechnungen
// während der emulation durchlaufen.    
// Mittels folgendem php Skript sind die 'case' labels generiert.
// Der C Preprocessor ist hier wenig hilfreich, da er keine selbst
// aus-rollenden Schleifen anbietet.
//         
//    $sOut = "";
//    $aInputs = ['v1', 'v2', 'v3', 've', 'Vlp', 'Vbp', 'Vhp'];
//
//    for ($i = 0; $i <= 0x7f; $i++) {
//
//        $sOut .= "case " . $i . ": ";
//
//        $sVi = "";
//        $iSize = 1;
//        $iOffset = 0;
//        $iCount = 0;
//
//        for ($j = 0; $j < 7; $j++) {
//
//            if ($i & (1 << $j)) {
//                $sVi .= $aInputs[ $j ] . " + ";
//
//                $iOffset += $iSize;
//                $iSize = ($iCount++ +1) << 16;
//            }
//        }
//
//        if ($sVi == "")
//            $sVi = "0;";
//        else
//            $sVi = substr($sVi, 0, -3) . ";";
//
//        $sOut .= " Vi = " . $sVi . " offset = ". $iOffset . "; break;<br/>";
//    }
//    echo $sOut;	

    Calculated& ca = calculated[ this->type ];
    
    // Summe aktiver Eingänge
	int Vi = 0;
    // Wir haben für jede Anzahl aktiver Eingänge, welche dem
	// Mixer zugewiesen sind ein separates Feld von Werten.
	// Diese Felder sind hintereinander gespeichert. 'offset'
    // definiert dessen Startpunkt.
	int offset = 0;
	
	switch( mix & 0x7f ) {
        case 0: Vi = 0;
            offset = 0;
            break;
        case 1: Vi = v1;
            offset = 1;
            break;
        case 2: Vi = v2;
            offset = 1;
            break;
        case 3: Vi = v1 + v2;
            offset = 65537;
            break;
        case 4: Vi = v3;
            offset = 1;
            break;
        case 5: Vi = v1 + v3;
            offset = 65537;
            break;
        case 6: Vi = v2 + v3;
            offset = 65537;
            break;
        case 7: Vi = v1 + v2 + v3;
            offset = 196609;
            break;
        case 8: Vi = ve;
            offset = 1;
            break;
        case 9: Vi = v1 + ve;
            offset = 65537;
            break;
        case 10: Vi = v2 + ve;
            offset = 65537;
            break;
        case 11: Vi = v1 + v2 + ve;
            offset = 196609;
            break;
        case 12: Vi = v3 + ve;
            offset = 65537;
            break;
        case 13: Vi = v1 + v3 + ve;
            offset = 196609;
            break;
        case 14: Vi = v2 + v3 + ve;
            offset = 196609;
            break;
        case 15: Vi = v1 + v2 + v3 + ve;
            offset = 393217;
            break;
        case 16: Vi = Vlp;
            offset = 1;
            break;
        case 17: Vi = v1 + Vlp;
            offset = 65537;
            break;
        case 18: Vi = v2 + Vlp;
            offset = 65537;
            break;
        case 19: Vi = v1 + v2 + Vlp;
            offset = 196609;
            break;
        case 20: Vi = v3 + Vlp;
            offset = 65537;
            break;
        case 21: Vi = v1 + v3 + Vlp;
            offset = 196609;
            break;
        case 22: Vi = v2 + v3 + Vlp;
            offset = 196609;
            break;
        case 23: Vi = v1 + v2 + v3 + Vlp;
            offset = 393217;
            break;
        case 24: Vi = ve + Vlp;
            offset = 65537;
            break;
        case 25: Vi = v1 + ve + Vlp;
            offset = 196609;
            break;
        case 26: Vi = v2 + ve + Vlp;
            offset = 196609;
            break;
        case 27: Vi = v1 + v2 + ve + Vlp;
            offset = 393217;
            break;
        case 28: Vi = v3 + ve + Vlp;
            offset = 196609;
            break;
        case 29: Vi = v1 + v3 + ve + Vlp;
            offset = 393217;
            break;
        case 30: Vi = v2 + v3 + ve + Vlp;
            offset = 393217;
            break;
        case 31: Vi = v1 + v2 + v3 + ve + Vlp;
            offset = 655361;
            break;
        case 32: Vi = Vbp;
            offset = 1;
            break;
        case 33: Vi = v1 + Vbp;
            offset = 65537;
            break;
        case 34: Vi = v2 + Vbp;
            offset = 65537;
            break;
        case 35: Vi = v1 + v2 + Vbp;
            offset = 196609;
            break;
        case 36: Vi = v3 + Vbp;
            offset = 65537;
            break;
        case 37: Vi = v1 + v3 + Vbp;
            offset = 196609;
            break;
        case 38: Vi = v2 + v3 + Vbp;
            offset = 196609;
            break;
        case 39: Vi = v1 + v2 + v3 + Vbp;
            offset = 393217;
            break;
        case 40: Vi = ve + Vbp;
            offset = 65537;
            break;
        case 41: Vi = v1 + ve + Vbp;
            offset = 196609;
            break;
        case 42: Vi = v2 + ve + Vbp;
            offset = 196609;
            break;
        case 43: Vi = v1 + v2 + ve + Vbp;
            offset = 393217;
            break;
        case 44: Vi = v3 + ve + Vbp;
            offset = 196609;
            break;
        case 45: Vi = v1 + v3 + ve + Vbp;
            offset = 393217;
            break;
        case 46: Vi = v2 + v3 + ve + Vbp;
            offset = 393217;
            break;
        case 47: Vi = v1 + v2 + v3 + ve + Vbp;
            offset = 655361;
            break;
        case 48: Vi = Vlp + Vbp;
            offset = 65537;
            break;
        case 49: Vi = v1 + Vlp + Vbp;
            offset = 196609;
            break;
        case 50: Vi = v2 + Vlp + Vbp;
            offset = 196609;
            break;
        case 51: Vi = v1 + v2 + Vlp + Vbp;
            offset = 393217;
            break;
        case 52: Vi = v3 + Vlp + Vbp;
            offset = 196609;
            break;
        case 53: Vi = v1 + v3 + Vlp + Vbp;
            offset = 393217;
            break;
        case 54: Vi = v2 + v3 + Vlp + Vbp;
            offset = 393217;
            break;
        case 55: Vi = v1 + v2 + v3 + Vlp + Vbp;
            offset = 655361;
            break;
        case 56: Vi = ve + Vlp + Vbp;
            offset = 196609;
            break;
        case 57: Vi = v1 + ve + Vlp + Vbp;
            offset = 393217;
            break;
        case 58: Vi = v2 + ve + Vlp + Vbp;
            offset = 393217;
            break;
        case 59: Vi = v1 + v2 + ve + Vlp + Vbp;
            offset = 655361;
            break;
        case 60: Vi = v3 + ve + Vlp + Vbp;
            offset = 393217;
            break;
        case 61: Vi = v1 + v3 + ve + Vlp + Vbp;
            offset = 655361;
            break;
        case 62: Vi = v2 + v3 + ve + Vlp + Vbp;
            offset = 655361;
            break;
        case 63: Vi = v1 + v2 + v3 + ve + Vlp + Vbp;
            offset = 983041;
            break;
        case 64: Vi = Vhp;
            offset = 1;
            break;
        case 65: Vi = v1 + Vhp;
            offset = 65537;
            break;
        case 66: Vi = v2 + Vhp;
            offset = 65537;
            break;
        case 67: Vi = v1 + v2 + Vhp;
            offset = 196609;
            break;
        case 68: Vi = v3 + Vhp;
            offset = 65537;
            break;
        case 69: Vi = v1 + v3 + Vhp;
            offset = 196609;
            break;
        case 70: Vi = v2 + v3 + Vhp;
            offset = 196609;
            break;
        case 71: Vi = v1 + v2 + v3 + Vhp;
            offset = 393217;
            break;
        case 72: Vi = ve + Vhp;
            offset = 65537;
            break;
        case 73: Vi = v1 + ve + Vhp;
            offset = 196609;
            break;
        case 74: Vi = v2 + ve + Vhp;
            offset = 196609;
            break;
        case 75: Vi = v1 + v2 + ve + Vhp;
            offset = 393217;
            break;
        case 76: Vi = v3 + ve + Vhp;
            offset = 196609;
            break;
        case 77: Vi = v1 + v3 + ve + Vhp;
            offset = 393217;
            break;
        case 78: Vi = v2 + v3 + ve + Vhp;
            offset = 393217;
            break;
        case 79: Vi = v1 + v2 + v3 + ve + Vhp;
            offset = 655361;
            break;
        case 80: Vi = Vlp + Vhp;
            offset = 65537;
            break;
        case 81: Vi = v1 + Vlp + Vhp;
            offset = 196609;
            break;
        case 82: Vi = v2 + Vlp + Vhp;
            offset = 196609;
            break;
        case 83: Vi = v1 + v2 + Vlp + Vhp;
            offset = 393217;
            break;
        case 84: Vi = v3 + Vlp + Vhp;
            offset = 196609;
            break;
        case 85: Vi = v1 + v3 + Vlp + Vhp;
            offset = 393217;
            break;
        case 86: Vi = v2 + v3 + Vlp + Vhp;
            offset = 393217;
            break;
        case 87: Vi = v1 + v2 + v3 + Vlp + Vhp;
            offset = 655361;
            break;
        case 88: Vi = ve + Vlp + Vhp;
            offset = 196609;
            break;
        case 89: Vi = v1 + ve + Vlp + Vhp;
            offset = 393217;
            break;
        case 90: Vi = v2 + ve + Vlp + Vhp;
            offset = 393217;
            break;
        case 91: Vi = v1 + v2 + ve + Vlp + Vhp;
            offset = 655361;
            break;
        case 92: Vi = v3 + ve + Vlp + Vhp;
            offset = 393217;
            break;
        case 93: Vi = v1 + v3 + ve + Vlp + Vhp;
            offset = 655361;
            break;
        case 94: Vi = v2 + v3 + ve + Vlp + Vhp;
            offset = 655361;
            break;
        case 95: Vi = v1 + v2 + v3 + ve + Vlp + Vhp;
            offset = 983041;
            break;
        case 96: Vi = Vbp + Vhp;
            offset = 65537;
            break;
        case 97: Vi = v1 + Vbp + Vhp;
            offset = 196609;
            break;
        case 98: Vi = v2 + Vbp + Vhp;
            offset = 196609;
            break;
        case 99: Vi = v1 + v2 + Vbp + Vhp;
            offset = 393217;
            break;
        case 100: Vi = v3 + Vbp + Vhp;
            offset = 196609;
            break;
        case 101: Vi = v1 + v3 + Vbp + Vhp;
            offset = 393217;
            break;
        case 102: Vi = v2 + v3 + Vbp + Vhp;
            offset = 393217;
            break;
        case 103: Vi = v1 + v2 + v3 + Vbp + Vhp;
            offset = 655361;
            break;
        case 104: Vi = ve + Vbp + Vhp;
            offset = 196609;
            break;
        case 105: Vi = v1 + ve + Vbp + Vhp;
            offset = 393217;
            break;
        case 106: Vi = v2 + ve + Vbp + Vhp;
            offset = 393217;
            break;
        case 107: Vi = v1 + v2 + ve + Vbp + Vhp;
            offset = 655361;
            break;
        case 108: Vi = v3 + ve + Vbp + Vhp;
            offset = 393217;
            break;
        case 109: Vi = v1 + v3 + ve + Vbp + Vhp;
            offset = 655361;
            break;
        case 110: Vi = v2 + v3 + ve + Vbp + Vhp;
            offset = 655361;
            break;
        case 111: Vi = v1 + v2 + v3 + ve + Vbp + Vhp;
            offset = 983041;
            break;
        case 112: Vi = Vlp + Vbp + Vhp;
            offset = 196609;
            break;
        case 113: Vi = v1 + Vlp + Vbp + Vhp;
            offset = 393217;
            break;
        case 114: Vi = v2 + Vlp + Vbp + Vhp;
            offset = 393217;
            break;
        case 115: Vi = v1 + v2 + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 116: Vi = v3 + Vlp + Vbp + Vhp;
            offset = 393217;
            break;
        case 117: Vi = v1 + v3 + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 118: Vi = v2 + v3 + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 119: Vi = v1 + v2 + v3 + Vlp + Vbp + Vhp;
            offset = 983041;
            break;
        case 120: Vi = ve + Vlp + Vbp + Vhp;
            offset = 393217;
            break;
        case 121: Vi = v1 + ve + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 122: Vi = v2 + ve + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 123: Vi = v1 + v2 + ve + Vlp + Vbp + Vhp;
            offset = 983041;
            break;
        case 124: Vi = v3 + ve + Vlp + Vbp + Vhp;
            offset = 655361;
            break;
        case 125: Vi = v1 + v3 + ve + Vlp + Vbp + Vhp;
            offset = 983041;
            break;
        case 126: Vi = v2 + v3 + ve + Vlp + Vbp + Vhp;
            offset = 983041;
            break;
        case 127: Vi = v1 + v2 + v3 + ve + Vlp + Vbp + Vhp;
            offset = 1376257;
            break;
    }
	
    // Wir erhalten den vorberechneten Wert dieses Mixes.
    // Abschließend wird die Amplitude der resultierenden Spannung
    // angepasst. Ebenso dafür haben wir in Abhängigkeit der Register
    // Einstellung und der Eingangs Spannung vorberechnete Werte.
    // Das Gesamt Ergebnis ist 16 bit 'unsigned', welches wir in
    // 15 bit 'signed' überführen. Die erste Hälfte des Wertebereiches
    // bilden die negativen Werte. 
    // 1 << 15 enspricht dem halben Wertebereich

    return (short)(ca.gain[vol][ ca.mixer[offset + Vi] ] - (1 << 15) );
}

// siehe 'output' mit dem Unterschied das keine vorberechneten Werte zum Einsatz kommen.
// Das Newton Raphson Verfahren wird zur Laufzeit berechnet. Jedoch werden im Gegensatz
// zur vorberechneten Version alle eingehenden Spannungen als eigenständige Transistoren
// berechnet.
auto Sid::Filter::outputMulti() -> short {

	Calculated& ca = calculated[ this->type ];
	
	int c = 0;
	
	for(int* vi : multiMix )
		// n = 8/6 , siehe Erklärung zur vorberechneten Version in 'build.cpp'
		c += ca.calcC[ *vi ] * 8 / 6;
	
	// Um die Suchzeit des Newton Raphson Verfahrens zu reduzieren, verwenden wir das letzte X
	// als Startpunkt der Suche des nächsten X.
	return (short)(ca.gain[vol][ solveOpampMulti( ca.opamp, (multiMix.size() << 7) * 8/6, c, ca.nrXMixer, ca ) ] - (1 << 15) );
}

auto Sid::Filter::multiPrecalculate() -> void {
    // Für die akurate emulation gruppieren wir Zeiger aller benötigten Spannungen
    // jeweils für den Filter bzw. Mixer.
    
    multiSum.clear();
    // Tiefpass und Bandpass sind immmer eingehend.
	multiSum.push_back( VlpP );
	multiSum.push_back( VbpResP );
		
	switch ( sum & 0xf ) {
		case 0x0:                                                                                                   break;
		case 0x1: multiSum.push_back( v1P );                                                                           break;
		case 0x2: multiSum.push_back( v2P );                                                                           break;
		case 0x3: multiSum.push_back( v1P ); multiSum.push_back( v2P );                                                   break;
		case 0x4: multiSum.push_back( v3P );                                                                           break;
		case 0x5: multiSum.push_back( v3P ); multiSum.push_back( v1P );                                                   break;
		case 0x6: multiSum.push_back( v3P ); multiSum.push_back( v2P );                                                   break;
		case 0x7: multiSum.push_back( v1P ); multiSum.push_back( v2P ); multiSum.push_back( v3P );                           break;
		case 0x8: multiSum.push_back( veP );                                                                           break;
		case 0x9: multiSum.push_back( veP ); multiSum.push_back( v1P );                                                   break;
		case 0xa: multiSum.push_back( veP ); multiSum.push_back( v2P );                                                   break;
		case 0xb: multiSum.push_back( veP ); multiSum.push_back( v1P ); multiSum.push_back( v2P );                           break;
		case 0xc: multiSum.push_back( veP ); multiSum.push_back( v3P );                                                   break;
		case 0xd: multiSum.push_back( veP ); multiSum.push_back( v3P ); multiSum.push_back( v1P );                           break;
		case 0xe: multiSum.push_back( veP ); multiSum.push_back( v3P ); multiSum.push_back( v2P );                           break;
		case 0xf: multiSum.push_back( veP ); multiSum.push_back( v3P ); multiSum.push_back( v2P ); multiSum.push_back( v1P );   break;
	}
    
// Die cases für den mixer sind mit folgendem php Skript erstellt.    
//    <?php
//
//        $sOut = "";
//        $aInputs = ['v1P', 'v2P', 'v3P', 'veP', 'VlpP', 'VbpP', 'VhpP'];
//
//        for ($i = 0; $i <= 0x7f; $i++) {
//
//            $sOut . = "case " . $i . ": ";
//
//            $sVi = "";
//
//            for ($j = 0; $j < 7; $j++) {
//
//                if ($i & (1 << $j))
//                    $sVi . = "multiMix.push_back( ".$aInputs[ $j ]." );\n";
//            }
//
//            $sOut . = $sVi . " break;\n";
//        }
//        echo $sOut;
    
    multiMix.clear();
    
    switch( mix & 0x7f ) {
        case 0:  break;
        case 1: multiMix.push_back( v1P );
         break;
        case 2: multiMix.push_back( v2P );
         break;
        case 3: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
         break;
        case 4: multiMix.push_back( v3P );
         break;
        case 5: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
         break;
        case 6: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
         break;
        case 7: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
         break;
        case 8: multiMix.push_back( veP );
         break;
        case 9: multiMix.push_back( v1P );
        multiMix.push_back( veP );
         break;
        case 10: multiMix.push_back( v2P );
        multiMix.push_back( veP );
         break;
        case 11: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
         break;
        case 12: multiMix.push_back( v3P );
        multiMix.push_back( veP );
         break;
        case 13: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
         break;
        case 14: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
         break;
        case 15: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
         break;
        case 16: multiMix.push_back( VlpP );
         break;
        case 17: multiMix.push_back( v1P );
        multiMix.push_back( VlpP );
         break;
        case 18: multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
         break;
        case 19: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
         break;
        case 20: multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
         break;
        case 21: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
         break;
        case 22: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
         break;
        case 23: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
         break;
        case 24: multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 25: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 26: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 27: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 28: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 29: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 30: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 31: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
         break;
        case 32: multiMix.push_back( VbpP );
         break;
        case 33: multiMix.push_back( v1P );
        multiMix.push_back( VbpP );
         break;
        case 34: multiMix.push_back( v2P );
        multiMix.push_back( VbpP );
         break;
        case 35: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VbpP );
         break;
        case 36: multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
         break;
        case 37: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
         break;
        case 38: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
         break;
        case 39: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
         break;
        case 40: multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 41: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 42: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 43: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 44: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 45: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 46: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 47: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
         break;
        case 48: multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 49: multiMix.push_back( v1P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 50: multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 51: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 52: multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 53: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 54: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 55: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 56: multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 57: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 58: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 59: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 60: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 61: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 62: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 63: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
         break;
        case 64: multiMix.push_back( VhpP );
         break;
        case 65: multiMix.push_back( v1P );
        multiMix.push_back( VhpP );
         break;
        case 66: multiMix.push_back( v2P );
        multiMix.push_back( VhpP );
         break;
        case 67: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VhpP );
         break;
        case 68: multiMix.push_back( v3P );
        multiMix.push_back( VhpP );
         break;
        case 69: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VhpP );
         break;
        case 70: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VhpP );
         break;
        case 71: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VhpP );
         break;
        case 72: multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 73: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 74: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 75: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 76: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 77: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 78: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 79: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VhpP );
         break;
        case 80: multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 81: multiMix.push_back( v1P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 82: multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 83: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 84: multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 85: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 86: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 87: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 88: multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 89: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 90: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 91: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 92: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 93: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 94: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 95: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VhpP );
         break;
        case 96: multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 97: multiMix.push_back( v1P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 98: multiMix.push_back( v2P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 99: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 100: multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 101: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 102: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 103: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 104: multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 105: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 106: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 107: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 108: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 109: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 110: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 111: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 112: multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 113: multiMix.push_back( v1P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 114: multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 115: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 116: multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 117: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 118: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 119: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 120: multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 121: multiMix.push_back( v1P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 122: multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 123: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 124: multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 125: multiMix.push_back( v1P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 126: multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;
        case 127: multiMix.push_back( v1P );
        multiMix.push_back( v2P );
        multiMix.push_back( v3P );
        multiMix.push_back( veP );
        multiMix.push_back( VlpP );
        multiMix.push_back( VbpP );
        multiMix.push_back( VhpP );
         break;

    }
}

}
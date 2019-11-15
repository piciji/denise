
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

#include "../sid.h"

namespace LIBC64 {
    
Sid::ExternalFilter::ExternalFilter() {
    // Zwischen Ausgang am Sid und Ausgang am C64 findet noch eine weitere Filterung statt.
    // Dabei werden nur Frequenzen zwischen 16 Hz u. 16 kHz durchgelassen.
    // Mittels 2 RC Gliedern findet zuerst die Tiefpass im Anschluß die Hochpass Filterung statt.
	// Ein RC Glied besteht aus einem Widerstand und einem Kondensator.
	// Ist der Kondensator parallel geschalten, sprechen wir von einem Tiefpass Filter.
	// Ist der Widerstand parallel geschalten sprechen wir von einem Hochpass Filter.
	// Die folgenden Formeln betrachten den Spannungsabfall am Kondensator für beide Filter.
	// Bei einem Hochpass Filter muss jedoch der Spannungsabfall am Widerstand betrachtet werden.
    // Durch einen Trick kännen wir mit 2 Tiefpass Filtern rechnen.
	// Zieht man die Ausgangsspannung des 2. Filters von der des 1. ab
	// wirkt der 2. Tiefpass Filter wie ein Hochpass Filter.
    // Beispiel:
    // Liegt die Frequenz der Spannung unter 16 Hz wird sie von beiden Tiefpass Filtern durchgelassen.
    // Durch die Subtraktion beider Spannungen hebt diese sich annährend auf, wird also blokiert.
	// Es handelt sich um 2 'STC' Netzwerke, 'single time constant'.
	// Die Filterung wird dabei für jede Mikro Sekunde betrachtet und nicht für eine Periode.
	// Das delta der Spannungen innerhalb einer Mikro Sekunde gibt Aufschluß über die Frequenz.
    
	// Ein Tiefpass Filter senkt die Amplitude (Max Spannung) von hohen Frequenzen
	// (schnelle Spannungswechsel) ab einer bestimmten Grenzfrequenz. Ein Hochpass Filter senkt
	// die Amplitude von tiefen Tönen (langsame Spannungswechsel) ab einer bestimmten Grenzfrequenz.
	// Die Grenzfrequenz ergibt sich aus 1 / ( R * C )
        
    // Low-pass:  R = 10 kOhm, C = 1000 pF; w0l = 1/RC = 1/(1e4*1e-9) = 100 000
    // High-pass: R =  1 kOhm, C =   10 uF; w0h = 1/RC = 1/(1e3*1e-5) =     100
    
    // 1 kOhm = 1000 Ohm
    // 1 uF = 1e-6 F
    // 1 pF = 1e-12 F
    
    // Die Spannung am Kondesator wird derart berechnet:
    // Uc = 1/RC * dt * Ue(t)
    // Wir berechnen die Konstanten vor: 1/RC * dt
    // dt = 1 Mikro Sekunde = 1e-6
    
    // 1. Tiefpass: skaliert: 2^7
    w0lp_1_s7 = int(100000 * 1.0e-6 * (1 << 7) + 0.5);
    
    // 2. Tiefpass: skaliert: 2^17
    w0hp_1_s17 = int(100 * 1.0e-6 * (1 << 17) + 0.5);    
}

auto Sid::ExternalFilter::reset() -> void {
    Vlp = 0;
    Vhp = 0;
}
    
inline auto Sid::ExternalFilter::clock( short Vi ) -> void {
    // Wir berechnen die deltas für beide Filter in jeder Mikro Sekunde
    // vi: Eingangsspannung (skaliert: 16 bit)
    // vlp: Ausgangsspannung Filter 1 (skaliert: 27 bit)
    // vhp: Ausgangsspannung Filter 2 (skaliert: 27 bit)
    
    // delta Skalierung: 27 bit
    int dVlp = w0lp_1_s7 * ((Vi << 11) - Vlp) >> 7;
    // vlp ist der Eingang für den 2. Filter
    int dVhp = w0hp_1_s17 * (Vlp - Vhp) >> 17;
    
    Vlp += dVlp;
    Vhp += dVhp;
}

inline auto Sid::ExternalFilter::output() -> short {
    
    // 2. Tiefpass Filter wirkt durch Subtraktion wie ein Hochpass Filter.
    // Skalierung: 27 - 11 = 16 bit
    int Vo = (Vlp - Vhp) >> 11;
    
    // Abschließend checken wir ob der Wertebereich überschritten wurde. Ist das der Fall
    // schneiden wir am größten bzw. kleinsten Wert ab.
    // Der Wertebereich umfasst 16 bit: 15 bit + Vorzeichenbit        
    return Emulator::sclamp(16, Vo);
}
    
}
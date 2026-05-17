
//  Modified by PiCiJi
//  ---------------------------------------------------------------------------
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

#include "../reSid.h"

namespace LIBC64 {

// [ NMOS FETS (Field Effect Transistor) ]
// Alle Widerstände sind sogenannte NMOS FETS und lassen sich nicht durch die bekannte Formel R = U / I berechnen.
    
//          |D
//        __|
//    G| |
// ____| |__
// Vdd      |
//          |S
    
// Grundsätzlich bestehen diese aus 3 Komponenten source(S), drain(D) und gate(G)
// Am 'gate' liegt eine Versorgungs-Spannung 'Vdd' an. Der interne Widerstand
// der 'drain' zu 'source' Strecke wird mit der Spannung am 'gate' gesteuert.
// 'drain' : Eingang
// 'source': Ausgang 
// FETS können in einem von 3 Modi arbeiten: 'subthreshold', 'triode' oder 'saturation'
// Für die Stromstärke der FETS gilt abhängig vom Modus:

// I = 0						            , Vgst < 0              (subthreshold) 
// I = ucox/2k * W/L * (2*Vgst - Vds)*Vds	, Vgst >= 0, Vds < Vgst  (triode)
// I = ucox/2k * W/L * Vgst^2				, Vgst >= 0, Vds >= Vgst  (saturation)

// Variablen:
// Vgst = Gate Spannung - Source Spannung - Threshold Spannung
// Vgdt = Gate Spannung - Drain Spannung - Threshold Spannung
// Vddt = Gate Spannung - Threshold Spannung = Effektiv Spannung
// Vds = Drain Spannung - Source Spannung
// W/L beschreibt die Geometrie und somit die Widerstandswirkung
// k' = u * cox = Elektonen Beweglichkeit * Oxid Kapazitanz pro Flächeneinheit
// k = Material Faktor. Es bezeichnet den Verlust von Kopplungs Effizienz zwischen
// 'gate' und Durchgang, verursacht durch das Material.
// n = ucox/2k * W/L ( Zusammenfassung zur Material Konstante 'n' )

// 'subthreshold' Modus
// Die Threshold Spannung ist die Mindestspannung, welche notwendig ist, das überhaupt ein Strom im Transistor fließt.
// Wenn (Vgate - Vsource) < Vthreshold fließt am Ausgang kein Strom. (I = 0)

// 'saturation' Modus
// Erreicht die Eingangs Spannung am 'drain' die Efektiv Spannung am 'gate'
// liegt der 'saturation' Modus vor. In diesem Modus kann man nicht
// mehr von einer Widerstands Wirkung sprechen.

// 'triode' Modus
// Treffen die Bedingungen der beiden anderen Modi nicht zu, spricht man vom 'triode' Modus.
// Als Widerstände verhalten sich die FETS nur in diesem Modus.
// Vds = Vgst - Vgdt
// Vds = Gate - Source - Threshold - ( Gate - Drain - Threshold )
// Vds = - Source + Drain = Drain - Source  
// Die obige 'triode' Formel lässt sich dann durch Umstellen wie folgt darstellen.  
// I = n * (2*Vgst - Vds) * Vds  // Ausgangsformel von oben
// I = n * ((2*Vgst - (Vgst - Vgdt)) * (Vgst - Vgdt)
// I = n * (Vgst + Vgdt) * (Vgst - Vgdt)
// I = n * (Vgst^2 - Vgdt^2)

// Für den Widerstand gilt:
// 'drain' zu 'source' Widerstand: Rds = 1 / ( ucox/2k * W/L * Vgst) = 1 / (n * Vgst)    
    
// In den meisten Fällen arbeiten die FETS im triode Modus. Es gibt jedoch
// einen FET im Schaltkreis des '6581' welcher in allen Modis arbeitet.
// Je nach Modus gelten die entsprechenden Formeln. Aus mathematischer Sicht ist das mühsig.
// Eine elegante Möglichkeit alle Modi fortlaufend in einer Formel zu beschreiben,
// bietet das EKV Modell. 

// I = Is * (if - ir)
// Is = 2 * u*Cox * Ut^2/k * W/L
// if = ln^2(1 + e^((k*(Vg - Vt) - Vs) / (2*Ut))
// ir = ln^2(1 + e^((k*(Vg - Vt) - Vd) / (2*Ut))

// ln : natürlicher Logarithmus
// e : eulersche Zahl
// Ut : thermodynamische Spannung
// u*Cox : Elektonen Beweglichkeit * Oxid Kapazitanz pro Flächeneinheit
// Vg : gate Spannung
// Vs : source Spannung
// Vd : drain Spannung
// Vt : threshold Spannung
// k : Material Faktor ( 'gate' Kopplungs Koeffizient )
  
// [Kondensatoren]
// Kondensatoren werden benötigt um bestimmte Frequenzen zu filtern.
// Die beiden Kondensatoren für band pass und tief pass Filterung
// befinden sich außerhalb des SID und sind über jeweis 2 pins herausgeführt.
// Für die Stromstärke im Kondensator gilt:
// I = C * dv / dt
// 'C' beschreibt die Kapazität des Kondesators
// 'dv' beschreibt die Spannungsänderung in Bezug auf die vergangene Zeit 'dt'
// 'dt' wrid auf eine Mikro Sekunde gesetzt, da die Kalkulation jeden Zyklus
// durchgeführt wird. Der SID läuft wie die CPU für PAL etwas unter einer Million
// und für NTSC etwas über einer Million Ticks pro Sekunde.
// Den genauen Wert zu verwenden, bringt in der Größenordnung keine Verbesserung.
    
// [ invertierende Operationsverstärker 'op-amp' ]
// op-amps dienen der Signal Verstärkung indem Spannungen generiert werden.
// Der invertierende op-amp senkt die Spannung ab einer bestimmten Eingangsspannung.    
// Damit ein Operationsverstärker funktionieren kann, benötigt dieser eine Versorgungsspannung.
// Für Operationsverstärker im SID gibt es Spannungs Messpunkte vor und nach der Verstärkung.
// Durch Interpolation werden die Lücken zwischen den Messpunkten aufgefüllt.
// Eine direkte Berechnung ist somit nicht nötig.
// Widerstände und Kondensatoren welche parallel zum op-amp betrieben werden,
// verwenden die gleiche Spannung wie der Verstärker selbst.
    
// [ Impedanzwandler ]    
// Jeder Verbraucher ändert die Stromverhältnisse und dies würde zu einer Änderung
// der Versorgungsspannung führen. Mittels eines Impedanzwandlers (auch
// Spannungsfolger genannt) kann dies verhindert werden, indem Ausgang und Eingang
// des op-amp über einen Widerstand direkt verbunden werden.
// Der Strom kommt somit vom Operationsverstärker und nicht von der davor liegenden Schaltung.
// Ohne Operations Verstärker würden sich alle in Reihe geschaltenen Verbraucher der 
// gesamten Schaltung die Gesamt Spannung aufteilen. Die Aufteilung der Spannung beginnt
// somit ab jedem op-amp mit der Ausgangsspannung des op-amps neu.
// Der parallele Widerstand wirkt als Strom-Spannungs Wandler, da er den eingespeisten Strom
// wieder abzieht.    
// Die Eingänge haben eine hohe Impedanz, die bewirkt, dass kein nennenswerter Strom
// in die oder aus den Eingängen fließt. 
// Die Stromstärke am Eingang des op-amp beträgt annährend 0.
    
// [ integrierender Operationsverstärker ]
// Hierbei werden Ausgang und Eingang des op-amp mittels eines Kondensators
// direkt miteinander verbunden. Dies dient zur Filterung von Frequenzen.
// Die Kondensatoren befinden sich außerhalb des SID.
// Der Widerstand davor ist steuerbar und dient dazu die Grenz Frequenz
// der Filter zu regulieren.
// Die Häufigkeit / Frequenz in der eine Spannung seine Polarisierung ändert,
// wird als hoher oder tiefer Ton wahr genommen.
// Der Kondensator mittels Widerständen sorgt dafür, das bestimmte Frequenzen 
// abgeschwächt werden. Die Amplitude also die Max Spannung wird reduziert.
// Die Frequenz an sich ändert sich nicht. Der Ton wird also leiser,
// je mehr dieser von der Grenzfrequenz abweicht.
// Bandpass Filter dämpfen alles was von der Grenz Frequenz abweicht, egal in welche
// Richtung.

// [ Parallelschaltung von Widerständen ]    
// Allgemein gilt für die Parallelschaltung von Verbrauchern, das an allen Verbrauchern
// die selbe Spannung anliegt, jedoch erhält jeder Verbraucher nur einen Teil der 
// gesamt zur Verfügung stehenden Stromstärke.
// Dies ist eine wichtige Grundlage, wodurch bekannte Spannungen am op-amp für
// parallel zu geschaltene Widerstände oder Kondensatoren ebenso verwendet werden können.
// Speziell für parallele Widerstände gilt für den Gesamt Widerstand:
// 1 / Rges = 1 / R1 + 1 / R2 + 1 / R3 ...    
// Sind nur 2 parallele Widerstände im Spiel lässt sich die Formel auch so darstellen:
// Rges = (R1 * R2) / (R1 + R2)
    
// [ Reihenschaltung von Widerständen ]
// Im Gegensatz zur Parallelschaltung bleibt hier die Stromstärke für alle Verbraucher
// konstant aber die Spannung ist für alle Verbraucher aufgeteilt.
// Für den Gesamt Widerstand gilt:
// Rges = R1 + R2 + R3 + ...    
    
// [ Kirchhoffsches Gesetz ]
// In einem Knotenpunkt ist die Summe der zufließenden Ströme gleich der Summe der
// abfließenden Ströme.

// ----------------------------------------------------------------------------------------
    
// [Berechnungen]
// Für folgende Modelle ist es das Ziel für eine beliebige Eingangsspannung 'vi' die Ausgangsspannung 'vo' zu berechnen.

// [ Impedanzwandler ]    

//             -<-R2--
//            |       |
// vi ---R1->--->-[A>----- vo
//            vx

auto ReSid::Filter::solveOpamp(Opamp* opamp, double n, int vi, int& x, Calculated& ca) -> int {
    // Ausgehend von Kirchhoff's Gesetz gilt für die Stromstärke im Knotenpunkt 'vx' für die zufließenden Ströme:
    // IR1 + IR2 und für die abfließenden Ströme 0
    // Somit gilt: IR1 + IR2 = 0
    // Beide Widerstände arbeiten im 'triode' Modus.
    // Für die Stromstärke im triode Modus gilt: n * (Vgst^2 - Vgdt^2)
    // 'vx' stellt für beide Widerstände die Ausgangs Spannung (source) dar
    // [einsetzen]
	// n1 * (Vgst^2 - Vgdt1^2) + n2 * (Vgst^2 - Vgdt2^2) = 0
    // n1 * ((Vddt - vx)^2 - (Vddt - vi)^2) + n2 * ((Vddt - vx)^2 - (Vddt - vo)^2) = 0
    // Die gesamte Gleichung wird durch 'n2' geteilt, mit dem Ziel die beiden Widerstände in Relation zu setzen.
    // n1 / n2 * ((Vddt - vx)^2 - (Vddt - vi)^2) + (Vddt - vx)^2 - (Vddt - vo)^2 = 0 / n2 = 0
    // 'n2' gehört zu dem Widerstand parallel zum Operationsverstärker
    // 'n1' gehört zu dem Gesamt Widerstand vor dem Verstärker
    // Für den Widerstand gilt: Rds = 1 / (n * Vgst)
    // Beide Widerstände haben die selbe 'gate' und 'source' Spannung.
    // [ umstellen nach Vgst ]
    // Vgst = 1 / (n * Rds)
    // [ gleichsetzen ]
    // 1 / (n1 * R1) = 1 / (n2 * R2)
    // n2 * R2 = n1 * R1
    // R2 / R1 = n1 / n2 (umgekehrt proportional)
    // das Verhältnis n1/n2 wird mit 'n' bezeichnet.
    // [vereinfachen]
    // f = (n + 1)*(Vddt - vx)^2 - n*(Vddt - vi)^2 - (Vddt - vo)^2 = 0
    // vo = vx (Y Achse) + x (delta zwischen vx und vo)
    // [einsetzen für vo]
    // f = (n + 1)*(Vddt - vx)^2 - n*(Vddt - vi)^2 - (Vddt - (vx + x))^2 = 0
    // Zur Vereinfachung werden Konstanten durch kürzere Ausdrücke ersetzt.
    // a = n + 1
    // b = Vddt
    // c = n*(b - vi)^2
    // Die beiden "Nicht" Konstanten sind 'x' und 'vx'
    // f(x, vx) = a*(b - vx)^2 - c - (b - (vx + x))^2 = 0
    // Durch Interpolation der Messpunkte ist zu jedem 'x' das passende 'vx' bereits vorberechnet.
    // Aufgabe: für die Eingangsspannung 'vi' muss das passende Paar aus 'x' und 'vx'
    // welches am Nähesten an 0 reicht durch Ausprobieren ermittelt werden.
    // Zur Lösung mit 2 unbekannten verwendet man das Newton-Raphson Verfahren: x -= f / f'
    // Newton-Raphson beschreibt eine schrittweise Verbesserung durch Annäherung.
    // Die Ableitung f' kann immer nur nach einer veränderlichen Variable formuliert werden.
    // Die andere Variable wird als Konstante behandelt.
    // f'(vx) = 2 * a*(b - vx) - 2 * (b - (vx + x))

    // ungültig: totales Differential ?
    // f'(x) = 2 * (b - (vx + x))
    // Ableiten mittels Kettenregel: innere * äußere Ableitung
    // das totale Differential aus beiden: f'(x) * dx + f'(vx) * dvx
    // f'(x, vx) = (-2 * a * (b - vx) + 2 * (b - (vx + x))) * dvx + 2 * (b - (vx + x)) * dx
    // vereinfachen: dx = 1. Die X-Werte sind direkt aufeinander folgende Ganzzahlen
    // Somit ist das delta zwischen 2 X-Werten immer 1
    // dvx: jezt wird klar wozu das delta zwischen 2 Y-Werten benötigt wird.
    // f'(x, vx) = 2 * (b - (vx + x)) * (dvx + 1) - 2 * a * dvx * (b - vx)
    // [ausklammern]
    // f'(x, vx) = 2 * ( (b - (vx + x)) * (dvx + 1) - a * dvx * (b - vx) )

    // Start: maximal größter Suchbereich für 'x'
    int ak = ca.ak, bk = ca.bk;
    double a = n + 1.0;
    int b = ca.kVddt;                            // skaliert: m * 2^16
    double b_vi = b > vi ? double(b - vi) : 0.0; // skaliert: m * 2^16
    double c = n * (b_vi * b_vi);                // skaliert: m^2 * 2^32

    for (;;) {
        int xk = x;

        int vx = opamp[x].vx; // skaliert m * 2^16
        int dvx = opamp[x].dvx; // skaliert 2^11
        // x wird vorher zurück übersetzt ( kann jetzt auch negativ sein )
        // Erklärung dafür, siehe 'build.cpp' -> 'Emulator::Coordinate'
        int vo = vx + (x << 1) - (1 << 16);
        // prüfen ob 'vo' 16 bit vozeichenlos ist.
        if (vo > ((1 << 16) - 1)) {
            vo = (1 << 16) - 1;
        } else if (vo < 0) {
            vo = 0;
        }

        double b_vx = b > vx ? double(b - vx) : 0.0;
        double b_vo = b > vo ? double(b - vo) : 0.0;

        double f = a * (b_vx * b_vx) - c - (b_vo * b_vo); // skaliert:  m^2 * 2^32
        // Da 'dv' Schrittgröße nicht "1" ist, muss mit delta multipliziert werden
        // double df = 2.0 * (b_vo - a * b_vx) * double(dvx); // skaliert m * 2^27
        double df = 2.0 * (a * b_vx - b_vo ) * double(dvx); // skaliert m * 2^27
        if (df) {
            x -= int(double(1 << 11) * f/df); // Der Quotient ist sklaiert: m^2 * 2^32 / m * 2^27 = m * 2^5.
        }

        if (unlikely(x == xk)) {
            // 'x' hat sich nicht mehr verändert. Somit ist kein weiterer Verbesserungsschritt möglich.
            return vo;
        }

        if (f < 0)
            ak = xk;
        else
            bk = xk;

        if (unlikely(x <= ak) || unlikely(x >= bk)) {
            // 'x' wird mittig in den Suchbereich gesetzt um in beide Richtungen weiter suchen zu können.
            x = (ak + bk) >> 1;

            if (unlikely(x == ak)) {
                // Der Suchbereich ist erschöpft.
                return vo;
            }
        }
    }
}

// [ integrierende Operationsverstärker für SID 8580 ]

//                 -<-C---
//                |       |
//  vi -----Rfc->--->-[A>----- vo
//                vx
//
// Vorberechnung ist nicht möglich, da neben der Eingangsspannung eine weitere Spannung,
// welche die Grenzfrequenz beschreibt, ins Spiel kommt. Diese Spannung gestaltet den
// Widerstandswert 'Rfc', welcher auf die Eingangsspannung wirkt, dynamisch. 

// Ausgehend von Kirchhoff's Gesetz gilt für die Stromstärke
// im Knotenpunkt 'vx' für die zufließenden Ströme: IRfc + IC
// und für die abfließenden Ströme zum op-amp 0.
// Somit gilt: IRfc + IC = 0    
// Für die Stromstärken im Widerstand (triode) und Kondensator gilt:
// IC = C * dv / dt
// IRfc = n * (Vgst^2 - Vgdt^2)
// 'Vgt' Gate Spannung
// 'vx' ist 'source'
// 'vi' ist 'drain'
// 'Vgst' = Vgt - vx
// 'Vgdt' = Vgt - vi
// n * (Vgst^2 - Vgdt^2) + C * dv / dt = 0
// n * ( (Vgt - vx)^2 - (Vgt - vi)^2 ) + C * (vc - vc0) / dt = 0
// Um die Spannungen am Kondensator zu isolieren, wird die Gleichung
// durch 'C' dividiert und mit 'dt' multipliziert.
// n * dt/C * ( (Vgt - vx)^2 - (Vgt - vi)^2 ) + vc - vc0 = 0
// vc = vc0 - n * dt/C ( (Vgt - vx)^2 - (Vgt - vi)^2 )
// n_dac = n * dt/C
// 'n' ist die Widerstandswirkung, welche sich aus der Grenzfrequenz ergiebt.

inline auto ReSid::Filter::solveIntegrate8580(int vi, int& vx, int& vc, Calculated& ca) -> int {

    unsigned int Vgst = kVgt - vx;
    unsigned int Vgdt = (vi < kVgt) ? kVgt - vi : 0;  // triode/saturation mode

    // Skalierung: (1/m)*2^13*m*2^16*m*2^16*2^-15 = m*2^30 = 13 + 16 + 16 - 15 = 30 
    int n_I_rfc = n_dac * (int(Vgst*Vgst - Vgdt*Vgdt) >> 15);

    // 'vc0' ist die Spannung zu Beginn, 'vc' die Spannung nach einer Mikro Sekunde.
    vc -= n_I_rfc;

    // Die Spannungen am op-amp und Kondensator sind gleich.
    // Die vom Kondensator abgegebene Spannung 'vc' entspricht der Differenz
    // zwischen 'vx' und 'vo' des op-amps.
    // Für den Feld Index gilt: x + (1 << 16) / 2  (16 bit skaliert)
    // 'vc' ist 2^30 skaliert 
    // (vc >> 14) + (1 << 16) / 2
	// (vc >> 15) + (1 << 15)
    // 'vc' und 'vx' sind Ausgangswerte für die Berechnung des Kondensators im Abstand einer Mikro Sekunde.
    vx = ca.opamp_rev[(vc >> 15) + (1 << 15)];

    // ermitteln der Ausgangsspannung 'vo'
    return vx + (vc >> 14);
}


// [integrierende Operationsverstärker für SID 6581]

//                 -<-C---
//                |       |
//  vi -----Rw->---->-[A>---- vo
//       |      | vx
//        --Rs->
//
// Vorberechnung ist nicht möglich, da neben der Eingangsspannung eine weitere Spannung,
// welche die Grenzfrequenz beschreibt, ins Spiel kommt. Diese Spannung gestaltet den
// Widerstandswert 'Rw', welcher auf die Eingangsspannung wirkt, dynamisch. 

// [ 'Rw' - vergrößerte Darstellung ]
//
//                   Vw                   
//                   |
//           Vdd     |
//              |---| R1
//             _|_   |
//           -- R1 --| Vg
//          |      __|__
//          |      -----  Rw
//          |      |   |
//  vi ------------     -------- vo

// Ausgehend von Kirchhoff's Gesetz gilt für die Stromstärke
// im Knotenpunkt 'vx' für die zufließenden Ströme: IRw + IRs + IC
// und für die abfließenden Ströme zum op-amp 0.
// Somit gilt: IRw + IRs + IC = 0
// IC = C * dv / dt
// IRw + IRs + C * (vc - vc0) / dt = 0
// Um die Spannungen am Kondensator zu isolieren, wird die Gleichung
// durch 'C' dividiert und mit 'dt' multipliziert
// dt/C * (IRw + IRs) + vc - vc0 = 0
// vc = vc0 - dt/C * (IRw + IRs) 
// 'Rs' hat eine konstante Gate Spannung und man kann davon ausgehen das der Transistor immer im 'triode' Modus arbeitet.
// IRs = n * (Vgst^2 - Vgdt^2)
// vc = vc0 - dt/C * (IRw + n * (Vgst^2 - Vgdt^2) ) 
// 'vx' ist source für 'Rw' und 'Rs'
// 'Rw' kann jedoch in allen 3 Modi arbeiten, da die 'gate' Spannung 'Vg' nicht konstant ist.
// Verwendung des EKV Modell für Berechnung von IRw
// vc = vc0 - dt/C * ( Is * (if - ir) + n * (Vgst^2 - Vgdt^2) ) 

// Berechnung der gate Spannung für Rw
// Die Spannung am 'gate' ist dynmaisch und hängt von 'vi' und der Spannung 'Vw',
// welche in Form der Grenzfrequenz einfließt, ab.
// 'Vg' beschreibt die 'source' Spannung für die beiden eingebetteten Widerstände R1,
// deren 'gate' Spannung wiederum 'Vdd' ist.
// 'Vg' ist der Knotenpunkt und nach Kirchhoff:
// IR1 + IR2 = 0 ( abfließender Strom am 'gate' Knotenpunkt ist 0 )
// n * ((Vddt - Vg)^2 - (Vddt - vi)^2 ) + n * ((Vddt - Vg)^2 - (Vddt - Vw)^2 ) = 0
// 'n' kürzt sich weg, da beide Widerstände von der Geometrie her gleich sind.
// 2 * (Vddt - Vg)^2 - (Vddt - vi)^2 - (Vddt - Vw)^2 = 0
// 2 * (Vddt - Vg)^2 = (Vddt - vi)^2 + (Vddt - Vw)^2
// (Vddt - Vg)^2 = ( (Vddt - vi)^2 + (Vddt - Vw)^2 ) / 2
// um 'Vg' zu isolieren, wird die Wurzel gezogen
// Vddt - Vg = sqrt( ( (Vddt - vi)^2 + (Vddt - Vw)^2 ) / 2 )
// Vg = Vddt - sqrt(((Vddt - vi)^2 + (Vddt - Vw)^2) / 2)

inline auto ReSid::Filter::solveIntegrate6581(int vi, int& vx, int& vc, Calculated& ca) -> int {

    int Vddt = ca.kVddt;  // skaliert m*2^16

    // ermitteln der 'gate' Spannung 'vg' für 'Rw'
    // Die Vorberechnung enthält das Ergebis für: Vg = Vddt - sqrt( index )
    unsigned int Vgst = Vddt - vx;
    unsigned int Vgdt = Vddt - vi;
    unsigned int Vgdt_2 = Vgdt * Vgdt; // 32 bit

	// Das Argument umfasst 32 bit: Vddt_Vw_2 + (Vgdt_2 >> 1)
    // Die oberen 16 bit bilden den index im Feld der vorberechneten Werte. Dabei werden
    // die unteren 16 bit verworfen. Das beeinträchtigt die Genauigkeit geringfügig.
    // Das Ergebnis ist bereits mit 'k' multipliziert.

    int kVg = vcr_kVg[ (Vddt_Vw_2 + (Vgdt_2 >> 1)) >> 16 ];
    
	// Berechnung Knotenpunkt 'vx'.
    // vc = vc0 - dt/C * ( Is * (if - ir) + n * (Vgst^2 - Vgdt^2) ) 
	// Is = 2 * u*Cox * Ut^2/k * W/L
	// if = ln^2(1 + e^((k*(Vg - Vt) - Vs) / (2*Ut))
	// ir = ln^2(1 + e^((k*(Vg - Vt) - Vd) / (2*Ut))	

    // IRs: dt/C * n * (Vgst^2 - Vgdt^2)
	// 'n_snake' = dt/C * n  ist bereits vorberechnet.
    // Skalierung: 2^13 * ( 2^16 * 2^16 / 2^15 ) = 2^30
    int n_I_snake = n_snake * (int(Vgst * Vgst - Vgdt_2) >> 15);
    
    // 'if' und 'ir' sind vorberechnet. Da die Formeln mathematisch identisch sind,
    // gibt es nur eine Vorberechnung. Über den index wird zwischen 'if' und 'ir'
    // unterschieden. Die beiden indexe werden ermittelt. 'vx' entspricht 'Vs'.
    
    int Vgs = kVg - vx + (1 << 15);
    //if (Vgs < 0) Vgs = 0;
    int Vgd = kVg - vi + (1 << 15);
    //if (Vgd < 0) Vgd = 0;

	// IRw: dt/C * Is * if - dt/C * is * ir
	// 'Vgs' bzw. 'Vgd' sind die Indexe zur entsprechenden Vorberechnung.
    // Die Ausmultiplizierung von dt/C * Is für jeweils 'if' und 'ir' ist ebenfalls
    // in der Vorberechnung enthalten.
    int n_I_vcr = int(unsigned(vcr_n_Ids_term[Vgs] - vcr_n_Ids_term[Vgd]) << 15);

    // vc = vc0 - dt/C * Rs + dt/C * Rw
    vc -= n_I_snake + n_I_vcr;
    
    // Zu jedem Spannungs delta existiert am Operations Verstärker die entsprechende Eingangsspannung 'vx'.
    vx = ca.opamp_rev[(vc >> 15) + (1 << 15)];

    // ermitteln der Ausgangsspannung 'vo'
    return vx + (vc >> 14);
}

}

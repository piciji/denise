
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

// Der gesamte Schaltkreis besteht aus Operations Verstärkern, Widerständen
// und Kondensatoren, welche wir berechnen müssen. Bevor wir in die 
// Berechnungen einsteigen, erfolgt eine Beschreibung der Bauteile, welche
// die Ausgangsformeln und Rechen-Grundlagen enthalten. Später verweise
// ich in den Erklärungen auf diese Glosar Stellen. Außerdem verweise ich
// hier auf die Datei 'circuit', welche den gesamten Filter Schaltkreis für 
// beide Sid Modelle darstellt.    

// [ NMOS FETS (Field Effect Transistor) ]
// Alle Widerstände sind sogenannte NMOS FETS, welche wir aktiv berechnen
// müssen. Derartige Widerstände lassen sich nicht durch die bekannte
// Formel R = U / I berechnen.
    
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

// Klären wir zuerst die Variablen:
// Vgst = Gate Spannung - Source Spannung - Threshold Spannung
// Vgdt = Gate Spannung - Drain Spannung - Threshold Spannung
// Vddt = Gate Spannung - Threshold Spannung = Effektiv Spannung
// Vds = Drain Spannung - Source Spannung
// W/L beschreibt die Geometrie und somit die Widerstandswirkung
// k' = u * cox = Elektonen Beweglichkeit * Oxid Kapazitanz pro Flächeneinheit
// k = Material Faktor. Es bezeichnet den Verlust von Kopplungs Effizienz zwischen
// 'gate' und Durchgang, verursacht durch das Material.
// n = ucox/2k * W/L ( Zusammenfassung zu Material Konstante 'n' )

// 'subthreshold' Modus
// Die Threshold Spannung ist die Mindestspannung, welche notwendig
// ist, das überhaupt ein Strom im Transistor fließt.
// Wenn (Vgate - Vsource) < Vthreshold fließt am Ausgang
// kein Strom. (I = 0)  

// 'saturation' Modus
// Erreicht die Eingangs Spannung am 'drain' die Efektiv Spannung am 'gate'
// befinden wir uns im 'saturation' Modus. In diesem Modus kann man nicht
// mehr von einer Widerstands Wirkung sprechen.

// 'triode' Modus
// Treffen die Bedingungen der beiden anderen Modi nicht zu befinden
// wir uns im 'triode' Modus. Als Widerstände verhalten sie die FETS
// nur in diesem Modus.
// 'Vds' können wir ersetzen durch: Vds = Vgst - Vgdt
// Probe
// Vds = Gate - Source - Threshold - ( Gate - Drain - Threshold )
// Vds = - Source + Drain = Drain - Source  
// Die obige 'triode' Formel lässt sich dann durch Umstellen wie folgt darstellen.  
// I = n * (2*Vgst - Vds) * Vds  // Ausgangsformel von oben
// I = n * ((2*Vgst - (Vgst - Vgdt)) * (Vgst - Vgdt) // einsetzen für Vds
// I = n * (Vgst + Vgdt) * (Vgst - Vgdt) // aus multiplizieren
// I = n * (Vgst^2 - Vgdt^2)

// Für den Widerstand gilt:
// 'drain' zu 'source' Widerstand: Rds = 1 / ( ucox/2k * W/L * Vgst) = 1 / (n * Vgst)    
    
// In den meisten Fällen arbeiten die FETS im triode Modus. Es gibt jedoch
// einen FET im Schaltkreis des '6581' welcher in allen Modis arbeitet.
// Je nach Modus müssten wir die entsprechende Formel verwenden.
// Aus mathematischer Sicht ist das mühsig.
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
// befinden sich außerhalb des Sid und sind über jeweis 2 pins herausgeführt.
// Für die Stromstärke im Kondensator gilt:
// I = C * dv / dt
// 'C' beschreibt die Kapazität des Kondesators
// 'dv' beschreibt die Spannungsänderung in Bezug auf die vergangene Zeit 'dt'
// 'dt' setzen wir auf eine Mikro Sekunde, da wir die Kalkulation jeden Zyklus
// durchführen. Der Sid läuft wie die Cpu für pal etwas unter einer Million
// und für ntsc etwas über einer Million Ticks pro Sekunde.
// Wir einigen uns auf eine Million für pal und ntsc gleichermaßen.
// Den genauen Wert zu verwenden, bringt in der Größenordnung keine Verbesserung.
    
// [ invertierende Operationsverstärker 'op-amp' ]
// op-amps dienen der Signal Verstärkung indem Spannungen generiert werden.
// Der invertierende op-amp senkt die Spannung ab einer bestimmten Eingangsspannung.    
// Damit ein Operationsverstärker funktionieren kann, benötigt dieser eine Versorgungsspannung.
// Für Operationsverstärker im Sid gibt es Spannungs Messpunkte vor und nach der Verstärkung.
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
// Beide Kondensatoren befinden sich außerhalb des Sid.
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
// Dies ist eine wichtige Grundlage, wodurch wir bekannte Spannungen am op-amp für
// parallel zu geschaltene Widerstände oder Kondensatoren ebenso verwenden können.
// Speziell für parallele Widerstände gilt für den Gesamt Widerstand:
// 1 / Rges = 1 / R1 + 1 / R2 + 1 / R3 ...    
// Sind nur 2 parallele Widerstände im Spiel lässt sich die Formel vereinfachen:
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
// Für folgende 3 Modelle ist es unser Ziel für eine beliebige
// Eingangsspannung 'vi' die Ausgangsspannung 'vo' zu berechnen.

// [ Impedanzwandler ]    
// Alle Operationsverstärker im Gesamtschaltkreis welche parallel zu einem Widerstand
// geschalten sind, werden durch folgende Funktion berechnet.
    
//             -<-R2--
//            |       |
// vi ---R1->--->-[A>----- vo
//            vx    
    
auto Sid::Filter::solveOpamp(Opamp* opamp, int n, int vi, int& x, Calculated& ca) -> int {  
    // Ausgehend von Kirchhoff's Gesetz (siehe Glossar) gilt für die Stromstärke
    // im Knotenpunkt 'vx' für die zufließenden Ströme: IR1 + IR2
    // und für die abfließenden Ströme 0 (siehe Glossar: Impedanzwandler)
    // Somit gilt: IR1 + IR2 = 0
    // Beide Widerstände ( Glossar: NMOS FETS) arbeiten im 'triode' Modus.
    // Für die Stromstärke im triode Modus gilt: n * (Vgst^2 - Vgdt^2)
    // Hinweis: Die Varibalen sind im Glossar erklärt.
    // 'vx' stellt für beide Widerstände die Ausgangs Spannung (source) dar, 
    // siehe Pfeilrichtung in der oberen Abbildung.
    // [einsetzen]
	// n1 * (Vgst^2 - Vgdt1^2) + n2 * (Vgst^2 - Vgdt2^2) = 0
    // n1 * ((Vddt - vx)^2 - (Vddt - vi)^2) + n2 * ((Vddt - vx)^2 - (Vddt - vo)^2) = 0
    // Wir teilen die gesamte Gleichung durch 'n2' mit dem Ziel die beiden Widerstände
    // in Relation zu setzen. Um den Wert der Gleichung nicht zu verändern, müssen wir
    // dies auf beiden Seiten durchführen.
    // n1 / n2 * ((Vddt - vx)^2 - (Vddt - vi)^2) + (Vddt - vx)^2 - (Vddt - vo)^2 = 0 / n2 = 0
    // 'n2' gehört zu dem Widerstand parallel zum Operationsverstärker.
    // 'n1' gehört zu dem Gesamt Widerstand vor dem Verstärker, welcher entweder
    // aus parallelen durch Schalter zuschaltbaren Einzelwiderständen besteht
    // oder mehreren Spannungseingängen mit zugehörigen Einzelwiderständen.
    // Für den Widerstand gilt: Rds = 1 / (n * Vgst)
    // Beide Widerstände haben die selbe 'gate' und 'source' Spannung.
    // [ umstellen nach Vgst ]
    // Vgst = 1 / (n * Rds)
    // [ gleichsetzen ]
    // 1 / (n1 * R1) = 1 / (n2 * R2)
    // n2 * R2 = n1 * R1
    // R2 / R1 = n1 / n2 
    // Je nach dem ob uns die Geometrie der Widerstände 'n' oder die Widerstandswerte direkt
    // zur Verfügung stehen, verwenden wir das entsprechende Verhältnis.
    // Hinweis: die Verhältnisse beider Darstellungen stehen umgekehrt zueinander.
    // Wir ersetzen das Verhältnis mit 'n'. 
    // [vereinfachen]
    // f = (n + 1)*(Vddt - vx)^2 - n*(Vddt - vi)^2 - (Vddt - vo)^2 = 0
    // Wir erinnern uns an den Bezug zwischen X-Achse und Y-Achse der Spannungen
    // am Operations Verstärker.
    // vo = vx (Y - Achse) + x (delta zwischen vx und vo)
    // [einsetzen für vo]
    // f = (n + 1)*(Vddt - vx)^2 - n*(Vddt - vi)^2 - (Vddt - (vx + x))^2 = 0
    // Zur Vereinfachung verwenden wir weitere Konstanten.
    // a = n + 1
    // b = Vddt
    // c = n*(Vddt - vi)^2
    // Die einzigen beiden "Nicht" Konstanten sind 'x' und 'vx'
    // f(x, vx) = a*(b - vx)^2 - c - (b - (vx + x))^2 = 0
    // Zu jedem 'x' haben wir das passende 'vx' bereits vorberechnet.
    // Jetzt müssten wir für jedes 'vi' das passende Paar aus 'x' und 'vx',
    // welches nach Lösung am Nähesten an 0 reicht durch Ausprobieren ermitteln.
    // Das ist mühsam.
    // Zur Lösung verwenden wir das Newton-Raphson Verfahren: x -= f / f'
    // Newton-Raphson beschreibt eine schrittweise Verbesserung durch Annäherung.
    // Dafür benötigen wir aber die Ableitung der Funktion.
    // Die Ableitung f' kann immer nur nach einer veränderlichen Variable formuliert werden.
    // Die andere Variable wird als Konstante behandelt.
    // Zum Ableiten verwenden wir die Kettenregel: innere * äußere Ableitung
    // f'(vx) = 2 * a*(b - vx) * -1 - 2 * (b - (vx + x)) * -1
    // f'(x) = -2 * (b - (vx + x)) * -1
    // das totale Differential aus beiden: f'(x) * dx + f'(vx) * dvx
    // f'(x, vx) = (-2 * a * (b - vx) + 2 * (b - (vx + x))) * dvx + 2 * (b - (vx + x)) * dx
    // vereinfachen: dx = 1 : zur Erinnerung die X-Werte sind direkt aufeinander folgende Ganzzahlen
    // Somit ist das delta zwischen 2 X-Werten immer 1
    // dvx: jezt wird klar wozu wir das delta zwischen 2 Y-Werten benötigen.
    // f'(x, vx) = 2 * (b - (vx + x)) * (dvx + 1) - 2 * a * dvx * (b - vx)
    // [ausklammern]
    // f'(x, vx) = 2 * ( (b - (vx + x)) * (dvx + 1) - a * dvx * (b - vx) )	

    // Wir starten mit dem maximal größten Suchbereich für 'x'
    int ak = ca.ak, bk = ca.bk;
    // Nun berechnen wir die oben formulierten Konstanten.
    // 'n' ist skaliert mit 2^7, 1 entspricht dann 1 * 2^7
    int a = n + (1 << 7); // n + 1
    // normalisierte und in 16 bit übersetzte Effektivspannung
    int b = ca.kVddt; // skaliert: m * 2^16   
    int b_vi = b - vi; // skaliert: m * 2^16
    // Wir ermitteln Werte für jedes "vi" was sich in 16 bit darstellen lässt. Somit müssen wir
    // nicht herausfinden, welche Werte überhaupt vorkommen können.
    // 'vi' kann nicht größer als die Effektivspannung sein
    if (b_vi < 0)
        b_vi = 0;
    // 16 bit * 16 bit = 32 bit >> 12 = 20 bit * 7 bit = 27 bit
    int c = n * int(unsigned(b_vi) * unsigned(b_vi) >> 12); // skaliert: m^2 * 2^27

    for (;;) {
        // Aus performance Gründen wird der Wert von "x" aus der Funktion herausgegeben und
        // als Startwert für den nächsten Funktionsaufruf verwendet. Das macht Sinn wenn die 
        // eingehenden Werte für "vi" aufeinander folgend sind.
        int xk = x;

        int vx = opamp[x].vx; // skaliert m * 2^16
        int dvx = opamp[x].dvx; // skaliert 2^11
        // x wird vorher zurück übersetzt ( kann jetzt auch negativ sein )
		// Erklärung dafür, siehe 'build.cpp' -> 'Emulator::Coordinate'
        int vo = vx + (x << 1) - (1 << 16);
        // prüfen ob 'vo' 16 bit vozeichenlos ist.
        if (vo >= (1 << 16)) {
            vo = (1 << 16) - 1; //obere Grenze überschritten
        } else if (vo < 0) {
            vo = 0; // untere Grenze überschritten
        }

        int b_vx = b - vx;
        // 'vx' kann nicht größer als die Effektivspannung sein
        if (b_vx < 0)
            b_vx = 0;
        int b_vo = b - vo;
        // 'vo' kann nicht größer als die Effektivspannung sein
        if (b_vo < 0)
            b_vo = 0;
        // skaliert : 2^7 * ( ( m * 2^16 *  m * 2^16 ) / 2^12 ) = m^2 * 2^27
        int f = a * int(unsigned(b_vx) * unsigned(b_vx) >> 12) - c - int(unsigned(b_vo) * unsigned(b_vo) >> 5);
		// dvx + 1, dvx ist 2^11 skaliert. Somit entspricht 'eins'  1 << 11
        // skaliert : m * 2^11 + Vorzeichen bit
        int df = (b_vo * (dvx + (1 << 11)) - a * (b_vx * dvx >> 7)) >> 15;
        // Der Quotient ist sklaiert: m^2 * 2^27  / m * 2^11 = m * 2^16.
        if (df)
            x -= f / df; // Newton Raphson

        if (x == xk) {
            // 'x' hat sich nicht mehr verändert. Somit ist kein weiterer Verbesserungsschritt möglich.
            // Mittels Newton Raphson haben wir uns trotz zwei unbekannter Variablen an die Lösung
            // der Gleichung herangetastet. 
            return vo;
        }

        // Abhängig in welche Richtung wir vom Nullpunkt entfernt sind, passen wir den Suchbereich neu an.
        if (f < 0) {
            // untere Grenze
            ak = xk;
        } else {
            // obere Grenze
            bk = xk;
        }
        // prüfen ob neues 'x' innerhalb des Suchbereiches liegt
        if ((x <= ak) || (x >= bk)) {
            // Wir setzen 'x' mittig in den Suchbereich um in beide Richtungen weiter suchen zu können.
            x = (ak + bk) >> 1;

            if (x == ak) {
                // Der Suchbereich ist erschöpft.
                return vo;
            }
        }
    }
}

// Das Problem der letzten Funktion ist, das multiple Eingangsspannungen als ein fiktiver Transistor
// modelliert werden. Trotz gleichem Widerstand für alle eingehenden Spannungen verhalten sich die 
// Transistoren nicht linear.
// Jetzt versuchen wir mehrere Eingangsspannungen als eigenständige zufließende Ströme zu berechnen.
// Vorraussetzung ist, das alle zufließenden Ströme den gleichen Widerstand haben. Das muss jedoch nicht
// für den Widerstand parallel zum op-amp gelten, nur für die Vor Widerstände.

// vi2---R1->- -<-R2--
//            |       |
// vi1---R1->--->-[A>----- vo
//            vx    

auto Sid::Filter::solveOpampMulti(Opamp* opamp, int n, int c, int& x, Calculated& ca) -> int {  

    // Ausgehend von Kirchhoff's Gesetz (siehe Glossar) gilt für die Stromstärke
    // im Knotenpunkt 'vx' für die zufließenden Ströme: '∑ count' IR1 + IR2    
    // und für die abfließenden Ströme 0 (siehe Glossar: Impedanzwandler)
    // Somit gilt: '∑ count' IR1 + IR2 = 0
    // '∑ count' - Anzahl 'count' von Eingängen    
    // Die Widerstände ( Glossar: NMOS FETS) arbeiten im 'triode' Modus.
    // Für die Stromstärke im triode Modus gilt: n * (Vgst^2 - Vgdt^2)
    // Hinweis: Die Varibalen sind im Glossar erklärt.
    // 'vx' stellt für alle Widerstände die Ausgangs Spannung (source) dar, 
    // siehe Pfeilrichtung in der oberen Abbildung.
    // [einsetzen z.B count=2 ]
    // n1 * (Vgst^2 - Vgdt1^2) + n1 * (Vgst^2 - Vgdt2^2) + n2 * (Vgst^2 - Vgdt3^2) = 0
    // n1 * ((Vddt - vx)^2 - (Vddt - vi1)^2) + n1 * ((Vddt - vx)^2 - (Vddt - vi2)^2) + n2 * ((Vddt - vx)^2 - (Vddt - vo)^2) = 0      
    // Wir teilen die gesamte Gleichung durch 'n2' mit dem Ziel die beiden Widerstände
    // in Relation zu setzen. Um den Wert der Gleichung nicht zu verändern, müssen wir
    // dies auf beiden Seiten durchführen.
    // n1 / n2 * ((Vddt - vx)^2 - (Vddt - vi1)^2) + n1 / n2 * ((Vddt - vx)^2 - (Vddt - vi2)^2) + (Vddt - vx)^2 - (Vddt - vo)^2 = 0 / n2 = 0
    // 'n2' gehört zu dem Widerstand parallel zum Operationsverstärker.
    // 'n1' gehört zu den Einzel Widerstanden vor dem Verstärker
    // Für den Widerstand gilt: Rds = 1 / (n * Vgst)
    // Beide Widerstände haben die selbe 'gate' und 'source' Spannung.
    // [ umstellen nach Vgst ]
    // Vgst = 1 / (n * Rds)
    // [ gleichsetzen ]
    // 1 / (n1 * R1) = 1 / (n2 * R2)
    // n2 * R2 = n1 * R1
    // R2 / R1 = n1 / n2 
    // Je nach dem ob uns die Geometrie der Widerstände 'n' oder die Widerstandswerte direkt
    // zur Verfügung stehen, verwenden wir das entsprechende Verhältnis.
    // Hinweis: die Verhältnisse beider Darstellungen stehen umgekehrt zueinander.
    // Wir ersetzen das Verhältnis mit 'n'. 
    // [vereinfachen]
    // f = (2n + 1) * (Vddt - vx)^2 - n*(Vddt - vi1)^2 - n*(Vddt - vi2)^2 - (Vddt - vo)^2 = 0
    
    // bei 3 Spannungseingängen (count = 3) ändert sich die Formel in:
    // f = (3n + 1) * (Vddt - vx)^2 - n*(Vddt - vi1)^2 - n*(Vddt - vi2)^2 - n*(Vddt - vi3)^2 - (Vddt - vo)^2 = 0
    // Das Muster für beliebige Eingänge sollte erkennbar sein.
	
    // Wir erinnern uns an den Bezug zwischen X-Achse und Y-Achse der Spannungen
    // am Operations Verstärker.
    // vo = vx (Y - Achse) + x (delta zwischen vx und vo)
    // [einsetzen für vo]
    // f = (2n + 1) * (Vddt - vx)^2 - n*(Vddt - vi1)^2 - n*(Vddt - vi2)^2 - (Vddt - (vx + x))^2 = 0
    // Zur Vereinfachung verwenden wir weitere Konstanten.
    // a = 2n + 1
    // b = Vddt
    // c = n*(Vddt - vi1)^2 + n*(Vddt - vi2)^2
    // 'c' wird in Abhängigkeit der eigehenden Spannungen als Konstante in diese Funktion übergeben.
    // Die einzigen beiden "Nicht" Konstanten sind 'x' und 'vx'
    // f(x, vx) = a*(b - vx)^2 - c - (b - (vx + x))^2 = 0
    
    // Zu jedem 'x' haben wir das passende 'vx' bereits vorberechnet.
    // Jetzt müssten wir für jedes 'vi' das passende Paar aus 'x' und 'vx',
    // welches nach Lösung am Nähesten an 0 reicht durch Ausprobieren ermitteln.
    // Das ist mühsam.
    // Zur Lösung verwenden wir das Newton-Raphson Verfahren: x -= f / f'
    // Newton-Raphson beschreibt eine schrittweise Verbesserung durch Annäherung.
    // Dafür benötigen wir aber die Ableitung der Funktion.
    // Durch Verwendung der Konstanten 'a' 'b' und 'c' sieht die Formel genau so aus
    // wie in der Funktion zuvor für nur einen Spannungseingang. Das gilt für eine beliebige
    // Anzahl an Eingängen. Die Ableitungen sind somit identisch, siehe Herleitung für einen
    // Spannungseingang.
    // f'(x, vx) = 2 * ( (b - (vx + x)) * (dvx + 1) - a * dvx * (b - vx) )	
    // Da 'c' auschließlich eine Konstante ist und keine Abhängigkeit zu 'x' oder 'vx' hat,
    // spielt es für die Ableitung keine Rolle.
    // Die Ableitungen für eine beliebige Anzahl an Spannungseingängen unterscheiden sich nur
    // durch 'a'
    // a = count * n + 1
    
    // Wir starten mit dem maximal größten Suchbereich für 'x'
    int ak = ca.ak, bk = ca.bk;
    // 'n' ist skaliert mit 2^7, 1 entspricht dann 1 * 2^7
    int a = n + (1 << 7); // n + 1
    // normalisierte und in 16 bit übersetzte Effektivspannung
    int b = ca.kVddt; // skaliert: m * 2^16
	
    for (;;) {
        // Aus performance Gründen wird der Wert von "x" aus der Funktion herausgegeben und
        // als Startwert für den nächsten Funktionsaufruf verwendet. Das macht Sinn wenn die 
        // eingehenden Werte für "vi" aufeinander folgend sind.
        int xk = x;

        int vx = opamp[x].vx; // skaliert m * 2^16
        int dvx = opamp[x].dvx; // skaliert 2^11
        // x wird vorher zurück übersetzt ( kann jetzt auch negativ sein )
        // Erklärung dafür, siehe 'build.cpp' -> 'Emulator::Coordinate'
        int vo = vx + (x << 1) - (1 << 16);
        // prüfen ob 'vo' 16 bit vozeichenlos ist.
        if (vo >= (1 << 16)) {
            vo = (1 << 16) - 1; //obere Grenze überschritten
        } else if (vo < 0) {
            vo = 0; // untere Grenze überschritten
        }

        int b_vx = b - vx;
        // 'vx' kann nicht größer als die Effektivspannung sein
        if (b_vx < 0)
            b_vx = 0;
        int b_vo = b - vo;
        // 'vo' kann nicht größer als die Effektivspannung sein
        if (b_vo < 0)
            b_vo = 0;
        // skaliert : 2^7 * ( ( m * 2^16 *  m * 2^16 ) / 2^12 ) = m^2 * 2^27
        //int f = a * int(unsigned(b_vx) * unsigned(b_vx) >> 12) - c - int(unsigned(b_vo) * unsigned(b_vo) >> 5);
        int f = a * ca.calcQ1[b_vx] - c - ca.calcQ2[b_vo];
        // dvx + 1, dvx ist 2^11 skaliert. Somit entspricht 'eins'  1 << 11
        // skaliert : m * 2^11 + Vorzeichen bit
        int df = (b_vo * (dvx + (1 << 11)) - a * (b_vx * dvx >> 7)) >> 15;
        // Der Quotient ist sklaiert: m^2 * 2^27  / m * 2^11 = m * 2^16.
        x -= f / df; // Newton Raphson

        if (x == xk) {
            // 'x' hat sich nicht mehr verändert. Somit ist kein weiterer Verbesserungsschritt möglich.
            // Mittels Newton Raphson haben wir uns trotz zwei unbekannter Variablen an die Lösung
            // der Gleichung herangetastet. 
            return vo;
        }

        // Abhängig in welche Richtung wir vom Nullpunkt entfernt sind, passen wir den Suchbereich neu an.
        if (f < 0) {
            // untere Grenze
            ak = xk;
        } else {
            // obere Grenze
            bk = xk;
        }
        // prüfen ob neues 'x' innerhalb des Suchbereiches liegt
        if ((x <= ak) || (x >= bk)) {
            // Wir setzen 'x' mittig in den Suchbereich um in beide Richtungen weiter suchen zu können.
            x = (ak + bk) >> 1;

            if (x == ak) {
                // Der Suchbereich ist erschöpft.
                return vo;
            }
        }
    }

}

// [ integrierende Operationsverstärker : gilt nur für Sid 8580 ]
// Alle Operationsverstärker im Schaltkreis des 8580 welche parallel zu einem Kondensator
// geschalten sind, werden durch folgende Funktion in jedem Zyklus neu berechnet.

//                 -<-C---
//                |       |
//  vi -----Rfc->--->-[A>----- vo
//                vx
//
// Vorberechnung ist nicht möglich, da neben der Eingangsspannung eine weitere Spannung,
// welche die Grenzfrequenz beschreibt, ins Spiel kommt. Diese Spannung gestaltet den
// Widerstandswert 'Rfc', welcher auf die Eingangsspannung wirkt, dynamisch. 

// Ausgehend von Kirchhoff's Gesetz (siehe Glossar) gilt für die Stromstärke
// im Knotenpunkt 'vx' für die zufließenden Ströme: IRfc + IC
// und für die abfließenden Ströme zum op-amp 0.
// Somit gilt: IRfc + IC = 0    
// Für die Stromstärken im Widerstand (triode) und Kondensator gilt: (siehe Glossar)
// IC = C * dv / dt
// IRfc = n * (Vgst^2 - Vgdt^2)
// 'vx' ist 'source'
// 'vi' ist 'drain'
// [einsetzen]
// n * (Vgst^2 - Vgdt^2) + C * dv / dt = 0
// n * ( (Vgt - vx)^2 - (Vgt - vi)^2 ) + C * (vc - vc0) / dt = 0
// Um die Spannungen am Kondensator zu isolieren, dividieren wir die Gleichung
// durch 'C' und multiplizieren mit 'dt'
// n * dt/C * ( (Vgt - vx)^2 - (Vgt - vi)^2 ) + vc - vc0 = 0
// vc = vc0 - n * dt/C ( (Vgt - vx)^2 - (Vgt - vi)^2 ) 
// n_dac = n * dt/C
// dt/C sind unveränderliche Werte
// 'n' ist die Widerstandswirkung, welche sich aus der Grenzfrequenz ergiebt.
// Wir berechnen den Faktor am Besten vor, immer dann wenn eine neue Grenzfrequenz
// im Register gesetzt wird.

inline auto Sid::Filter::solveIntegrate8580(int vi, int& vx, int& vc, Calculated& ca) -> int {

    unsigned int Vgst = kVgt - vx;
    unsigned int Vgdt = (vi < kVgt) ? kVgt - vi : 0;  // triode/saturation mode

    // Wir lösen die Gleichung auf: vc = vc0 - n * dt/C ( (Vgt - vx)^2 - (Vgt - vi)^2 ) 
    // 'n_dac' ist der vorberechnete Faktor.
    // Skalierung: (1/m)*2^13*m*2^16*m*2^16*2^-15 = m*2^30 = 13 + 16 + 16 - 15 = 30 
    int n_I_rfc = n_dac * (int(Vgst*Vgst - Vgdt*Vgdt) >> 15);

    // 'vc0' ist die Spannung zu Beginn, 'vc' die Spannung nach einer Mikro Sekunde.    
    // Die Funktion verwendet 'vc' als Referenz Parameter. Wir merken uns den
    // letzten Wert als Spannung 'vc0' für die Berechnung des nächsten Zeit Intervals.
    vc -= n_I_rfc;

    // Siehe Glossar ( Parallelschaltung ) sind die Spannungen am op-amp und 
    // Kondensator gleich.
    // Die vom Kondensator abgegebene Spannung 'vc' entspricht der Differenz
    // zwischen 'vx' und 'vo' des op-amps.
    // In einem Feld haben wir für alle Spannungs Differenzen die zugehörige
    // Spannung 'vx' vorberechnet.
    // Für den Feld Index gilt: x + (1 << 16) / 2  (16 bit skaliert)
    // 'vc' ist 2^30 skaliert 
    // (vc >> 14) + (1 << 16) / 2 
	// Die Division durch 2 wird aufgelöst.
	// (vc >> 15) + (1 << 15)
    // 'vx' müssen wir uns ebenso für die nächste Runde merken, da der     
    // Kondensator in jeder Mikro Sekunde neu berechnet wird. 
    // 'vc' und 'vx' sind Ausgangswerte für die nächste Mikro Sekunde.
    vx = ca.opamp_rev[(vc >> 15) + (1 << 15)];

    // Wir bestimmen 'vo' wie gehabt.
    return vx + (vc >> 14);
}


// [integrierende Operationsverstärker : gilt nur für Sid 6581]
// Alle Operationsverstärker im Schaltkreis des 6581, welche parallel zu einem Kondensator
// geschalten sind, werden durch folgende Funktion in jedem Zyklus neu berechnet.

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

// Ausgehend von Kirchhoff's Gesetz (siehe Glossar) gilt für die Stromstärke
// im Knotenpunkt 'vx' für die zufließenden Ströme: IRw + IRs + IC
// und für die abfließenden Ströme zum op-amp 0.
// Somit gilt: IRw + IRs + IC = 0    
// [einsetzen]
// IRw + IRs + C * (vc - vc0) / dt = 0
// Um die Spannungen am Kondensator zu isolieren, dividieren wir die Gleichung
// durch 'C' und multiplizieren mit 'dt'
// dt/C * (IRw + IRs) + vc - vc0 = 0
// vc = vc0 - dt/C * (IRw + IRs) 
// 'Rs' hat eine konstante Gate Spannung und man kann davon ausgehen das der
// Transistor immer im 'triode' Modus arbeitet.
// IRs = n * (Vgst^2 - Vgdt^2)
// [einsetzen von IRs]
// vc = vc0 - dt/C * (IRw + n * (Vgst^2 - Vgdt^2) ) 
// 'vx' ist source für 'Rw' und 'Rs'
// 'Rw' kann jedoch in allen 3 Modi arbeiten, da die 'gate' Spannung 'Vg'
// nicht konstant ist. Wir verwenden das EKV Modell, siehe Glossar.
// [einsetzen von IRw]
// vc = vc0 - dt/C * ( Is * (if - ir) + n * (Vgst^2 - Vgdt^2) ) 

// Zuerst müssen wir jedoch die 'gate' Spannung 'Vg' jede Mikro Sekunde neu berechnen.
// Die Spannung am 'gate' ist dynmaisch und hängt von 'vi' und der Spannung 'Vw',
// welche in Form der Grenzfrequenz einfließt, ab.
// 'Vg' beschreibt die 'source' Spannung für die beiden eingebetteten Widerstände R1,
// deren 'gate' Spannung wiederum 'Vdd' ist.
// 'Vg' ist der Knotenpunkt und nach Kirchhoff schreiben wir.
// IR1 + IR1 = 0 ( abfließender Strom am 'gate' Knotenpunkt ist 0 )
// n * ((Vddt - Vg)^2 - (Vddt - vi)^2 ) + n * ((Vddt - Vg)^2 - (Vddt - Vw)^2 ) = 0
// 'n' kürzt sich weg, da beide Widerstände von der Geometrie her gleich sind.
// 2 * (Vddt - Vg)^2 - (Vddt - vi)^2 - (Vddt - Vw)^2 = 0
// 2 * (Vddt - Vg)^2 = (Vddt - vi)^2 + (Vddt - Vw)^2
// wir dividieren beide Seiten durch 2
// (Vddt - Vg)^2 = ( (Vddt - vi)^2 + (Vddt - Vw)^2 ) / 2
// um 'Vg' zu isolieren, müssen wir die Wurzel ziehen
// Vddt - Vg = sqrt( ( (Vddt - vi)^2 + (Vddt - Vw)^2 ) / 2 )
// Vg = Vddt - sqrt(((Vddt - vi)^2 + (Vddt - Vw)^2) / 2)

// Wir müssen jeden Zyklus die 'gate' Spannung und mittels dieser 'vo' über das 
// EKV Modell berechnen. Das Problem ist nur, das wir das zur Laufzeit
// also eine Million mal pro Sekunde nicht berechnen wollen. Der Emulator würde in
// die Knie gehen. Alles vorberechnen können wir wie bereits erklärt auch nicht.
// Wir müssen versuchen Zwischenergebnisse vorzuberechnen.
// Z.b. Wurzel Ziehen ist ein mathematisch aufwendiger Prozess.
// Alle Werte sind 16 bit breit, das heißt wenn wir was vorberechnen, müssen wir dies
// für 1 << 16 Werte erledigen.
inline auto Sid::Filter::solveIntegrate6581(int vi, int& vx, int& vc, Calculated& ca, bool moreAccuracy) -> int {

    int Vddt = ca.kVddt;  // skaliert m*2^16
    int kVg;

    // Ok zuerst ermitteln wir die 'gate' Spannung 'vg' für 'Rw'.
    // Vg = Vddt - sqrt(((Vddt - vi)^2 + (Vddt - Vw)^2) / 2)
    // Das Ergebnis des Wurzel Ziehens ist vorberechnet. Das Argument der Wurzel dient als
    // 'index' im Feld der vorberechneten Werte.
    // Die Vorberechnung enthält das Ergebis für: Vg = Vddt - sqrt( index )
    // Wir ermitteln das Argument.
    unsigned int Vgst = Vddt - vx;
    unsigned int Vgdt = Vddt - vi;
    unsigned int Vgdt_2 = Vgdt * Vgdt; // 32 bit
    // 'Vddt_Vw_2' ist zu diesem Zeitpunkt bereits vorberechnet, siehe Änderung Grenzfrequenz.
    // Vddt_Vw_2 = (Vddt - Vw)^2 / 2
	// Das Argument umfasst 32 bit: Vddt_Vw_2 + (Vgdt_2 >> 1)
    // Die oberen 16 bit bilden den index im Feld der vorberechneten Werte. Dabei werden
    // die unteren 16 bit verworfen. Das beeinträchtigt die Genauigkeit geringfügig.
    // Das Ergebnis ist bereits mit 'k' multipliziert.
    
    if (!moreAccuracy)
        kVg = vcr_kVg[ (Vddt_Vw_2 + (Vgdt_2 >> 1)) >> 16 ];
    else
        // 'kVg' ohne Vorberechnung aber dafür ohne Genauigkeitsverlust. Das kostet natürlich ein paar frames extra.
        kVg = ( ca.kVddtWithoutVmin - std::sqrt( Vddt_Vw_2 + (Vgdt_2 >> 1) ) ) * ca.k - ca.vmin + 0.5;
    
	// Nun berechnen wir den Knotenpunkt 'vx'.    
    // vc = vc0 - dt/C * ( Is * (if - ir) + n * (Vgst^2 - Vgdt^2) ) 
	// Is = 2 * u*Cox * Ut^2/k * W/L
	// if = ln^2(1 + e^((k*(Vg - Vt) - Vs) / (2*Ut))
	// ir = ln^2(1 + e^((k*(Vg - Vt) - Vd) / (2*Ut))	
	    
    // Ausmultiplizieren von dt/C
    // IRs: dt/C * n * (Vgst^2 - Vgdt^2)
	// 'n_snake' = dt/C * n  ist bereits vorberechnet.
    // Skalierung: 2^13 * ( 2^16 * 2^16 / 2^15 ) = 2^30
    int n_I_snake = n_snake * (int(Vgst * Vgst - Vgdt_2) >> 15);
    
    // 'if' und 'ir' sind vorberechnet. Da beide mathematisch identisch sind,
    // gibt es nur eine Vorberechnung. Über den index wird zwischen 'if' und 'ir'
    // unterschieden. Die beiden indexe werden ermittelt. 'vx' entspricht 'Vs'.
    
    int Vgs = kVg - vx;
    if (Vgs < 0) Vgs = 0;
    int Vgd = kVg - vi;
    if (Vgd < 0) Vgd = 0;

	// IRw: dt/C * Is * if - dt/C * is * ir
	// 'Vgs' bzw. 'Vgd' sind die Indexe zur entsprechenden Vorberechnung.
    // Die Ausmultiplizierung von dt/C * Is für jeweils 'if' und 'ir' ist ebenfalls
    // in der Vorberechnung enthalten.
    int n_I_vcr = int(unsigned(vcr_n_Ids_term[Vgs] - vcr_n_Ids_term[Vgd]) << 15);

    // Einsetzen der Ergebnisse aller Vorberchnungen um die Formel abzuschließen:
    // vc = vc0 - dt/C * Rs + dt/C * Rw
    vc -= n_I_snake + n_I_vcr;
    
    // Wie bereits in 'solveIntegrate8580' erklärt, existiert zu jedem Spannungs delta
    // am Operations Verstärker die entsprechende Eingangsspannung 'vx'.
    vx = ca.opamp_rev[(vc >> 15) + (1 << 15)];

    // Wir bestimmen 'vo' wie gehabt.
    return vx + (vc >> 14);
}

}
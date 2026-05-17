
namespace LIBC64 {

auto ReSid::Filter::clockSeparate(int voice1, int voice2, int voice3) -> void {
    Calculated& ca = calculated[ type ];
	double c = 0.0;
	double a = 1.0;

    v1 = (voice1 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
    v2 = (voice2 * ca.voiceScaleS14 >> 18) + ca.voiceDC;
    v3 = (voice3 * ca.voiceScaleS14 >> 18) + ca.voiceDC;

	if ( type == Type::MOS_6581 ) {
        Vlp = solveIntegrate6581( Vbp, Vlp_x, Vlp_vc, ca );
		Vbp = solveIntegrate6581( Vhp, Vbp_x, Vbp_vc, ca );

	} else {
		Vlp = solveIntegrate8580( Vbp, Vlp_x, Vlp_vc, ca );
		Vbp = solveIntegrate8580( Vhp, Vbp_x, Vbp_vc, ca );
    }

	VbpRes = ca.resonance[res][Vbp];

	for(auto& flt : separateFlt ) {
		// ∑ n * (Vddt - vi)^2
		double _vi = double(ca.kVddt) - double(*flt.vi);
		if (_vi > 0.0)
			c += flt.n * _vi * _vi;

		a += flt.n;
	}

	Vhp = solveOpampSeparate( ca.opamp, a, c, nrXFilter, ca );
}

auto ReSid::Filter::outputSeparate() -> short {
	Calculated& ca = calculated[ this->type ];
	double c = 0.0;
	double a = 1.0;

	for(auto& mix : separateMix) {
		// ∑ n * (Vddt - vi)^2
		double _vi = double(ca.kVddt) - double(*mix.vi);
		if (_vi > 0.0)
			c += mix.n * _vi * _vi;

		a += mix.n;
	}

	return (short)(ca.gain[vol][ solveOpampSeparate( ca.opamp, a, c, nrXMixer, ca ) ] - (1 << 15) );
}

// vi2---R1->- -<-R2--
//            |       |
// vi1---R1->--->-[A>----- vo
//            vx

auto ReSid::Filter::solveOpampSeparate(Opamp* opamp, double a, double c, int& x, Calculated& ca) -> int {
    // Ausgehend von Kirchhoff's Gesetz (siehe Glossar) gilt für die Stromstärke
    // im Knotenpunkt 'vx' für die zufließenden Ströme: '∑ count' IR1 + IR2
    // und für die abfließenden Ströme 0 (siehe Glossar: Impedanzwandler)
    // Somit gilt: '∑ count' IR1 + IR2 = 0
    // '∑ count' - Anzahl von Eingängen
    // Die Widerstände arbeiten im 'triode' Modus.
    // Für die Stromstärke im triode Modus gilt: n * (Vgst^2 - Vgdt^2)
    // 'vx' stellt für alle Widerstände die Ausgangs Spannung (source) dar
    // [einsetzen z.B count=2 ]
    // n1 * (Vgst^2 - Vgdt1^2) + n2 * (Vgst^2 - Vgdt2^2) + no * (Vgst^2 - Vgdt3^2) = 0
    // n1 * ((Vddt - vx)^2 - (Vddt - vi1)^2) + n2 * ((Vddt - vx)^2 - (Vddt - vi2)^2) + no * ((Vddt - vx)^2 - (Vddt - vo)^2) = 0
	// Die gesamte Gleichung wird durch 'no' geteilt, mit dem Ziel die Widerstände in Relation zu setzen.
    // n1 / no * ((Vddt - vx)^2 - (Vddt - vi1)^2) + n2 / no * ((Vddt - vx)^2 - (Vddt - vi2)^2) + (Vddt - vx)^2 - (Vddt - vo)^2 = 0 / n0 = 0
    // 'no' gehört zu dem Widerstand parallel zum Operationsverstärker.
    // 'n1', 'n2' gehören zu den Einzel Widerständen vor dem Operationsverstärker.

	// n1 / no * (Vddt - vx)^2 - n1 / no * (Vddt - vi1)^2 + n2 / no * (Vddt - vx)^2 - n2 / no * (Vddt - vi2)^2 + (Vddt - vx)^2 - (Vddt - vo)^2 = 0
	// das Verhältnis n1/no wird mit 'n1' und n2/no mit 'n2' bezeichnet.
	// f = (n1 + n2 + 1) * (Vddt - vx)^2 - n1 * (Vddt - vi1)^2 - n2 * (Vddt - vi2)^2 - (Vddt - vo)^2 = 0

    // bei 3 Spannungseingängen (count = 3) ändert sich die Formel in:
	// f = (n1 + n2 + n3 + 1) * (Vddt - vx)^2 - n1 * (Vddt - vi1)^2 - n2 * (Vddt - vi2)^2 - n3 * (Vddt - vi3)^2 - (Vddt - vo)^2 = 0

	// vo = vx (Y Achse) + x (delta zwischen vx und vo)
    // [einsetzen für vo]
	// f = (n1 + n2 + 1) * (Vddt - vx)^2 - n1 * (Vddt - vi1)^2 - n2 * (Vddt - vi2)^2 - (Vddt - (vx + x))^2 = 0
	// Zur Vereinfachung werden Konstanten durch kürzere Ausdrücke ersetzt.
    // a = ('∑ count' nx) + 1
    // b = Vddt
    // c = '∑ count' nx * (b - vix)^2
    // 'c' wird in Abhängigkeit der eigehenden Spannungen als Konstante in diese Funktion übergeben.
    // Die beiden "Nicht" Konstanten sind 'x' und 'vx'
    // f(x, vx) = a * (b - vx)^2 - c - (b - (vx + x))^2 = 0

	// Durch Interpolation der Messpunkte ist zu jedem 'x' das passende 'vx' bereits vorberechnet.
	// Aufgabe: für die Eingangsspannung 'vi' muss das passende Paar aus 'x' und 'vx'
	// welches am Nähesten an 0 reicht durch Ausprobieren ermittelt werden.
	// Zur Lösung mit 2 unbekannten verwendet man das Newton-Raphson Verfahren: x -= f / f'
	// Newton-Raphson beschreibt eine schrittweise Verbesserung durch Annäherung.
	// Die Ableitung f' kann immer nur nach einer veränderlichen Variable formuliert werden.
	// Die andere Variable wird als Konstante behandelt.
	// f'(vx) = 2 * a * (b - vx) - 2 * (b - (vx + x))

	int ak = ca.ak, bk = ca.bk;
	int b = ca.kVddt;                            // skaliert: m * 2^16

	for (;;) {
		int xk = x;

		int vx = opamp[x].vx; // skaliert m * 2^16
		int dvx = opamp[x].dvx; // skaliert 2^11
		int vo = vx + (x << 1) - (1 << 16);

		if (vo > ((1 << 16) - 1)) {
			vo = (1 << 16) - 1;
		} else if (vo < 0) {
			vo = 0;
		}

		double b_vx = b > vx ? double(b - vx) : 0.0;
		double b_vo = b > vo ? double(b - vo) : 0.0;

		double f = a * (b_vx * b_vx) - c - (b_vo * b_vo); // skaliert:  m^2 * 2^32

		double df = 2.0 * (a * b_vx - b_vo ) * double(dvx); // skaliert m * 2^27

		if (df) {
			x -= int(double(1 << 11) * f/df); // Der Quotient ist sklaiert: m^2 * 2^32 / m * 2^27 = m * 2^5.
		}

		if (unlikely(x == xk)) {
			return vo;
		}

		if (f < 0)
			ak = xk;
		else
			bk = xk;

		if (unlikely(x <= ak) || unlikely(x >= bk)) {
			x = (ak + bk) >> 1;

			if (unlikely(x == ak)) {
				return vo;
			}
		}
	}
}

auto ReSid::Filter::prepareSeparate() -> void {
    separateFlt.clear();
	separateMix.clear();

	// Filter inputs
	SeparateInput mLF;
	mLF.vi = &Vlp;
	mLF.n = 1.0; // opamp + Eingangspannung haben den gleichen Widerstand R2/R1 = 1

	SeparateInput mBF;
	mBF.vi = &VbpRes;
	mBF.n = 1.0;

	SeparateInput mV1F;
	mV1F.vi = &v1;
	mV1F.n = 1.0;

	SeparateInput mV2F;
	mV2F.vi = &v2;
	mV2F.n = 1.0;

	SeparateInput mV3F;
	mV3F.vi = &v3;
	mV3F.n = 1.0;

	SeparateInput mVEF;
	mVEF.vi = &ve;
	mVEF.n = type == Type::MOS_8580 ? (7.0/12.0) : 1.0; // fixme

	// Mixer inputs (fixme: R2/R1)
	SeparateInput mLM;
	mLM.vi = &Vlp;
	mLM.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/5.0);

	SeparateInput mBM;
	mBM.vi = &Vbp;
	mBM.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/5.0);

	SeparateInput mHM;
	mHM.vi = &Vhp;
	mHM.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/5.0);

	SeparateInput mV1M;
	mV1M.vi = &v1;
	mV1M.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/6.0);

	SeparateInput mV2M;
	mV2M.vi = &v2;
	mV2M.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/6.0);

	SeparateInput mV3M;
	mV3M.vi = &v3;
	mV3M.n = type == Type::MOS_8580 ? (8.0/5.0) : (8.0/6.0);

	SeparateInput mVEM;
	mVEM.vi = &ve;
	mVEM.n = type == Type::MOS_8580 ? (8.0/9.0) : (8.0/6.0);

	//
	separateFlt.push_back( mLF );
	separateFlt.push_back( mBF );
	switch ( sum & 0xf ) {
		default:
		case 0x0:																													break;
		case 0x1: separateFlt.push_back( mV1F );																						break;
		case 0x2: separateFlt.push_back( mV2F );																						break;
		case 0x3: separateFlt.push_back( mV1F ); separateFlt.push_back( mV2F );															break;
		case 0x4: separateFlt.push_back( mV3F );																						break;
		case 0x5: separateFlt.push_back( mV3F ); separateFlt.push_back( mV1F );															break;
		case 0x6: separateFlt.push_back( mV3F ); separateFlt.push_back( mV2F );															break;
		case 0x7: separateFlt.push_back( mV1F ); separateFlt.push_back( mV2F ); separateFlt.push_back( mV3F );								break;
		case 0x8: separateFlt.push_back( mVEF );																						break;
		case 0x9: separateFlt.push_back( mVEF ); separateFlt.push_back( mV1F );															break;
		case 0xa: separateFlt.push_back( mVEF ); separateFlt.push_back( mV2F );															break;
		case 0xb: separateFlt.push_back( mVEF ); separateFlt.push_back( mV1F ); separateFlt.push_back( mV2F );								break;
		case 0xc: separateFlt.push_back( mVEF ); separateFlt.push_back( mV3F );															break;
		case 0xd: separateFlt.push_back( mVEF ); separateFlt.push_back( mV3F ); separateFlt.push_back( mV1F );								break;
		case 0xe: separateFlt.push_back( mVEF ); separateFlt.push_back( mV3F ); separateFlt.push_back( mV2F );								break;
		case 0xf: separateFlt.push_back( mVEF ); separateFlt.push_back( mV3F ); separateFlt.push_back( mV2F ); separateFlt.push_back( mV1F );	break;
	}

	switch( mix & 0xf ) {
		default:
		case 0: break;
		case 1: separateMix.push_back( mV1M );  break;
		case 2: separateMix.push_back( mV2M );  break;
		case 3: separateMix.push_back( mV1M ); separateMix.push_back( mV2M );  break;
		case 4: separateMix.push_back( mV3M );  break;
		case 5: separateMix.push_back( mV1M ); separateMix.push_back( mV3M );  break;
		case 6: separateMix.push_back( mV2M ); separateMix.push_back( mV3M );  break;
		case 7: separateMix.push_back( mV1M ); separateMix.push_back( mV2M ); separateMix.push_back( mV3M );  break;
		case 8: separateMix.push_back( mVEM );  break;
		case 9: separateMix.push_back( mV1M ); separateMix.push_back( mVEM );  break;
		case 10: separateMix.push_back( mV2M ); separateMix.push_back( mVEM );  break;
		case 11: separateMix.push_back( mV1M ); separateMix.push_back( mV2M ); separateMix.push_back( mVEM );  break;
		case 12: separateMix.push_back( mV3M ); separateMix.push_back( mVEM );  break;
		case 13: separateMix.push_back( mV1M ); separateMix.push_back( mV3M ); separateMix.push_back( mVEM );  break;
		case 14: separateMix.push_back( mV2M ); separateMix.push_back( mV3M ); separateMix.push_back( mVEM );  break;
		case 15: separateMix.push_back( mV1M ); separateMix.push_back( mV2M ); separateMix.push_back( mV3M ); separateMix.push_back( mVEM );  break;
	}

	switch( (mix >> 4) & 0x7 ) {
		default:
		case 0: break;
		case 1: separateMix.push_back( mLM ); break;
		case 2: separateMix.push_back( mBM ); break;
		case 3: separateMix.push_back( mLM ); separateMix.push_back( mBM ); break;
		case 4: separateMix.push_back( mHM ); break;
		case 5: separateMix.push_back( mLM ); separateMix.push_back( mHM ); break;
		case 6: separateMix.push_back( mBM ); separateMix.push_back( mHM ); break;
		case 7: separateMix.push_back( mLM ); separateMix.push_back( mBM ); separateMix.push_back( mHM ); break;
	}

	Calculated& ca = calculated[ type ];
	nrXMixer = nrXFilter = (ca.ak + ca.bk) >> 1;
}

}


#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

struct SincFirFilter {
    
    double bandWidth;
    double samplingRate;
    
    SincFirFilter( double bandWidth, double samplingRate ) {        
        setBandWidth( bandWidth );
        setSamplingRate( samplingRate );
    }
  
    auto setBandWidth( double bandWidth ) -> void {
        this->bandWidth = bandWidth;
    }
    
    auto setSamplingRate( double samplingRate ) -> void {
        this->samplingRate = samplingRate;
    }
    
    auto calculateBandpass(unsigned& N, double cuttOffLo, double cutoffHi) -> double* {
        
        return calculateBand( N, cuttOffLo, cutoffHi, false );
    }
    
    auto calculateBandstop(unsigned& N, double cuttOffLo, double cutoffHi) -> double* {
        
        return calculateBand( N, cuttOffLo, cutoffHi, true );
    }   
        
    auto calculateHipassInversion(unsigned& N, double cutoff) -> double* {
        
        auto samples = calculate(N, cutoff / samplingRate);
        
        for (int n = 0; n < N; n++)
            samples[n] = -1.0 * samples[n];                    
        
        samples[(N - 1) / 2] += 1;       
        
        return samples;
    }

    auto calculateHipassReversal(unsigned& N, double cutoff) -> double* {

        auto samples = calculate(N, cutoff / samplingRate);

        bool positive = true;
        for (int n = 0; n < N; n++) {
            
            if (!positive)
                samples[n] = -1 * samples[n];      
            
            positive ^= 1;
        }

        return samples;
    }
    
    auto calculateLopass(unsigned& N, double cutoff) -> double* {
        
        return calculate(N, cutoff / samplingRate);
    }
    
    auto calculate(unsigned& N, double cutoffN) -> double* {
        
        double bandWidthN = bandWidth / samplingRate;
        
        if (N == 0)
            N = (unsigned)(4.0 / bandWidthN);        
        
        if ((N % 2) == 0)
            N += 1;
        
        double windowF[N];              
        chebyshevWin( &windowF[0], N, 50.0 );
        //blackmanWin( &windowF[0], N );
        
        double* samples = new double[N];
        double _sum = 0.0;
        
        for (int n = 0; n < N; n++) {
            double x = cutoffN * ((double)n - ((double)N - 1.0) / 2.0);            
            
            if (x != 0.0)
                x = sin(M_PI * x) / (M_PI * x);
            else
                x = 1.0;
            
            samples[n] = windowF[n] * x;
            
            _sum += samples[n];
        }

        for (int n = 0; n < N; n++) {
            samples[n] /= _sum;
        }
        
        return samples;
    }

    auto calculateBand(unsigned& N, double cuttOffLo, double cutoffHi, bool stop) -> double* {

        unsigned NLo;
        unsigned NHi;

        auto samplesLo = calculateLopass(NLo, stop ? cuttOffLo : cutoffHi);

        auto samplesHi = calculateHipassInversion(NHi, stop ? cutoffHi : cuttOffLo);

        N = NHi + NLo - 1;

        auto samples = convolute(samplesLo, NLo, samplesHi, NHi);

        delete samplesLo;
        delete samplesHi;

        return samples;
    }
    
    auto blackmanWin(double* out, int N) -> void {
        
        for (unsigned n = 0; n < N; n++) {
            
            out[n] = 0.42 - 0.5 * cos(2.0 * M_PI * (double) n / ((double) N - 1.0)) +
                0.08 * cos(4 * M_PI * (double) n / ((double) N - 1.0));            
        }
    }   
    
    auto chebyshevWin(double* out, int N, double atten) -> void {
        int nn, i;
        double M, n, sum = 0, max=0;
        double tg = pow(10,atten/20);  /* 1/r term [2], 10^gamma [2] */
        double x0 = cosh((1.0/(N-1)) * acosh(tg));
        
        M = (N-1) / 2;
        
        if(N % 2 == 0)
            M = M + 0.5; /* handle even length windows */
        
        for(nn = 0; nn < (N / 2 + 1); nn++){
            n = nn-M;
            sum = 0;
            for(i=1; i<=M; i++){
                sum += chebyshevPoly(N-1, x0 * cos(M_PI * i / N)) * cos(2.0 * n * M_PI * i / N);
            }
            out[nn] = tg + 2*sum;
            out[N-nn-1] = out[nn];
            
            if(out[nn] > max)
                max=out[nn];
        }
        for( nn = 0; nn < N; nn++)
            out[nn] /= max; /* normalise */        
    }

protected:    
    
    auto chebyshevPoly(int n, double x) -> double {
        double res;
        if (fabs(x) <= 1) res = cos(n * acos(x));
        else res = cosh(n * acosh(x));
        return res;
    }

    // full convolution
    auto convolute(double* a, unsigned aSize, double* v, unsigned vSize) -> double* {

        int const N = aSize + vSize - 1;
        double* samples = new double[N];

        for (auto i = 0; i < N; ++i) {
            samples[i] = 0.0;
            
            int const jmn = (i >= vSize - 1) ? i - (vSize - 1) : 0;
            int const jmx = (i < aSize - 1) ? i : aSize - 1;

            for (auto j = jmn; j <= jmx; ++j) {
                samples[i] += a[j] * v[i - j];
            }
        }

        return samples;
    }

};
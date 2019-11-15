
#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

struct GaussianBlur {
    
    unsigned radius;  
    
    double sigma;              
    
    double* grid = nullptr;
    
    GaussianBlur( unsigned radius, double sigma ) {
        
        this->radius = radius;
        
        this->sigma = sigma;                
        
        calculate();
    }
    
    ~GaussianBlur() {
        clean();
    }
    
    auto get(unsigned distance) -> double {
        
        if (!grid)
            return 0.0;
        
        return grid[distance];
    }
    
    auto calculate() -> void {
        
        double factor = 1.0 / ( (std::sqrt(2 * M_PI) * sigma) );
        
        clean();
            
        grid = new double[radius + 1];
        
        double sum = 0.0;
        
        for (unsigned i = 0; i <= radius; i++) {
            
            double result = (-1.0 * (double(i) * double(i))) / (2.0 * sigma * sigma);
            
            grid[i] = std::exp( result ) * factor;
            
            sum += grid[i];
            
            if (i != 0)
                sum += grid[i];
        }
        
        normalise( sum );
    }    
    
    auto normalise( double sum ) -> void {
        
        for (unsigned i = 0; i <= radius; i++) {
            
            grid[i] = grid[i] / sum;
        }
    }
    
    auto clean() -> void {
        if (grid)
            delete[] grid;
            
        grid = nullptr;
    }
    
};

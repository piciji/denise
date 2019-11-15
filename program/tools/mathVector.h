
#pragma once

namespace MathVector {
    
    struct Vec2 {
        float x;
        float y;
    };
    
    
        auto Add( Vec2 a, Vec2 b) -> Vec2 {
            
            return { a.x + b.x, a.y + b.y };
        }
        
        auto Sub( Vec2 a, Vec2 b) -> Vec2 {
            
            return { a.x - b.x, a.y - b.y };
        }
        
        auto Mul( Vec2 a, Vec2 b) -> Vec2 {
            
            return { a.x * b.x, a.y * b.y };
        }
        
        auto Mul( Vec2 a, float b) -> Vec2 {
            
            return { a.x * b, a.y * b };
        }
    
    auto Dot( Vec2 a, Vec2 b ) -> float {
        
        return a.x * b.x + a.y * b.y;
    }
        
        auto Distance( Vec2 a, Vec2 b ) -> float {
            
            return sqrt( std::abs(a.x - b.x) * std::abs(a.x - b.x) + std::abs(a.y - b.y) * std::abs(a.y - b.y) );
        }
}

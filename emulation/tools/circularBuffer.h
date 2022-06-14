
// simple circular buffer, read/write single bytes.

#pragma once

template<typename T>
struct CircularBuffer {
    
    CircularBuffer() = default;

    ~CircularBuffer() {
        free();
    }

    auto size() const -> unsigned {
        return ringSize;
    }

    auto data() -> T* {
        return ring;
    }

    auto data() const -> const T* {
        return ring;
    }

    auto free() -> void {
        if (ring)
            delete[] ring;
        ring = nullptr;
    }
    
    auto reset() -> void {
        readPosition = 0;
        writePosition = 0;
    }

    auto resize(unsigned size, const T& value = {}) -> void {
                
        if (size != this->ringSize) {
            free();
            ring = new T[size];
        }        
        reset();        
        this->ringSize = size;
        
        for (unsigned i = 0; i < size; i++)
            ring[i] = value;            
    }

    auto pending() const -> bool {
        return readPosition != writePosition;
    }

    auto read() -> T {
        T result = ring[readPosition];
        
        if (++readPosition == ringSize)
            readPosition = 0;
        
        return result;
    }

    auto last() const -> T {
        return ring[writePosition];
    }

    auto write(const T& value) -> void {
        ring[writePosition] = value;
        
        if (++writePosition == ringSize)
            writePosition = 0;
    }

private:
    T* ring = nullptr;
    unsigned ringSize = 0;
    unsigned readPosition = 0;
    unsigned writePosition = 0;    
};


#pragma once

#include <cstring>
#include <cstdint>
#include <vector>

namespace Emulator {      
    
struct Serializer {
    
    enum Mode : uint8_t { Load, Save, Size };

    auto mode() const -> Mode {
        return _mode;
    }

    auto data() const -> uint8_t* {
        return _data;
    }

    auto size() const -> unsigned {
        return _size;
    }

    auto capacity() const -> unsigned {
        return _capacity;
    }
    
    virtual auto memUsage() -> bool { return false; }

    template<typename T> auto floatingpoint(T& value) -> Serializer& {
        
        unsigned typeSize = sizeof(T);

        auto p = (uint8_t*)&value;
        
        if(_mode == Save) {
            
            for( unsigned n = 0; n < typeSize; n++ )
                _data[_size++] = p[n];
            
        } else if(_mode == Load) {

            for( unsigned n = 0; n < typeSize; n++ )
                p[n] = _data[_size++];
            
        } else {
            
            _size += typeSize;
        }
        
        return *this;
    }
    
    template<typename T> auto integer(T& value) -> Serializer& {
        
        unsigned typeSize = std::is_same<bool, T>::value ? 1 : sizeof(T);
        
        if(_mode == Save) {
            T copy = value;
            for( unsigned n = 0; n < typeSize; n++ )
                _data[_size++] = copy, copy >>= 8;
            
        } else if(_mode == Load) {
            value = 0;
            
            for( unsigned n = 0; n < typeSize; n++ )
                value |= (T)_data[_size++] << (n << 3);
            
        } else if(_mode == Size) {
            _size += typeSize;
        }
        
        return *this;
    }
    
    template<typename T, unsigned N> auto array(T (&array)[N]) -> Serializer& {
        for( unsigned n = 0; n < N; n++ )
            operator()( array[n] );
        
        return *this;
    }

    template<typename T> auto array(T array, unsigned size) -> Serializer& {
        for( unsigned n = 0; n < size; n++ )
            operator()(array[n]);
        
        return *this;
    }
    
    template<typename T> auto vector( std::vector<T>& vector ) -> Serializer& {
        unsigned _vsize = vector.size();
        integer( _vsize );
        
        if (_mode == Load)
            vector.resize(_vsize);
        
        for( auto& element : vector )
            operator()( element );
        
        return *this;
    }

    template<typename T> auto operator()(T& value, typename std::enable_if<std::is_integral<T>::value>::type* = 0) -> Serializer& { return integer(value); }
    template<typename T> auto operator()(T& value, typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) -> Serializer& { return floatingpoint(value); }
    template<typename T> auto operator()(T& value, typename std::enable_if<std::is_array<T>::value>::type* = 0) -> Serializer& { return array(value); }
    template<typename T> auto operator()(T& value, unsigned size, typename std::enable_if<std::is_pointer<T>::value>::type* = 0) -> Serializer& { return array(value, size); }

    // copy object
    auto operator=(const Serializer& s) -> Serializer& {
        if(_data)
            delete[] _data;

        _mode = s._mode;
        _data = new uint8_t[s._capacity];
        _size = s._size;
        _capacity = s._capacity;

        std::memcpy(_data, s._data, s._capacity);
        return *this;
    }

    // move object
    auto operator=(Serializer&& s) -> Serializer& {
        if(_data)
            delete[] _data;

        _mode = s._mode;
        _data = s._data;
        _size = s._size;
        _capacity = s._capacity;
        
        s._data = nullptr;
        return *this;
    }

    Serializer() = default; // default constructor sets mode to 'Size'
    Serializer(const Serializer& s) { operator=(s); } // copy constructor
    Serializer(Serializer&& s) { operator=(std::forward<Serializer>(s)); } // move constructor

    Serializer(unsigned capacity) {
        _mode = Save;
        _data = new uint8_t[capacity]();
        _size = 0;
        _capacity = capacity;
    }

    Serializer(const uint8_t* data, unsigned capacity) {
        _mode = Load;
        _data = new uint8_t[capacity];
        _size = 0;
        _capacity = capacity;
        std::memcpy(_data, data, capacity);
    }

    ~Serializer() {
        if(_data)
            delete[] _data;
    }

protected:
    Mode _mode = Size;
    uint8_t* _data = nullptr;
    unsigned _size = 0;
    unsigned _capacity = 0;
};

struct MemSerializer : Serializer {

    auto memUsage() -> bool { return true; }
    
    auto setMode( Mode mode ) -> void {
        this->_mode = mode;
        this->_size = 0;
    }
    
    auto setData(unsigned capacity) -> void {
        
        if (!this->_data || (this->_capacity != capacity)) {
            if (this->_data)
                delete[] this->_data;
                
            this->_data = new uint8_t[capacity];
        }
        
        this->_capacity = capacity;
    }
}; 

}

#ifndef __BITMAP_H__
#define __BITMAP_H__
#include <cstdint>
#include <cstring>
#include <memory>
#include <iostream>
#include "utils/utils.h"
namespace cache {
  class Bitmap {
    public:
      struct Manipulator {
        Manipulator(uint8_t *data) : _data(data) {}
        bool get(uint32_t index)
        {
          return _data[index >> 3] & (1 << (index & 7));
        }

        void set(uint32_t index)
        {
          _data[index >> 3] |= (1 << (index & 7));
        }

        void clear(uint32_t index)
        {
          _data[index >> 3] &= ~(1 << (index & 7));
        }

        inline void store_bits(uint32_t b, uint32_t e, uint32_t v) {
          uint32_t base = b >> 3, base_e = e >> 3;
          uint32_t shift_b = b & 7, shift_e = e & 7;
          if (base == base_e) {
            _data[base] &= ((1 << shift_b) - 1) | (~((1 << shift_e) - 1));
            _data[base] |= v << shift_b;
          } else {
            //std::cout << "store v: " << v << std::endl;
            if (shift_b) {
              _data[base] &= (1 << shift_b) - 1;
              _data[base] |= (v << shift_b) & 0xff;
              v >>= (8 - shift_b);
              ++base;
            }
            while (base < base_e) {
              _data[base] = v & 0xff;
              v >>= 8;
              ++base;
            }
            if (shift_e) {
              _data[base] &= ~((1 << shift_e) - 1);
              _data[base] |= v;
            }
            //std::cout << "0: " << (uint32_t)_data[0] << " 1: " << (uint32_t)_data[1] << std::endl;
          }
        }

        inline void store_bits_64(uint32_t b, uint32_t e, uint64_t v) {
          uint32_t base = b >> 3, base_e = e >> 3;
          uint32_t shift_b = b & 7, shift_e = e & 7;
          if (base == base_e) {
            _data[base] &= ((1 << shift_b) - 1) | (~((1 << shift_e) - 1));
            _data[base] |= v << shift_b;
          } else {
            //std::cout << "store v: " << v << std::endl;
            if (shift_b) {
              _data[base] &= (1 << shift_b) - 1;
              _data[base] |= (v << shift_b) & 0xff;
              v >>= (8 - shift_b);
              ++base;
            }
            while (base < base_e) {
              _data[base] = v & 0xff;
              v >>= 8;
              ++base;
            }
            if (shift_e) {
              _data[base] &= ~((1 << shift_e) - 1);
              _data[base] |= v;
            }
            //std::cout << "0: " << (uint32_t)_data[0] << " 1: " << (uint32_t)_data[1] << std::endl;
          }
        }

        inline uint32_t get_bits(uint32_t b, uint32_t e) {
          uint32_t v = 0;
          uint32_t base = b >> 3, base_e = e >> 3;
          uint32_t shift_b = b & 7, shift_e = e & 7;
          if (base == base_e) {
            v = _data[base] & ~(((1 << shift_b) - 1) | (~((1 << shift_e) - 1)));
            v >>= shift_b;
          } else {
            //std::cout << "base: " << base << " base_e: " << base_e << std::endl;
            //std::cout << "shift_b: " << shift_b << " shift_e: " << shift_e << std::endl;
            uint32_t shift = 0;
            if (shift_b) {
              v |= _data[base] >> shift_b;
              shift += (8 - shift_b);
              ++base;
            }
            //std::cout << "v: " << v << " base: " << base << std::endl;
            while (base < base_e) {
              v |= _data[base] << shift;
              shift += 8;
              ++base;
            }
            v |= (_data[base] & ((1 << shift_e) - 1)) << shift;
            //std::cout << "v: " << v << " base: " << base << std::endl;
          }
          return v;
        }

        inline uint64_t get_bits_64(uint32_t b, uint32_t e) {
          uint64_t v = 0;
          uint32_t base = b >> 3, base_e = e >> 3;
          uint32_t shift_b = b & 7, shift_e = e & 7;
          if (base == base_e) {
            v = _data[base] & ~(((1 << shift_b) - 1) | (~((1 << shift_e) - 1)));
            v >>= shift_b;
          } else {
            //std::cout << "base: " << base << " base_e: " << base_e << std::endl;
            //std::cout << "shift_b: " << shift_b << " shift_e: " << shift_e << std::endl;
            uint32_t shift = 0;
            if (shift_b) {
              v |= _data[base] >> shift_b;
              shift += (8 - shift_b);
              ++base;
            }
            //std::cout << "v: " << v << " base: " << base << std::endl;
            while (base < base_e) {
              v |= _data[base] << shift;
              shift += 8;
              ++base;
            }
            v |= (_data[base] & ((1 << shift_e) - 1)) << shift;
            //std::cout << "v: " << v << " base: " << base << std::endl;
          }
          return v;
        }

        inline uint32_t get_32bits(uint32_t index) { 
          return get_bits(index * 32, (index + 1) * 32);
        }
        inline void set_32bits(uint32_t index, uint32_t v) {
          store_bits(index * 32, (index + 1) * 32, v);
        }

        uint8_t *_data;
      };
      Bitmap(uint32_t n_bits) :
        _n_bits(n_bits),
        _data(std::make_unique<uint8_t[]>((_n_bits + 7) / 8))
      {
        memset(_data.get(), 0, (_n_bits + 7) / 8);
      }

      Manipulator get_manipulator() {
        return Manipulator(_data.get());
      }

    public:
      uint32_t _n_bits;
      std::unique_ptr< uint8_t[] > _data;
  };
}
#endif

#ifndef __BUCKET_H__
#define __BUCKET_H__
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include "bitmap.h"
namespace cache {
  // Bucket is an abstraction of multiple key-value pairs (mapping)
  // bit level bucket
  class CAIndex;
  class CachePolicy;
  class CachePolicyExecutor;
  class Bucket {
    public:
      Bucket(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots,
          uint8_t *data, uint8_t *valid, CachePolicy *cache_policy, uint32_t bucket_id);
      ~Bucket();


      inline void init_k(uint32_t index, uint32_t &b, uint32_t &e)
      {
        b = index * _n_bits_per_slot;
        e = b + _n_bits_per_key;
      }
      inline uint32_t get_k(uint32_t index)
      {
        uint32_t b, e; init_k(index, b, e);
        return _data.get_bits(b, e);
      }
      inline void set_k(uint32_t index, uint32_t v)
      {
        uint32_t b, e; init_k(index, b, e);
        _data.store_bits(b, e, v);
      }
      inline void init_v(uint32_t index, uint32_t &b, uint32_t &e)
      {
        b = index * _n_bits_per_slot + _n_bits_per_key;
        e = b + _n_bits_per_value;
      }
      inline uint32_t get_v(uint32_t index)
      {
        uint32_t b, e; init_v(index, b, e);
        return _data.get_bits(b, e);
      }
      inline void set_v(uint32_t index, uint32_t v)
      {
        uint32_t b, e; init_v(index, b, e);
        _data.store_bits(b, e, v);
      }
      inline uint32_t get_data_32bits(uint32_t index)
      {
        return _data.get_32bits(index);
      }
      inline void set_data_32bits(uint32_t index, uint32_t v)
      {
        _data.set_32bits(index, v);
      }

      inline bool is_valid(uint32_t index) { return _valid.get(index); }
      inline void set_valid(uint32_t index) { _valid.set(index); }
      inline void set_invalid(uint32_t index) { _valid.clear(index); }
      inline uint32_t get_valid_32bits(uint32_t index) { return _valid.get_32bits(index); }
      inline void set_valid_32bits(uint32_t index, uint32_t v) { _valid.set_32bits(index, v); }

      inline uint32_t get_n_slots() { return _n_slots; }

      inline uint32_t get_bucket_id() { return _bucket_id; }

      Bitmap::Manipulator _data;
      Bitmap::Manipulator _valid;
      CachePolicyExecutor *_cache_policy;
      uint32_t _n_bits_per_slot, _n_slots,
               _n_bits_per_key, _n_bits_per_value;
      uint32_t _bucket_id;
  };

  /**
   *  @brief: LBABucket: store multiple entries
   *                     (lba signature -> ca hash) pair
   *
   *          layout:
   *          the key-value table (Bucket : Bitmap) (12 + 12 + x)  * 32 bits 
   *            key: 12 bit lba hash prefix (signature)
   *            value: (12 + x) bit ca hash
   *            -------------------------------------------
   *            | 12 bit signature | (12 + x) bit ca hash |
   *            -------------------------------------------
   */
  class LBABucket : public Bucket {
    public:
      LBABucket(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots,
          uint8_t *data, uint8_t *valid, CachePolicy *cache_policy, uint32_t bucket_id) :
        Bucket(n_bits_per_key, n_bits_per_value, n_slots, data, valid, cache_policy, bucket_id)
      {
      }
      /**
       * @brief Lookup the given lba signature and store the ca hash result into ca_hash
       *
       * @param lba_sig, lba signature, default 12 bit to achieve < 1% error rate
       * @param ca_hash, ca hash, used to lookup ca index
       *
       * @return ~0 if the lba signature does not exist, otherwise the corresponding index
       */
      uint32_t lookup(uint32_t lba_sig, uint32_t &ca_hash);
      void promote(uint32_t lba_sig);
      /**
       * @brief Update the lba index structure
       *        In the eviction procedure, we firstly check whether old
       *        entries has been invalid because of evictions in ca_index.
       *        LRU evict kicks in only after evicting old obsolete mappings.
       *
       * @param lba_sig
       * @param ca_hash
       * @param ca_index used to evict obselete entries that has been evicted in ca_index
       */
      void update(uint32_t lba_sig, uint32_t ca_hash, std::shared_ptr<CAIndex> ca_index);
  };

  /**
   * @brief CABucket: store multiple (ca signature -> cache device data pointer) pair
   *          Note: data pointer is computed according to alignment rather than a value
   *                to save memory usage and meantime nicely align with cache device blocks
   *        
   *        layout:
   *        1. the key-value table (Bucket : Bitmap) 12 * 32 = 384 bits (regardless of alignment)
   *          key: 12 bit ca hash value prefix
   *          value: 0 bit
   *          --------------------
   *          | 12 bit signature |
   *          --------------------
   *        2. valid bits - _valid (Bitmap) 32 bits
   */
  class CABucket : public Bucket {
    public:
      CABucket(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots,
          uint8_t *data, uint8_t *valid, CachePolicy *cache_policy, uint32_t bucket_id) :
        Bucket(n_bits_per_key, n_bits_per_value, n_slots, data, valid, cache_policy, bucket_id)
      {
      }

      /**
       * @brief Lookup the given ca signature and store the space (compression level) to size
       *
       * @param ca_sig
       * @param size
       *
       * @return ~0 if the lba signature does not exist, otherwise the corresponding index
       */
      uint32_t lookup(uint32_t ca_sig, uint32_t &n_slots_occupied);

      void promote(uint32_t ca_sig);
      /**
       * @brief Update the lba index structure
       *
       * @param lba_sig
       * @param size
       */
      uint32_t update(uint32_t ca_sig, uint32_t n_slots_to_occupy);
      // Delete an entry for a certain ca signature
      // This is required for hit but verification-failed chunk.
      void erase(uint32_t ca_sig);
  };


  class Buckets {
    public:
      Buckets(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots, uint32_t n_buckets);
      virtual ~Buckets();

      void set_cache_policy(std::unique_ptr<CachePolicy> cache_policy);
      std::mutex& get_mutex(uint32_t bucket_id) { return _mutexes[bucket_id]; }


      std::unique_ptr<Bucket> get_bucket(uint32_t bucket_id) {
        return std::move(std::make_unique<Bucket>(
            _n_bits_per_key, _n_bits_per_value, _n_slots,
            _data.get() + _n_data_bytes_per_bucket * bucket_id, 
            _valid.get() + _n_valid_bytes_per_bucket * bucket_id,
            _cache_policy.get(), bucket_id));
      }

     protected:
      uint32_t _n_bits_per_slot, _n_slots, _n_total_bytes,
               _n_bits_per_key, _n_bits_per_value,
               _n_data_bytes_per_bucket,
               _n_valid_bytes_per_bucket;
      std::unique_ptr< uint8_t[] > _data;
      std::unique_ptr< uint8_t[] > _valid;
      std::unique_ptr< CachePolicy > _cache_policy;
      std::unique_ptr< std::mutex[] > _mutexes;
  };

  class LBABuckets : public Buckets {
    public:
      LBABuckets(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots, uint32_t n_buckets);
      ~LBABuckets();

      std::unique_ptr<LBABucket> get_lba_bucket(uint32_t bucket_id)
      {
        return std::move(std::make_unique<LBABucket>(
            _n_bits_per_key, _n_bits_per_value, _n_slots,
            _data.get() + _n_data_bytes_per_bucket * bucket_id, 
            _valid.get() + _n_valid_bytes_per_bucket * bucket_id,
            _cache_policy.get(), bucket_id));
      }
  };

  class CABuckets : public Buckets {
    public:
      CABuckets(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots, uint32_t n_buckets);
      ~CABuckets();

      std::unique_ptr<CABucket> get_ca_bucket(uint32_t bucket_id) {
        return std::move(std::make_unique<CABucket>(
            _n_bits_per_key, _n_bits_per_value, _n_slots,
            _data.get() + _n_data_bytes_per_bucket * bucket_id, 
            _valid.get() + _n_valid_bytes_per_bucket * bucket_id,
            _cache_policy.get(), bucket_id));
      }

  };

  
  /**
   * @brief BlockBucket enlarges the number of slots.
   *        This is to address the random write problem. Inside one bucket,
   *        slots are grouped into "erase" blocks. Write (Persist to SSD)
   *        and Eviction is based on units of one block.
   *
   *        layout:
   *        1. Firstly we have an indexing structure from (ca signature) -> (index)
   *           This indexing structure needs to be fast:
   *           a. Binary Search Tree - O(logN) update/delete/lookup
   *           b. Sorted Array - O(N) update/delete, O(logN) lookup
   *           c. SkipList - O(logN)
   *           d. Cuckoo Hash - O(1)
   *           This indexing structure needs to be memory efficient:
   *           a. Binary Search Tree, If not balanced, the memory provision needs to be considered
   *           b. Sorted Array, no additional memory overhead
   *           c. SkipList - cannot be pointerless
   *           d. Cuckoo Hash - good candidate
   *        2. Memory Overhead (mainly inside the indexing structure)
   *           For a bucket with 0.5 Million slots (19 bits), 
   *           the signature length should be 28 bits (9 bits to tolerant the collision 1/512),
   *           the indexing pointer should be 19 bits,
   *           the compressibility level costs 2 bits,
   *           the reference count is 0.5 bits,
   *           the bitmap for validation is 1 bit
   *           in total 50.5 bits + some additional overhead for indexing structure. ~8 bytes
   *
   */
  //class BlockBucket : Bucket {
    //public:
      //BlockBucket(uint32_t n_bits_per_key, uint32_t n_bits_per_value, uint32_t n_slots);
      //~BlockBucket();

      /**
       * @brief Lookup the given ca signature and store the space (compression level) to size
       *
       * @param ca_sig
       * @param size
       *
       * @return ~0 if the lba signature does not exist, otherwise the corresponding index
       */
      //int lookup(uint32_t ca_sig, uint32_t &size);

      /**
       * @brief Update the lba index structure
       *
       * @param lba_sig
       * @param size
       */
      //void update(uint32_t ca_sig, uint32_t size);
    //private:
      //// An in-memory buffer for one block updates


      //uint32_t _current_block_no;
      //uint32_t _n_slots_per_block;
      //std::unique_ptr<uint16_t []> _reference_counts;
      //BitmapCuckooHashTable _index;
      //std::shared_ptr<IOModule> _io_module;

      //// anxiliary indexing structure to solve collisions
      //// from full CA to slot number
      //std::map<uint8_t [], uint32_t> _anxiliary_list;
  //};
}
#endif

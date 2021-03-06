//
//

#ifndef AUSTERECACHE_CACHEPOLICY_H
#define AUSTERECACHE_CACHEPOLICY_H

#include <metadata/index.h>

namespace cache {
    struct CachePolicyExecutor {
        explicit CachePolicyExecutor(Bucket *bucket);

        virtual void promote(uint32_t slotId, uint32_t nSlotsToOccupy = 1) = 0;

        virtual uint32_t allocate(uint32_t nSlotsToOccupy = 1) = 0;

        virtual void clearObsolete(std::shared_ptr <FPIndex> fpIndex) = 0;

        Bucket *bucket_;
    };
    class CachePolicy {
    public:
        virtual CachePolicyExecutor* getExecutor(Bucket *bucket) = 0;

        CachePolicy();
    };
}


#endif //AUSTERECACHE_CACHEPOLICY_H

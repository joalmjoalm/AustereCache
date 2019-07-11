#ifndef __METAVERIFICATION_H__
#define __METAVERIFICATION_H__
#include "common/common.h"
#include <cstdint>
#include "io/iomodule.h"
#include "compression/compressionmodule.h"
#include "frequentslots.h"
namespace cache {
  class MetaVerification {
   public:
    MetaVerification(std::shared_ptr<IOModule> io_module, std::shared_ptr<CompressionModule> compression_module);
    VerificationResult verify(Chunk &c);
    void update(Chunk &c);
   private:
    // In single threaded where verify and update is strictly sequential,
    // we can read the corresponding metadata during verify
    // and update it then write with io_module
    // to avoid a second metadata read in update function.
//    uint32_t _ssd_location;

    std::shared_ptr<IOModule> _io_module;
    std::unique_ptr<FrequentSlots> _frequent_slots;
    std::shared_ptr<CompressionModule> _compression_module;
  };
}
#endif

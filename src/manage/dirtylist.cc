#include "common/config.h"
#include "dirtylist.h"

#include <csignal>
#include <mutex>


namespace cache {

  DirtyList::DirtyList() : shutdown_(false)
  {
    std::thread flushThread([this] {
        while (true) {
          {
            std::unique_lock<std::mutex> l(mutex_);
            condVar_.wait(l);
            flush();
            if (shutdown_ == true) {
              break;
            }
          }
        }
        });
    flushThread.detach();
    size_ = -1;
  }

  void DirtyList::shutdown() {
    shutdown_ = true;
    condVar_.notify_all();
  }

  void DirtyList::setCompressionModule(std::shared_ptr<CompressionModule> compressionModule)
  {
    compressionModule_ = std::move(compressionModule);
  }

  DirtyList& DirtyList::getInstance() {
    static DirtyList instance;
    return instance;
  }

  void DirtyList::addLatestUpdate(uint64_t lba, uint64_t cachedataLocation, uint32_t len)
  {
    std::lock_guard<std::mutex> l(listMutex_);
    latestUpdates_[lba] = std::make_pair(cachedataLocation, len);
    if (latestUpdates_.size() >= size_) {
      condVar_.notify_all();
    }
  }

  /*
   * Description:
   *   Add a newly evicted block into the dirty list
   */
  void DirtyList::addEvictedChunk(uint64_t cachedataLocation, uint32_t len)
  {
    flushOneBlock(cachedataLocation, len);
  }
}

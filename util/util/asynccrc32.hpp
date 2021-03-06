#ifndef __UTIL_ASYNCCRC32_HPP
#define __UTIL_ASYNCCRC32_HPP

#include <memory>
#include <algorithm>
#include <cassert>
#include <string>
#include <cstdint>
#include <array>
#include <boost/thread/thread.hpp>
#include <mutex>
#include <condition_variable>
#include "util/crc32.hpp"

namespace util
{

class AsyncCRC32 : public CRC32
{
  typedef std::vector<uint8_t> DataVec;

  struct Buffer
  {
    size_t len;
    bool empty;
    DataVec data;
    
    Buffer(size_t bufferSize) : 
      len(0), empty(true)
    {
      data.resize(bufferSize);
    }
  };

  typedef std::vector<Buffer*> QueueVec;
  
  bool finished;
  unsigned pending;
  mutable std::mutex mutex;
  mutable std::condition_variable readCond;
  mutable std::condition_variable writeCond;
  QueueVec queue;
  typename QueueVec::iterator readIt;
  typename QueueVec::iterator writeIt;
  boost::thread thread;
  
  void Main()
  {
    while (true)
    {
      {
        std::unique_lock<std::mutex> lock(mutex);
        if ((*readIt)->empty)
        {
          if (finished) break;
          readCond.wait(lock);
          if (finished) break;
        }
      }
      
      CRC32::Update((*readIt)->data.data(), (*readIt)->len);

      mutex.lock();
      (*readIt)->empty = true;
      --pending;
      mutex.unlock();

      writeCond.notify_one();
      if (++readIt == queue.end()) readIt = queue.begin();
    }
  }
  
   void WaitPending() const
  {
    std::unique_lock<std::mutex> lock(mutex);
    while (pending > 0) writeCond.wait(lock);
  }
  
public:
  AsyncCRC32(size_t bufferSize, unsigned queueSize) : 
    finished(false),
    pending(0)
  {
    while (queue.size() < queueSize)
    { 
      queue.emplace_back(new Buffer(bufferSize));
    }
    
    readIt = queue.begin();
    writeIt = queue.begin();
    thread = boost::thread(&AsyncCRC32::Main, this);
  }
  
  ~AsyncCRC32()
  {
    mutex.lock();
    finished = true;
    mutex.unlock();
    
    readCond.notify_one();
    thread.join();
    for (auto buf : queue) delete buf;
  }
  
  void Update(const uint8_t* bytes, unsigned len)
  {
    assert(len <= (*writeIt)->data.size());
    
    {
      std::unique_lock<std::mutex> lock(mutex);
      while (!(*writeIt)->empty) writeCond.wait(lock);
    }

    std::copy(&bytes[0], &bytes[len], (*writeIt)->data.begin());

    (*writeIt)->len = len;

    mutex.lock();
    (*writeIt)->empty = false;    
    ++pending;
    mutex.unlock();
    
    readCond.notify_one();
    if (++writeIt == queue.end()) writeIt = queue.begin();
  }
  
  uint32_t Checksum() const
  {
    WaitPending();
    return CRC32::Checksum();
  }
  
  std::string HexString() const
  {
    WaitPending();
    return CRC32::HexString();
  }
};

} /* util namespace */

#endif

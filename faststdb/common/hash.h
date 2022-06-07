/*!
 * \file hash.h
 */
#ifndef FASTSTDB_COMMON_HASH_H_
#define FASTSTDB_COMMON_HASH_H_

#include <stdint.h>

#include <string>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "faststdb/common/rwlock.h"

namespace faststdb {
namespace common {

static inline uint64_t MurmurHash64A(const char *key, uint64_t len, uint64_t seed = 0) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t* data = (const uint64_t *)key;
  const uint64_t* end  = data + (len / 8);

  while (data != end) {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }
  const unsigned char * data2 = (const unsigned char*)data;

  switch (len & 7) {
    case 7: h ^= (uint64_t)((uint64_t)data2[6] << (uint64_t)48);
    case 6: h ^= (uint64_t)((uint64_t)data2[5] << (uint64_t)40);
    case 5: h ^= (uint64_t)((uint64_t)data2[4] << (uint64_t)32);
    case 4: h ^= (uint64_t)((uint64_t)data2[3] << (uint64_t)24);
    case 3: h ^= (uint64_t)((uint64_t)data2[2] << (uint64_t)16);
    case 2: h ^= (uint64_t)((uint64_t)data2[1] << (uint64_t)8 );
    case 1: h ^= (uint64_t)((uint64_t)data2[0]                );
            h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

class ConsistentHash {
 private:
  // hash to ip
  std::map<uint64_t, std::string> hash2ip_;
  // ip to hash
  std::map<std::string, std::vector<uint64_t>> ip2hash_;
  // disable set
  std::set<uint64_t>  disable_set_;

  // physical node ip
  std::set<std::string> physical_nodes_;
  // virtual node num per physical node
  int virtual_node_num_;

  RWLock lock_;

 public:
  explicit ConsistentHash(int virtual_node_num) : virtual_node_num_(virtual_node_num) { }

  virtual ~ConsistentHash() { }

  void Remove(const std::string& node) {
    LockGuard<RWLock, &RWLock::wrlock> lck(lock_);

    auto iter = ip2hash_.find(node);
    if (iter != ip2hash_.end()) {
      for (auto& hash : iter->second) {
        hash2ip_.erase(hash);
        disable_set_.erase(hash);
      }
    }
    ip2hash_.erase(node);
    physical_nodes_.erase(node);
  }

  void MarkEnable(const std::string& node, bool enable) {
    LockGuard<RWLock, &RWLock::wrlock> lck(lock_);

    auto iter = ip2hash_.find(node);
    if (iter != ip2hash_.end()) {
      for (auto& hash : iter->second) {
        if (enable) {
          disable_set_.erase(hash);
        } else {
          disable_set_.insert(hash);
        }
      }
    }
  }
  
  std::vector<uint64_t> GetHashs(const std::string& node) {
    LockGuard<RWLock, &RWLock::rdlock> lck(lock_);

    auto iter = ip2hash_.find(node);
    if (iter != ip2hash_.end()) {
      return iter->second;
    } else {
      static std::vector<uint64_t> kEmpty;
      return kEmpty;
    }
  }

  std::string GetNode(uint64_t hash) {
    LockGuard<RWLock, &RWLock::rdlock> lck(lock_);

    auto iter = hash2ip_.find(hash);
    if (iter == hash2ip_.end()) {
      return "";
    } else {
      return iter->second;
    }
  }

  std::tuple<uint64_t, uint64_t> GetNeighbour(uint64_t hash) {
    LockGuard<RWLock, &RWLock::rdlock> lck(lock_);

    auto iter = hash2ip_.find(hash);
    uint64_t left, right;

    if (iter == hash2ip_.end()) {
      right = hash2ip_.begin()->first;
    } else {
      iter++;
      right = iter->first;
    }

    if (iter == hash2ip_.begin()) {
      iter = hash2ip_.end();
    }
    --iter;
    left = iter->first;

    return std::make_tuple(left, right);
  }

  void AddNode(const std::string& node) {
    LockGuard<RWLock, &RWLock::wrlock> lck(lock_);
    
    std::vector<uint64_t> hashs;
    for (auto i = 0; i < virtual_node_num_; ++i) {
      std::stringstream ss;
      ss << node << "#" << i;
      auto hash = MurmurHash64A(ss.str().c_str(), ss.str().length());
      hash2ip_[hash] = node;
      hashs.push_back(hash);
    }
    ip2hash_[node] = hashs;
    physical_nodes_.insert(node);
  }

  void AddNode(const std::string& node, const std::vector<uint64_t>& hashs) {
    LockGuard<RWLock, &RWLock::wrlock> lck(lock_);
    
    for (auto& hash : hashs) {
      hash2ip_[hash] = node;
    }
    ip2hash_[node] = hashs;
    physical_nodes_.insert(node);
  }

  std::string GetKeyNode(const std::string& key) {
    LockGuard<RWLock, &RWLock::rdlock> lck(lock_);
    
    auto hash = MurmurHash64A(key.c_str(), key.length());

    if (disable_set_.size() >= hash2ip_.size()) {
      return "";
    }
    auto iter = hash2ip_.lower_bound(hash);
    do {
      if (iter == hash2ip_.end()) iter = hash2ip_.begin();
      if (!disable_set_.count(iter->first)) break;
      ++iter;
    } while (true);
    return iter->second;
  }
};

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_HASH_H_

#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#include "log_duration.h"
//#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {

    struct Bucket {
        std::mutex bucket_mut;
        std::map<Key, Value> bucket_map;
        void Erase(const Key& key) {
            bucket_map.erase(key);
        }
    };

public:

    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.bucket_mut)
            , ref_to_value(bucket.bucket_map[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        :buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    void Erase(const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        bucket.Erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [bucket_mut, bucket_map] : buckets_) {
            std::lock_guard g(bucket_mut);
            result.insert(bucket_map.begin(), bucket_map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};
#pragma once
#include <string>
#include <mutex>
#include <map>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
    };

    [[maybe_unused]] explicit ConcurrentMap(size_t bucket_count)
            : buckets_(bucket_count) {
    };

    Access operator[](const Key& key) {
        auto& bucket = buckets_[GetIndex(key)];
        return {std::lock_guard<std::mutex>(bucket.mut), bucket.data[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& bucket : buckets_) {
            std::lock_guard<std::mutex> lock (bucket.mut);
            for (auto& [key, value] : bucket.data) {
                result[key] = value;
            }
        }
        return result;
    }

    void Erase(const Key& key) {
        auto& bucket = buckets_[GetIndex(key)];
        std::lock_guard<std::mutex> bucketLock(bucket.mut);
        bucket.data.erase(key);
    }

    typename std::map<Key, Value>::iterator begin() {
        std::lock_guard<std::mutex> lock(mutex);
        if (buckets_.empty()) {
            return typename std::map<Key, Value>::iterator();
        }
        std::lock_guard<std::mutex> bucketLock(buckets_[0].mut);
        return buckets_[0].data.begin();
    }

    typename std::map<Key, Value>::iterator end() {
        std::lock_guard<std::mutex> lock(mutex);
        if (buckets_.empty()) {
            return typename std::map<Key, Value>::iterator();
        }
        std::lock_guard<std::mutex> bucketLock(buckets_[buckets_.size() - 1].mut);
        return buckets_[buckets_.size() - 1].data.end();
    }

private:
    struct Bucket {
        std::mutex mut;
        std::map<Key, Value> data;
    };

    std::vector<Bucket> buckets_;
    std::mutex mutex;

    int GetIndex(const Key& key) {
        return static_cast<std::uint64_t>(key) % buckets_.size();
    }
};

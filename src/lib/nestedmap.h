#ifndef lava_NESTEDMAP_H
#define lava_NESTEDMAP_H

// reference: xstl https://github.com/MaxXSoft/XSTL


#include <memory>
#include <unordered_map>

namespace lava::lib {

  namespace Nested {
    template<typename K, typename V>
    class NestedMap;

    template<typename K, typename V>
    using NestedMapPtr = std::shared_ptr<NestedMap<K, V>>;

    template<typename K, typename V>
    class NestedMap {
    private:
      std::unordered_map<K, V> map_;
      NestedMapPtr<K, V> outer_;

    public:
      NestedMap() : outer_(nullptr) {};

      explicit NestedMap(const NestedMapPtr<K, V> &Outer) : outer_(Outer) {}

      // add item to current map
      void AddItem(const K &key, const V &value) {
        map_.insert({key, value});
      }

      // get item
      V GetItem(const K &key, bool recursive) const;

      V GetItem(const K &key) const { return GetItem(key, true); }

      // replace item
      void Replace(const K &key, const V &value) {
        auto it = this->map_.find(key);
        if (it != this->map_.end()) {
           map_[key] = value;
        } else if (this->outer_) {
           this->outer_->Replace(key, value);
        } else {
          DBG_ASSERT(0, "can't find this key");
        }
      }

      // remove item
      bool RemoveItem(const K &key, bool recursive);

      bool RemoveItem(const K &key) {
        return RemoveItem(key, true);
      }

      // access item
      V &AccessItem(const K &key) { return map_[key]; }

      // access operator
      V &operator[](const K &key) { return map_[key]; }

      // getters
      // outer map
      const NestedMapPtr<K, V> &outer() const { return outer_; }

      // check if current map is root map
      bool is_root() const { return outer_ == nullptr; }
    };

    template<typename K, typename V>
    V NestedMap<K, V>::GetItem(const K &key, bool recursive) const {
      auto it = this->map_.find(key);
      if (it != this->map_.end()) {
        return it->second;
      } else if (this->outer_ && recursive) {
        return this->outer_->GetItem(key, true);
      } else {
        return nullptr;
      }
    }

    template<typename K, typename V>
    bool NestedMap<K, V>::RemoveItem(const K &key, bool recursive) {
      auto num = map_.erase(key);
      if (num) {
        return true;
      } else if (outer_ && recursive) {
        return outer_->RemoveItem(key, true);
      } else {
        return false;
      }
    }


  }

  // create a new nested map
  template<typename K, typename V>
  Nested::NestedMapPtr<K, V> MakeNestedMap() {
    return std::make_shared<Nested::NestedMap<K, V>>();
  }

  // create a new nested map (with outer map)
  template<typename K, typename V>
  Nested::NestedMapPtr<K, V> MakeNestedMap(Nested::NestedMapPtr<K, V> &outer) {
    return std::make_shared<Nested::NestedMap<K, V>>(outer);
  }

}

#endif //lava_NESTEDMAP_H

/*
 * concurrent_box.h
 *
 *  Created on: Apr 18, 2013
 *      Author: vincent
 */

#ifndef ATLAS_CONCURRENT_BOX_H_
#define ATLAS_CONCURRENT_BOX_H_

#include <unordered_map>
#include <mutex>

namespace atlas {

  template<typename Key, typename Value, typename AssocContainer = std::unordered_map<Key, Value>>
  class concurrent_box {
  public:

    typedef Key key_type;
    typedef Value value_type;
    typedef AssocContainer assoc_container;

  public:

    void put(const key_type& key, const value_type& value) {
      std::lock_guard<std::mutex> guard(_mutex);

      _container.insert(std::make_pair(key, value));
    }

    boost::optional<value_type> get(const key_type& key) const {
      std::lock_guard<std::mutex> guard(_mutex);

      auto it = _container.find(key);
      if (it != _container.end()) return *it;

      return boost::none;
    }

    boost::optional<value_type> take(const key_type& key) {
      std::lock_guard<std::mutex> guard(_mutex);

      auto it = _container.find(key);
      if (it == _container.end()) return boost::none;

      boost::optional<value_type> value = *it;
      _container.erase(it);

      return value;
    }

    boost::optional<value_type> random_take() {
      std::random_device rd;
      std::mt19937 gen(rd());

      std::lock_guard<std::mutex> guard(_mutex);
      std::uniform_int_distribution<> dis(0, _container.size());

      auto it = _container.begin();
      std::advance(it, dis(gen));
      if (it == _container.end()) return boost::none;

      boost::optional<value_type> value = *it;
      _container.erase(it);

      return value;
    }

    boost::optional<value_type> top() {
      std::lock_guard<std::mutex> guard(_mutex);
      if (_container.empty()) return boost::none;

      boost::optional<value_type> value = _container.begin();

      return value;
    }

    boost::optional<value_type> pop() {
      std::lock_guard<std::mutex> guard(_mutex);
      if (_container.empty()) return boost::none;

      boost::optional<value_type> value = _container.begin();
      _container.erase(_container.begin());

      return value;
    }

    void erase(const key_type& key) {
      std::lock_guard<std::mutex> guard(_mutex);

      _container.erase(key);
    }

    void clear() {
      std::lock_guard<std::mutex> guard(_mutex);
      _container.clear();
    }

    size_t size() const {
      std::lock_guard<std::mutex> guard(_mutex);
      return _container.size();
    }

    bool empty() const {
      std::lock_guard<std::mutex> guard(_mutex);
      return _container.empty();
    }

    template<typename F>
    void for_each(F&& f) {
      std::lock_guard<std::mutex> guard(_mutex);
      std::for_each(_container.begin(), _container.end(), f);
    }

  private:

    mutable std::mutex _mutex;
    assoc_container _container;
  };

} // atlas

#endif /* CONCURRENT_BOX_H_ */

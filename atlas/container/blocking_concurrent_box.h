/*
 * blocking_concurrent_box.h
 *
 *  Created on: Apr 18, 2013
 *      Author: vincent
 */

#ifndef ATLAS_BLOCKING_CONCURRENT_BOX_H_
#define ATLAS_BLOCKING_CONCURRENT_BOX_H_

#include <unordered_map>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include <boost/optional.hpp>

namespace atlas {

  template<typename Key, typename Value, typename AssocContainer = std::unordered_map<Key, Value>>
  class blocking_concurrent_box {
  public:

    typedef Key key_type;
    typedef Value value_type;
    typedef AssocContainer assoc_container;

  public:

    blocking_concurrent_box(std::chrono::microseconds time) : _wait_time(time) {}

  public:

    void put(const key_type& key, const value_type& value) {
      {
        std::unique_lock<std::mutex> lock(_mutex);
        _container.insert(std::make_pair(key, value));
      }

      _not_empty.notify_one();
    }

    boost::optional<value_type> get(const key_type& key) const {
      std::unique_lock<std::mutex> lock(_mutex);

      auto it = _container.find(key);
      if (it != _container.end()) return *it;

      return boost::none;
    }

    boost::optional<value_type> take(const key_type& key) {
      std::unique_lock<std::mutex> lock(_mutex);
      _not_empty.wait_for(lock, _wait_time, [this]() { return !_container.empty(); });

      auto it = _container.find(key);
      if (it == _container.end()) return boost::none;

      boost::optional<value_type> value = it->second;
      _container.erase(it);

      return value;
    }

    // TODO : optimization required
    boost::optional<value_type> random_take() {
      std::unique_lock<std::mutex> lock(_mutex);
      _not_empty.wait_for(lock, _wait_time, [this]() { return !_container.empty(); });

      auto it = _container.begin();
      std::advance(it, get_random_index());
      if (it == _container.end()) return boost::none;

      boost::optional<value_type> value = it->second; // move
      _container.erase(it);

      return value;
    }

    boost::optional<value_type> top() {
      std::lock_guard<std::mutex> lock(_mutex);
      if (_container.empty()) return boost::none;

      boost::optional<value_type> value = _container.begin();

      return value;
    }

    boost::optional<value_type> pop() {
      std::unique_lock<std::mutex> lock(_mutex);
      _not_empty.wait_for(lock, _wait_time, [this]() { return !_container.empty(); });

      if (_container.empty()) return boost::none;

      boost::optional<value_type> value = _container.begin();
      _container.erase(_container.begin());

      return value;
    }

    void erase(const key_type& key) {
      std::lock_guard<std::mutex> lock(_mutex);
      _container.erase(key);
    }

    void clear() {
      std::lock_guard<std::mutex> lock(_mutex);
      _container.clear();
    }

    size_t size() const {
      std::lock_guard<std::mutex> lock(_mutex);
      return _container.size();
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(_mutex);
      return _container.empty();
    }

    template<typename F>
    void for_each(F&& f) {
      std::lock_guard<std::mutex> lock(_mutex);
      std::for_each(_container.begin(), _container.end(), f);
    }

  private:

    size_t get_random_index() {
      std::random_device rd;
      std::mt19937 gen(rd());

      std::uniform_int_distribution<> dis(0, _container.size());

      return dis(gen);
    }

  private:

    std::chrono::microseconds _wait_time;

    mutable std::mutex _mutex;
    mutable std::condition_variable _not_empty;

    assoc_container _container;
  };

} // atlas

#endif /* ATLAS_BLOCKING_CONCURRENT_BOX_H_ */

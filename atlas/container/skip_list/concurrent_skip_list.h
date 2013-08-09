/*
 * Copyright 2013 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author: Xin Liu <xliux@fb.com>
//
// A concurrent skip list (CSL) implementation.
// Ref: http://www.cs.tau.ac.il/~shanir/nir-pubs-web/Papers/OPODIS2006-BA.pdf

#ifndef FOLLY_CONCURRENT_SKIP_LIST_H_
#define FOLLY_CONCURRENT_SKIP_LIST_H_

#include <algorithm>
#include <climits>
#include <type_traits>
#include <utility>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>

#include <boost/iterator/iterator_facade.hpp>

#include <atlas/likely.h>
#include <atlas/lock.h>

namespace atlas {

  template<typename T, typename Comp = std::less<T>, int MAX_HEIGHT = 24>
  class concurrent_skip_list {

    // MAX_HEIGHT needs to be at least 2 to suppress compiler
    // warnings/errors (Werror=uninitialized tiggered due to preds_[1]
    // being treated as a scalar in the compiler).
    static_assert(MAX_HEIGHT >= 2 && MAX_HEIGHT < 64, "MAX_HEIGHT can only be in the range of [2, 64)");

    typedef std::unique_lock<micro_spin_lock> scoped_locker;
    typedef concurrent_skip_list<T, Comp, MAX_HEIGHT> skip_list_type;

  public:

    typedef detail::SkipListNode<T> NodeType;
    typedef T value_type;
    typedef T key_type;

    typedef detail::csl_iterator<value_type, NodeType> iterator;
    typedef detail::csl_iterator<const value_type, const NodeType> const_iterator;

    class Accessor;
    class Skipper;

    // convenient function to get an Accessor to a new instance.
    static Accessor create(int height = 1) {
      return Accessor(createInstance(height));
    }

    // create a shared_ptr skiplist object with initial head height.
    static std::shared_ptr<skip_list_type> createInstance(int height = 1) {
      return std::shared_ptr<skip_list_type>(new skip_list_type(height));
    }

    // create a unique_ptr skiplist object with initial head height.
    static std::unique_ptr<skip_list_type> createRawInstance(int height = 1) {
      return std::unique_ptr<skip_list_type>(new skip_list_type(height));
    }

    //===================================================================
    // Below are implementation details.
    // Please see concurrent_skip_list::Accessor for stdlib-like APIs.
    //===================================================================

    ~concurrent_skip_list() {
      // CHECK_EQ(recycler_.refs(), 0);
      while (NodeType* current = head_.load(std::memory_order_relaxed)) {
        NodeType* tmp = current->skip(0);
        NodeType::destroy(current);
        head_.store(tmp, std::memory_order_relaxed);
      }
    }

  private:

    static bool greater(const value_type &data, const NodeType *node) {
      return node && Comp()(node->data(), data);
    }

    static bool less(const value_type &data, const NodeType *node) {
      return (node == nullptr) || Comp()(data, node->data());
    }

    static int findInsertionPoint(NodeType *cur, int cur_layer, const value_type &data, NodeType *preds[],
        NodeType *succs[]) {
      int foundLayer = -1;
      NodeType *pred = cur;
      NodeType *foundNode = nullptr;
      for (int layer = cur_layer; layer >= 0; --layer) {
        NodeType *node = pred->skip(layer);
        while (greater(data, node)) {
          pred = node;
          node = node->skip(layer);
        }
        if (foundLayer == -1 && !less(data, node)) { // the two keys equal
          foundLayer = layer;
          foundNode = node;
        }
        preds[layer] = pred;

        // if found, succs[0..foundLayer] need to point to the cached foundNode,
        // as foundNode might be deleted at the same time thus pred->skip() can
        // return NULL or another node.
        succs[layer] = foundNode ? foundNode : node;
      }
      return foundLayer;
    }

    struct Recycler: private boost::noncopyable {
      Recycler() : refs_(0), dirty_(false) { lock_.init(); }

      ~Recycler() {
        if (nodes_) {
          for (auto& node : *nodes_) {
            NodeType::destroy(node);
          }
        }
      }

      void add(NodeType* node) {
        std::lock_guard<micro_spin_lock> g(lock_);
        if (nodes_.get() == nullptr) { nodes_.reset(new std::vector<NodeType*>(1, node)); }
        else { nodes_->push_back(node); }

        // DCHECK_GT(refs(), 0);
        dirty_.store(true, std::memory_order_relaxed);
      }

      int refs() const { return refs_.load(std::memory_order_relaxed); }

      int addRef() { return refs_.fetch_add(1, std::memory_order_relaxed); }

      int release() {
        // We don't expect to clean the recycler immediately everytime it is OK
        // to do so. Here, it is possible that multiple accessors all release at
        // the same time but nobody would clean the recycler here. If this
        // happens, the recycler will usually still get cleaned when
        // such a race doesn't happen. The worst case is the recycler will
        // eventually get deleted along with the skiplist.
        if (likely(!dirty_.load(std::memory_order_relaxed) || refs() > 1)) {
          return refs_.fetch_add(-1, std::memory_order_relaxed);
        }

        std::unique_ptr<std::vector<NodeType*> > newNodes;
        {
          std::lock_guard<micro_spin_lock> g(lock_);
          if (nodes_.get() == nullptr || refs() > 1) {
            return refs_.fetch_add(-1, std::memory_order_relaxed);
          }
          // once refs_ reaches 1 and there is no other accessor, it is safe to
          // remove all the current nodes in the recycler, as we already acquired
          // the lock here so no more new nodes can be added, even though new
          // accessors may be added after that.
          newNodes.swap(nodes_);
          dirty_.store(false, std::memory_order_relaxed);
        }

        // TODO(xliu) should we spawn a thread to do this when there are large
        // number of nodes in the recycler?
        for (auto& node : *newNodes) {
          NodeType::destroy(node);
        }

        // decrease the ref count at the very end, to minimize the
        // chance of other threads acquiring lock_ to clear the deleted
        // nodes again.
        return refs_.fetch_add(-1, std::memory_order_relaxed);
      }

    private:

      std::unique_ptr<std::vector<NodeType*>> nodes_;
      std::atomic<int32_t> refs_; // current number of visitors to the list
      std::atomic<bool> dirty_; // whether *nodes_ is non-empty
      micro_spin_lock lock_; // protects access to *nodes_
    };  // class concurrent_skip_list::Recycler

    explicit concurrent_skip_list(int height) : head_(NodeType::create(height, value_type(), true)), size_(0) {}

    size_t size() const { return size_.load(std::memory_order_relaxed); }
    int height() const { return head_.load(std::memory_order_consume)->height(); }
    int maxLayer() const { return height() - 1; }

    size_t incrementSize(int delta) { return size_.fetch_add(delta, std::memory_order_relaxed) + delta; }

    // Returns the node if found, nullptr otherwise.
    NodeType* find(const value_type &data) {
      auto ret = findNode(data);
      if (ret.second && !ret.first->markedForRemoval()) return ret.first;
      return nullptr;
    }

    // lock all the necessary nodes for changing (adding or removing) the list.
    // returns true if all the lock acquried successfully and the related nodes
    // are all validate (not in certain pending states), false otherwise.
    bool lockNodesForChange(int nodeHeight, scoped_locker guards[MAX_HEIGHT], NodeType *preds[MAX_HEIGHT],
        NodeType *succs[MAX_HEIGHT], bool adding = true) {
      NodeType *pred, *succ, *prevPred = nullptr;
      bool valid = true;
      for (int layer = 0; valid && layer < nodeHeight; ++layer) {
        pred = preds[layer];
        // DCHECK(pred != nullptr) << "layer=" << layer << " height=" << height() << " nodeheight=" << nodeHeight;
        succ = succs[layer];
        if (pred != prevPred) {
          guards[layer] = pred->acquireGuard();
          prevPred = pred;
        }
        valid = !pred->markedForRemoval() && pred->skip(layer) == succ;  // check again after locking

        if (adding) {  // when adding a node, the succ shouldn't be going away
          valid = valid && (succ == nullptr || !succ->markedForRemoval());
        }
      }

      return valid;
    }

    // Returns a paired value:
    //   pair.first always stores the pointer to the node with the same input key.
    //     It could be either the newly added data, or the existed data in the
    //     list with the same key.
    //   pair.second stores whether the data is added successfully:
    //     0 means not added, otherwise reutrns the new size.
    template<typename U>
    std::pair<NodeType*, size_t> addOrGetData(U &&data) {
      NodeType *preds[MAX_HEIGHT], *succs[MAX_HEIGHT];
      NodeType *newNode;
      size_t newSize;
      while (true) {
        int max_layer = 0;
        int layer = findInsertionPointGetMaxLayer(data, preds, succs, &max_layer);

        if (layer >= 0) {
          NodeType *nodeFound = succs[layer];
          // DCHECK(nodeFound != nullptr);
          if (nodeFound->markedForRemoval()) {
            continue;  // if it's getting deleted retry finding node.
          }

          // wait until fully linked.
          while (unlikely(!nodeFound->fullyLinked())) {}

          return std::make_pair(nodeFound, 0);
        }

        // need to capped at the original height -- the real height may have grown
        int nodeHeight = detail::SkipListRandomHeight::ref()->
        getHeight(max_layer + 1);

        scoped_locker guards[MAX_HEIGHT];
        if (!lockNodesForChange(nodeHeight, guards, preds, succs)) {
          continue; // give up the locks and retry until all valid
        }

        // locks acquired and all valid, need to modify the links under the locks.
        newNode = NodeType::create(nodeHeight, std::forward<U>(data));
        for (int layer = 0; layer < nodeHeight; ++layer) {
          newNode->setSkip(layer, succs[layer]);
          preds[layer]->setSkip(layer, newNode);
        }

        newNode->setFullyLinked();
        newSize = incrementSize(1);
        break;
      }

      int hgt = height();
      size_t sizeLimit =
      detail::SkipListRandomHeight::ref()->getSizeLimit(hgt);

      if (hgt < MAX_HEIGHT && newSize > sizeLimit) {
        growHeight(hgt + 1);
      }

      // CHECK_GT(newSize, 0);
      return std::make_pair(newNode, newSize);
    }

    bool remove(const value_type &data) {
      NodeType *nodeToDelete = nullptr;
      scoped_locker nodeGuard;
      bool isMarked = false;
      int nodeHeight = 0;
      NodeType* preds[MAX_HEIGHT], *succs[MAX_HEIGHT];

      while (true) {
        int max_layer = 0;
        int layer = findInsertionPointGetMaxLayer(data, preds, succs, &max_layer);
        if (!isMarked && (layer < 0 || !okToDelete(succs[layer], layer))) {
          return false;
        }

        if (!isMarked) {
          nodeToDelete = succs[layer];
          nodeHeight = nodeToDelete->height();
          nodeGuard = nodeToDelete->acquireGuard();
          if (nodeToDelete->markedForRemoval()) return false;
          nodeToDelete->setMarkedForRemoval();
          isMarked = true;
        }

        // acquire pred locks from bottom layer up
        scoped_locker guards[MAX_HEIGHT];
        if (!lockNodesForChange(nodeHeight, guards, preds, succs, false)) {
          continue;  // this will unlock all the locks
        }

        for (int layer = nodeHeight - 1; layer >= 0; --layer) {
          preds[layer]->setSkip(layer, nodeToDelete->skip(layer));
        }

        incrementSize(-1);
        break;
      }
      recycle(nodeToDelete);
      return true;
    }

    const value_type *first() const {
      auto node = head_.load(std::memory_order_consume)->skip(0);
      return node ? &node->data() : nullptr;
    }

    const value_type *last() const {
      NodeType *pred = head_.load(std::memory_order_consume);
      NodeType *node = nullptr;

      for (int layer = maxLayer(); layer >= 0; --layer) {
        do {
          node = pred->skip(layer);
          if (node) pred = node;
        }while (node != nullptr);
      }

      return pred == head_.load(std::memory_order_relaxed) ? nullptr : &pred->data();
    }

    static bool okToDelete(NodeType *candidate, int layer) {
      // DCHECK(candidate != nullptr);
      return candidate->fullyLinked() && candidate->maxLayer() == layer && !candidate->markedForRemoval();
    }

    // find node for insertion/deleting
    int findInsertionPointGetMaxLayer(const value_type &data, NodeType *preds[], NodeType *succs[], int *max_layer) const {
      *max_layer = maxLayer();
      return findInsertionPoint(head_.load(std::memory_order_consume), *max_layer, data, preds, succs);
    }

    // Find node for access. Returns a paired values:
    // pair.first = the first node that no-less than data value
    // pair.second = 1 when the data value is founded, or 0 otherwise.
    // This is like lower_bound, but not exact: we could have the node marked for
    // removal so still need to check that.
    std::pair<NodeType*, int> findNode(const value_type &data) const {
      return findNodeDownRight(data);
    }

    // Find node by first stepping down then stepping right. Based on benchmark
    // results, this is slightly faster than findNodeRightDown for better
    // localality on the skipping pointers.
    std::pair<NodeType*, int> findNodeDownRight(const value_type &data) const {
      NodeType *pred = head_.load(std::memory_order_consume);
      int ht = pred->height();
      NodeType *node = nullptr;

      bool found = false;
      while (!found) {
        // stepping down
        for (; ht > 0 && less(data, pred->skip(ht - 1)); --ht) {}
        if (ht == 0) return std::make_pair(pred->skip(0), 0);  // not found

        node = pred->skip(--ht);// node <= data now
        // stepping right
        while (greater(data, node)) {
          pred = node;
          node = node->skip(ht);
        }
        found = !less(data, node);
      }
      return std::make_pair(node, found);
    }

    // find node by first stepping right then stepping down.
    // We still keep this for reference purposes.
    std::pair<NodeType*, int> findNodeRightDown(const value_type &data) const {
      NodeType *pred = head_.load(std::memory_order_consume);
      NodeType *node = nullptr;
      auto top = maxLayer();
      int found = 0;
      for (int layer = top; !found && layer >= 0; --layer) {
        node = pred->skip(layer);
        while (greater(data, node)) {
          pred = node;
          node = node->skip(layer);
        }
        found = !less(data, node);
      }
      return std::make_pair(node, found);
    }

    NodeType* lower_bound(const value_type &data) const {
      auto node = findNode(data).first;
      while (node != nullptr && node->markedForRemoval()) {
        node = node->skip(0);
      }
      return node;
    }

    void growHeight(int height) {
      NodeType* oldHead = head_.load(std::memory_order_consume);
      if (oldHead->height() >= height) {  // someone else already did this
        return;
      }

      NodeType* newHead = NodeType::create(height, value_type(), true);

      { // need to guard the head node in case others are adding/removing
        // nodes linked to the head.
        scoped_locker g = oldHead->acquireGuard();
        newHead->copyHead(oldHead);
        NodeType* expected = oldHead;
        if (!head_.compare_exchange_strong(expected, newHead,
                std::memory_order_release)) {
          // if someone has already done the swap, just return.
          NodeType::destroy(newHead);
          return;
        }
        oldHead->setMarkedForRemoval();
      }
      recycle(oldHead);
    }

    void recycle(NodeType *node) {
      recycler_.add(node);
    }

    private:

    std::atomic<NodeType*> head_;
    Recycler recycler_;
    std::atomic<size_t> size_;
  };

  template<typename T, typename Comp, int MAX_HEIGHT>
  class concurrent_skip_list<T, Comp, MAX_HEIGHT>::Accessor {
    typedef detail::SkipListNode<T> NodeType;
    typedef concurrent_skip_list<T, Comp, MAX_HEIGHT> skip_list_type;

  public:

    typedef T value_type;
    typedef T key_type;
    typedef T& reference;
    typedef T* pointer;
    typedef const T& const_reference;
    typedef const T* const_pointer;
    typedef size_t size_type;
    typedef Comp key_compare;
    typedef Comp value_compare;

    typedef typename skip_list_type::iterator iterator;
    typedef typename skip_list_type::const_iterator const_iterator;
    typedef typename skip_list_type::Skipper Skipper;

    explicit Accessor(std::shared_ptr<skip_list_type> skip_list) :
        slHolder_(std::move(skip_list)) {
      sl_ = slHolder_.get();
      // DCHECK(sl_ != nullptr);
      sl_->recycler_.addRef();
    }

    // Unsafe initializer: the caller assumes the responsibility to keep
    // skip_list valid during the whole life cycle of the Acessor.
    explicit Accessor(concurrent_skip_list *skip_list) : sl_(skip_list) {
      // DCHECK(sl_ != nullptr);
      sl_->recycler_.addRef();
    }

    Accessor(const Accessor &accessor) : sl_(accessor.sl_), slHolder_(accessor.slHolder_) {
      sl_->recycler_.addRef();
    }

    Accessor& operator=(const Accessor &accessor) {
      if (this != &accessor) {
        slHolder_ = accessor.slHolder_;
        sl_->recycler_.release();
        sl_ = accessor.sl_;
        sl_->recycler_.addRef();
      }
      return *this;
    }

    ~Accessor() { sl_->recycler_.release(); }

    bool empty() const { return sl_->size() == 0; }
    size_t size() const { return sl_->size(); }
    size_type max_size() const { return std::numeric_limits<size_type>::max(); }

    // returns end() if the value is not in the list, otherwise returns an
    // iterator pointing to the data, and it's guaranteed that the data is valid
    // as far as the Accessor is hold.
    iterator find(const key_type &value) { return iterator(sl_->find(value)); }
    const_iterator find(const key_type &value) const { return iterator(sl_->find(value)); }
    size_type count(const key_type &data) const { return contains(data); }

    iterator begin() const {
      NodeType* head = sl_->head_.load(std::memory_order_consume);
      return iterator(head->next());
    }

    iterator end() const { return iterator(nullptr); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    template<typename U, typename = typename std::enable_if<std::is_convertible<U, T>::value>::type>
    std::pair<iterator, bool> insert(U&& data) {
      auto ret = sl_->addOrGetData(std::forward<U>(data));
      return std::make_pair(iterator(ret.first), ret.second);
    }
    size_t erase(const key_type &data) {return remove(data);}

    iterator lower_bound(const key_type &data) const { return iterator(sl_->lower_bound(data)); }

    size_t height() const {return sl_->height();}

    // first() returns pointer to the first element in the skiplist, or
    // nullptr if empty.
    //
    // last() returns the pointer to the last element in the skiplist,
    // nullptr if list is empty.
    //
    // Note: As concurrent writing can happen, first() is not
    //   guaranteed to be the min_element() in the list. Similarly
    //   last() is not guaranteed to be the max_element(), and both of them can
    //   be invalid (i.e. nullptr), so we name them differently from front() and
    //   tail() here.
    const key_type *first() const {return sl_->first();}
    const key_type *last() const {return sl_->last();}

    // Try to remove the last element in the skip list.
    //
    // Returns true if we removed it, false if either the list is empty
    // or a race condition happened (i.e. the used-to-be last element
    // was already removed by another thread).
    bool pop_back() {
      auto last = sl_->last();
      return last ? sl_->remove(*last) : false;
    }

    std::pair<key_type*, bool> addOrGetData(const key_type &data) {
      auto ret = sl_->addOrGetData(data);
      return std::make_pair(&ret.first->data(), ret.second);
    }

    skip_list_type* skiplist() const {return sl_;}

    // legacy interfaces
    // TODO:(xliu) remove these.
    // Returns true if the node is added successfully, false if not, i.e. the
    // node with the same key already existed in the list.
    bool contains(const key_type &data) const {return sl_->find(data);}
    bool add(const key_type &data) {return sl_->addOrGetData(data).second;}
    bool remove(const key_type &data) {return sl_->remove(data);}

  private:
    skip_list_type *sl_;
    std::shared_ptr<skip_list_type> slHolder_;
  };

// implements forward iterator concept.
  template<typename ValT, typename NodeT>
  class detail::csl_iterator: public boost::iterator_facade<csl_iterator<ValT, NodeT>, ValT,
      boost::forward_traversal_tag> {
  public:
    typedef ValT value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef ptrdiff_t difference_type;

    explicit csl_iterator(NodeT* node = nullptr) : node_(node) { }

    template<typename OtherVal, typename OtherNode>
    csl_iterator(const csl_iterator<OtherVal, OtherNode> &other,
        typename std::enable_if<std::is_convertible<OtherVal, ValT>::value>::type* = 0) :
        node_(other.node_) {
    }

    size_t nodeSize() const { return node_ == nullptr ? 0 : node_->height() * sizeof(NodeT*) + sizeof(*this); }

    bool good() const { return node_ != nullptr; }

  private:

    friend class boost::iterator_core_access;
    template<class, class > friend class csl_iterator;

    void increment() { node_ = node_->next(); }

    bool equal(const csl_iterator& other) const { return node_ == other.node_; }

    value_type& dereference() const { return node_->data(); }

    NodeT* node_;
  };

  // Skipper interface
  template<typename T, typename Comp, int MAX_HEIGHT>
  class concurrent_skip_list<T, Comp, MAX_HEIGHT>::Skipper {

    typedef detail::SkipListNode<T> NodeType;
    typedef concurrent_skip_list<T, Comp, MAX_HEIGHT> skip_list_type;
    typedef typename skip_list_type::Accessor Accessor;

  public:

    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef ptrdiff_t difference_type;

    Skipper(const std::shared_ptr<skip_list_type>& skipList) : accessor_(skipList) { init(); }

    Skipper(const Accessor& accessor) : accessor_(accessor) { init(); }

    void init() {
      // need to cache the head node
      NodeType* head_node = head();
      headHeight_ = head_node->height();
      for (int i = 0; i < headHeight_; ++i) {
        preds_[i] = head_node;
        succs_[i] = head_node->skip(i);
      }

      int max_layer = maxLayer();
      for (int i = 0; i < max_layer; ++i) {
        hints_[i] = i + 1;
      }

      hints_[max_layer] = max_layer;
    }

    // advance to the next node in the list.
    Skipper& operator ++() {
      preds_[0] = succs_[0];
      succs_[0] = preds_[0]->skip(0);
      int height = curHeight();
      for (int i = 1; i < height && preds_[0] == succs_[i]; ++i) {
        preds_[i] = succs_[i];
        succs_[i] = preds_[i]->skip(i);
      }

      return *this;
    }

    bool good() const { return succs_[0] != nullptr; }

    int maxLayer() const { return headHeight_ - 1; }

    int curHeight() const {
      // need to cap the height to the cached head height, as the current node
      // might be some newly inserted node and also during the time period the
      // head height may have grown.
      return succs_[0] ? std::min(headHeight_, succs_[0]->height()) : 0;
    }

    const value_type &data() const { return succs_[0]->data(); }
    value_type &operator *() const { return succs_[0]->data(); }
    value_type *operator->() { return &succs_[0]->data(); }

    /*
     * Skip to the position whose data is no less than the parameter.
     * (I.e. the lower_bound).
     *
     * Returns true if the data is found, false otherwise.
     */
    bool to(const value_type &data) {
      int layer = curHeight() - 1;
      if (layer < 0) return false;   // reaches the end of the list

      int lyr = hints_[layer];
      int max_layer = maxLayer();
      while (skip_list_type::greater(data, succs_[lyr]) && lyr < max_layer) {
        ++lyr;
      }
      hints_[layer] = lyr;  // update the hint

      int foundLayer = skip_list_type::findInsertionPoint(preds_[lyr], lyr, data, preds_, succs_);
      if (foundLayer < 0) return false;

      // DCHECK(succs_[0] != NULL) << "lyr=" << lyr << "; max_layer=" << max_layer;
      return !succs_[0]->markedForRemoval();
    }

  private:

    NodeType* head() const { return accessor_.skiplist()->head_.load(std::memory_order_consume); }

    Accessor accessor_;
    int headHeight_;
    NodeType *succs_[MAX_HEIGHT], *preds_[MAX_HEIGHT];
    uint8_t hints_[MAX_HEIGHT];
  };

} // atlas

#endif  // FOLLY_CONCURRENT_SKIP_LIST_H_

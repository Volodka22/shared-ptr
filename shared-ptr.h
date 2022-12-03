#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

#include "control-block.h"

template <typename T_>
class weak_ptr;

template <typename T_>
class shared_ptr;

template<typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args);

template <typename T>
class shared_ptr {
public:
  shared_ptr() noexcept {}
  shared_ptr(std::nullptr_t) noexcept {}

    template <typename E, typename Deleter = std::default_delete<E>,
              typename = std::enable_if_t<std::is_convertible_v<E*, T*>>>
    shared_ptr(E* ptr_, Deleter&& deleter = Deleter()) try
        : cb(new ptr_block(ptr_, std::forward<Deleter>(deleter))),
          ptr(ptr_) {
      increment();
    } catch (...) {
      deleter(ptr_);
      throw;
    }

  template <typename O>
  shared_ptr(const shared_ptr<O>& other, T* ptr_) : cb(other.cb), ptr(ptr_) {
    increment();
  }

  template <typename O,
            typename = std::enable_if_t<std::is_convertible_v<O*, T*>>>
  shared_ptr(const shared_ptr<O>& other) noexcept
      : cb(other.cb), ptr(static_cast<T*>(other.ptr)) {
    increment();
  }

  template <typename O,
            typename = std::enable_if_t<std::is_convertible_v<O*, T*>>>
  shared_ptr(shared_ptr<O>&& other) noexcept
      : cb(other.cb), ptr(static_cast<T*>(other.ptr)) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  shared_ptr(shared_ptr&& other) noexcept : cb(other.cb), ptr(other.ptr) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  shared_ptr(const shared_ptr& other) noexcept : cb(other.cb), ptr(other.ptr) {
    increment();
  }

  void swap(shared_ptr& other) noexcept {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }

  template <typename O>
  shared_ptr& operator=(const shared_ptr<O>& other) noexcept {
    shared_ptr(other).swap(*this);
    return *this;
  }

  template <typename O>
  shared_ptr& operator=(shared_ptr<O>&& other) noexcept {
    shared_ptr(std::move(other)).swap(*this);
    return *this;
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    shared_ptr(other).swap(*this);
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    shared_ptr(std::move(other)).swap(*this);
    return *this;
  }

  T* get() const noexcept {
    return ptr;
  }

  operator bool() const noexcept {
    return get();
  }

  T& operator*() const noexcept {
    return *get();
  }

  T* operator->() const noexcept {
    return get();
  }

  friend bool operator==(const shared_ptr& l, const shared_ptr& r) {
    return l.get() == r.get();
  }

  friend bool operator!=(const shared_ptr& l, const shared_ptr& r) {
    return !(l == r);
  }

  std::size_t use_count() const noexcept {
    return cb ? cb->strong_ref : 0;
  }

  void reset() noexcept {
    shared_ptr().swap(*this);
  }

  template <typename E, typename Deleter = std::default_delete<E>,
            typename = std::enable_if_t<std::is_convertible_v<E*, T*>>>
  void reset(E* new_ptr, Deleter&& deleter = Deleter()) {
    shared_ptr(new_ptr, std::forward<Deleter>(deleter)).swap(*this);
  }

  template <typename T_>
  friend class weak_ptr;

  template <typename T_>
  friend class shared_ptr;

  template <typename T_, typename... Args>
  friend shared_ptr<T_> make_shared(Args&&...);

  ~shared_ptr() {
    decrement();
  }

private:
  control_block* cb{nullptr};

  T* ptr{nullptr};

  shared_ptr(control_block* cb_, T* ptr_) : cb(cb_), ptr(ptr_) {
    increment();
  }

  void increment() {
    if (cb != nullptr) {
      cb->inc_strong();
    }
  }

  void decrement() {
    if (cb) {
      cb->dec_strong();
    }
  }
};

template <typename T>
class weak_ptr {
public:
  weak_ptr() noexcept : cb(nullptr) {}
  weak_ptr(const shared_ptr<T>& other) noexcept : cb(other.cb), ptr(other.ptr) {
    increment();
  }

  weak_ptr(const weak_ptr<T>& other) noexcept : cb(other.cb), ptr(other.ptr) {
    increment();
  }

  weak_ptr(weak_ptr<T>&& other) noexcept : cb(other.cb), ptr(other.ptr) {
    other.cb = nullptr;
  }

  void swap(weak_ptr& other) noexcept {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }

  weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr<T>& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(weak_ptr<T>&& other) noexcept {
    weak_ptr(std::move(other)).swap(*this);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    if (cb && cb->strong_ref) {
      return shared_ptr<T>(cb, ptr);
    }
    return shared_ptr<T>();
  }

  ~weak_ptr() {
    decrement();
  }

private:
  control_block* cb;
  T* ptr;

  void increment() {
    if (cb != nullptr) {
      cb->inc_weak();
    }
  }

  void decrement() {
    if (cb) {
      cb->dec_weak();
    }
  }
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* block = new obj_block<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(block, block->get_ptr());
}

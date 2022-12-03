#pragma once

#include <algorithm>
#include <cstddef>

struct control_block {
  size_t strong_ref = 0;
  size_t weak_ref = 0;

  void inc_weak();
  void inc_strong();
  void dec_weak();
  void dec_strong();
  virtual ~control_block() = default;
  control_block() noexcept = default;

private:
  virtual void unlink() = 0;
};

template <typename T, typename Deleter>
struct ptr_block : control_block {

  ptr_block(T* ptr_, Deleter&& deleter_) : ptr(ptr_), deleter(std::move(deleter_)) {}

  ~ptr_block() = default;

private:
  T* ptr;
  [[no_unique_address]] Deleter deleter;

  void unlink() override {
    deleter(ptr);
  }

};

template <typename T>
struct obj_block : control_block {

  template <typename... Args>
  explicit obj_block(Args&&... args) {
    ::new (&data) T(std::forward<Args>(args)...);
  }

  T* get_ptr() {
    return reinterpret_cast<T*>(&data);
  }

  ~obj_block() = default;

private:
  std::aligned_storage_t<sizeof(T), alignof(T)> data;

  void unlink() override {
    std::destroy_at(get_ptr());
  }
};

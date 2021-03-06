/*
 * \file singleton.h
 * \brief Singleton utility
 **/
#include <utility>
#include <memory>
#include <mutex>

namespace faststdb {
namespace common {

template<typename T>
class SingletonBase {
 public:
  friend class std::unique_ptr<T>;

  static T *Get() {
    return Instance();
  }

  /// Create singleton instance
  template<typename... Args>
  static T *Instance(Args &&... args) {
    if (instance_ == nullptr) {
      std::unique_lock<std::mutex> lock(instance_mu_);
      if (instance_ == nullptr) {
        instance_.reset(new T(std::forward<Args>(args)...));
      }
    }
    return instance_.get();
  }

 protected:
  SingletonBase() {}
  virtual ~SingletonBase() {}

 private:
  static std::mutex instance_mu_;
  static std::unique_ptr<T> instance_;
};

template<class T> std::unique_ptr<T> SingletonBase<T>::instance_;
template<class T> std::mutex SingletonBase<T>::instance_mu_;

template <typename T>
class Singleton : public SingletonBase<T>{ };

}  // namespace common
}  // namespace faststdb

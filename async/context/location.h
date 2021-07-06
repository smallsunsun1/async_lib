#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_LOCATION_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_LOCATION_

#include <string>

namespace sss {
namespace async {
class LocationHandler;
struct DecodedLocation {
  std::string filename;
  int line = -1;
  int column = -1;
};
class Location {
 public:
  Location() = default;
  Location(const LocationHandler *handler, intptr_t data)
      : data(data), mHandler(handler) {}
  explicit operator bool() const { return mHandler != nullptr; }
  DecodedLocation Decode() const;
  intptr_t data = 0;

 private:
  friend class LocationHandler;
  const LocationHandler *mHandler = nullptr;
};
class LocationHandler {
 public:
  virtual DecodedLocation DecodeLocation(Location loc) const = 0;

 protected:
  ~LocationHandler() {}

 private:
  virtual void VtableAnchor();
};

inline DecodedLocation Location::Decode() const {
  if (!mHandler) return DecodedLocation();
  return mHandler->DecodeLocation(*this);
}

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_LOCATION_ */

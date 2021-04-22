#ifndef ASYNC_TENSOR_TENSOR_
#define ASYNC_TENSOR_TENSOR_

namespace sss {
namespace async {

enum class TensorDtype { kFloat = 0, kDouble = 1, kInt = 2, kString = 3 };

class Tensor {
 public:
  enum class TensorType { kUnknownTypeTensor = 0, kHostDenseTensor = 1, kDeviceDenseTensor = 2 };
  bool isTensorType(TensorType inTensorType) const { return tensorType == inTensorType; }

 protected:
  Tensor(const Tensor&) = delete;
  Tensor& operator=(const Tensor&) = delete;
  Tensor(Tensor&& other);
  Tensor& operator=(Tensor&& other);

 private:
  TensorType tensorType = TensorType::kUnknownTypeTensor;
};

template <typename Derived>
class TensorTraits {
  static const Tensor::TensorType kTensorDtype;
  static bool classof(Tensor* v) { return v->isTensorType(kTensorDtype); }
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_TENSOR_TENSOR_ */

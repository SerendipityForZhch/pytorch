#include <ATen/core/op_registration/op_registration.h>
#include "caffe2/core/export_c10_op_to_caffe2.h"
#include "caffe2/operators/elementwise_ops_utils.h"
#include "caffe2/utils/math.h"

using caffe2::BaseContext;
using caffe2::Tensor;

namespace caffe2 {
namespace {

template <class DataType>
void add_op_cpu_impl(
    const at::Tensor& A_,
    const at::Tensor& B_,
    const at::Tensor& C_,
    bool legacy_broadcast,
    int64_t axis) {
  Tensor A(A_);
  Tensor B(B_);
  Tensor C(C_);
  CPUContext context;
  const DataType* A_data = A.template data<DataType>();
  const DataType* B_data = B.template data<DataType>();
  std::vector<int> A_dims;
  std::vector<int> B_dims;

  if (legacy_broadcast) {
    CAFFE_ENFORCE(
        !B.is_same(C),
        "In-place is allowed only with the first tensor when "
        "legacy-broadcasting");
    C.ResizeLike(A);
    if (B.numel() == 1) {
      A_dims = {static_cast<int>(A.numel())};
      B_dims = {1};
    } else {
      size_t pre, n, post;
      std::tie(pre, n, post) =
          caffe2::elementwise_ops_utils::ComputeLegacyBroadcastSizes(
              A, B, axis);
      A_dims = {
          static_cast<int>(pre), static_cast<int>(n), static_cast<int>(post)};
      B_dims = {static_cast<int>(n), 1};
    }
  } else {
    std::copy(A.sizes().cbegin(), A.sizes().cend(), std::back_inserter(A_dims));
    std::copy(B.sizes().cbegin(), B.sizes().cend(), std::back_inserter(B_dims));
    const std::vector<int> C_dims =
        caffe2::elementwise_ops_utils::ComputeBinaryBroadcastForwardDims(
            A_dims, B_dims);
    if (A.is_same(C)) {
      CAFFE_ENFORCE_EQ(C_dims, A_dims);
    } else if (B.is_same(C)) {
      CAFFE_ENFORCE_EQ(C_dims, B_dims);
    } else {
      C.Resize(C_dims);
    }
  }
  auto* C_data = C.template mutable_data<DataType>();

  caffe2::math::Add(
      A_dims.size(),
      A_dims.data(),
      B_dims.size(),
      B_dims.data(),
      A.data<DataType>(),
      B.data<DataType>(),
      C.mutable_data<DataType>(),
      static_cast<CPUContext*>(&context));
}

static auto registry = c10::RegisterOperators().op(
    "_c10_experimental::Add",
    c10::RegisterOperators::options()
      .kernel<decltype(add_op_cpu_impl<float>), &add_op_cpu_impl<float>>(TensorTypeId::CPUTensorId));

} // namespace

C10_EXPORT_C10_OP_TO_CAFFE2_CPU(
    "_c10_experimental::Add",
    C10Add_DontUseThisOpYet)

} // namespace caffe2

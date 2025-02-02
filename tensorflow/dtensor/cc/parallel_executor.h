/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_DTENSOR_CC_PARALLEL_EXECUTOR_H_
#define TENSORFLOW_DTENSOR_CC_PARALLEL_EXECUTOR_H_

#include <memory>
#include <optional>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "mlir/IR/BuiltinOps.h"  // from @llvm-project
#include "tensorflow/c/eager/c_api_experimental.h"
#include "tensorflow/compiler/xla/pjrt/pjrt_future.h"
#include "tensorflow/dtensor/cc/tensor_layout.h"
#include "tensorflow/dtensor/cc/tensor_with_layout.h"

namespace tensorflow {
namespace dtensor {

template <typename T>
using Future = ::xla::PjRtFuture<T>;

// ParallelExecutor Interface
// Note: The interface is under development and APIs are subject to change.
class ParallelExecutor {
 public:
  virtual ~ParallelExecutor() = default;

  // Broadcasts `tensor` to `mesh` using replicated sharding and returns a
  // DTensor representation.
  virtual StatusOr<std::unique_ptr<TensorWithLayout>> Broadcast(
      const Tensor& tensor, const Mesh& mesh,
      std::optional<NodeDef> const_value) = 0;

  // Takes input TensorWithLayouts, a MLIR module and the entry function name.
  // Attributes are forwarded to executed operations unmodified.
  // The execute is non-blocking and returns a Future of output TensorWithLayout
  // raw pointers.
  // The client is responsible for the ownership of the outputs.
  struct ExecutionResult {
    Future<Status> status;
    // The pointed data of `outputs` are filled after `status` future resolves
    // as ok.
    std::vector<TensorWithLayout*> outputs;
  };
  virtual StatusOr<ExecutionResult> Execute(
      TFE_Context* context, const std::vector<TensorWithLayout*>& inputs,
      mlir::ModuleOp module, llvm::StringRef entry_function_name,
      const TFE_OpAttrs* attributes) const = 0;

  // Disassembles `t` into multiple TensorWithLayouts. `t` may or may not be
  // valid to use afterwards.
  virtual StatusOr<std::vector<std::unique_ptr<TensorWithLayout>>> Disassemble(
      TensorWithLayout* t) = 0;

  // Returns a tensor copied from `t` when `t` contains only a single device.
  virtual Future<StatusOr<Tensor>> ToHostBuffer(TensorWithLayout* t) = 0;
};

// Factory method for Default ParallelExecutor instance.
StatusOr<std::unique_ptr<ParallelExecutor>> CreateDefaultParallelExecutor();

}  // namespace dtensor
}  // namespace tensorflow

#endif  // TENSORFLOW_DTENSOR_CC_PARALLEL_EXECUTOR_H_

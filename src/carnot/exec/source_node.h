#pragma once

#include <memory>
#include <string>
#include <vector>

#include "src/carnot/exec/exec_node.h"
#include "src/carnot/exec/exec_state.h"

namespace pl {
namespace carnot {
namespace exec {

class SourceNode : public ExecNode {
 public:
  SourceNode() : ExecNode(ExecNodeType::kSourceNode) {}

 protected:
  std::string DebugStringImpl() override;
  Status InitImpl(const plan::Operator &plan_node, const RowDescriptor &output_descriptor,
                  const std::vector<RowDescriptor> &input_descriptors) override;
  Status PrepareImpl(ExecState *exec_state) override;
  Status OpenImpl(ExecState *exec_state) override;
  Status CloseImpl(ExecState *exec_state) override;
  Status GenerateNextImpl(ExecState *exec_state) override;

 private:
  int64_t current_chunk_ = 0;
  std::unique_ptr<plan::MemorySourceOperator> plan_node_;
  std::unique_ptr<RowDescriptor> output_descriptor_;
  Table *table_ = nullptr;
};

}  // namespace exec
}  // namespace carnot
}  // namespace pl

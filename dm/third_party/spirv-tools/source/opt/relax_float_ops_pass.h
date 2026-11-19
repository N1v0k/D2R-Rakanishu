// Copyright (c) 2019 Valve Corporation
// Copyright (c) 2019 LunarG Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBSPIRV_OPT_RELAX_FLOAT_OPS_PASS_H_
#define LIBSPIRV_OPT_RELAX_FLOAT_OPS_PASS_H_

#include "source/opt/ir_builder.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {

class RelaxFloatOpsPass : public Pass {
 public:
  RelaxFloatOpsPass() : Pass() {}

  ~RelaxFloatOpsPass() override = default;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping;
  }


  Status Process() override;

  const char* name() const override { return "convert-to-half-pass"; }

 private:


  bool IsRelaxable(Instruction* inst);



  bool IsFloat32(Instruction* inst);


  bool IsRelaxed(uint32_t r_id);



  bool ProcessInst(Instruction* inst);


  bool ProcessFunction(Function* func);

  Pass::Status ProcessImpl();


  void Initialize();

  struct hasher {
    size_t operator()(const spv::Op& op) const noexcept {
      return std::hash<uint32_t>()(uint32_t(op));
    }
  };


  std::unordered_set<spv::Op, hasher> target_ops_core_f_rslt_;


  std::unordered_set<spv::Op, hasher> target_ops_core_f_opnd_;


  std::unordered_set<uint32_t> target_ops_450_;


  std::unordered_set<spv::Op, hasher> sample_ops_;
};

}
}

#endif

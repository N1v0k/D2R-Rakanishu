// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#ifndef SOURCE_OPT_LOCAL_ACCESS_CHAIN_CONVERT_PASS_H_
#define SOURCE_OPT_LOCAL_ACCESS_CHAIN_CONVERT_PASS_H_

#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/mem_pass.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {


class LocalAccessChainConvertPass : public MemPass {
 public:
  LocalAccessChainConvertPass();

  const char* name() const override { return "convert-local-access-chains"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisConstants |
           IRContext::kAnalysisTypes;
  }

  using ProcessFunction = std::function<bool(Function*)>;

 private:




  bool HasOnlySupportedRefs(uint32_t ptrId);




  void FindTargetVars(Function* func);



  void BuildAndAppendInst(spv::Op opcode, uint32_t typeId, uint32_t resultId,
                          const std::vector<Operand>& in_opnds,
                          std::vector<std::unique_ptr<Instruction>>* newInsts);



  uint32_t BuildAndAppendVarLoad(
      const Instruction* ptrInst, uint32_t* varId, uint32_t* varPteTypeId,
      std::vector<std::unique_ptr<Instruction>>* newInsts);




  void AppendConstantOperands(const Instruction* ptrInst,
                              std::vector<Operand>* in_opnds);




  bool GenAccessChainStoreReplacement(
      const Instruction* ptrInst, uint32_t valId,
      std::vector<std::unique_ptr<Instruction>>* newInsts);





  bool ReplaceAccessChainLoad(const Instruction* address_inst,
                              Instruction* original_load);



  bool Is32BitConstantIndexAccessChain(const Instruction* acp) const;












  Status ConvertLocalAccessChains(Function* func);




  bool AnyIndexIsOutOfBounds(const Instruction* access_chain_inst);




  bool IsIndexOutOfBounds(const analysis::Constant* index,
                          const analysis::Type* type) const;


  void InitExtensions();


  bool AllExtensionsSupported() const;

  void Initialize();
  Pass::Status ProcessImpl();



  std::unordered_set<uint32_t> supported_ref_ptrs_;


  std::unordered_set<std::string> extensions_allowlist_;
};

}
}

#endif

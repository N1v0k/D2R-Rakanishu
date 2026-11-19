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

#ifndef SOURCE_OPT_LOCAL_SINGLE_STORE_ELIM_PASS_H_
#define SOURCE_OPT_LOCAL_SINGLE_STORE_ELIM_PASS_H_

#include <algorithm>
#include <map>
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


class LocalSingleStoreElimPass : public Pass {
  using cbb_ptr = const BasicBlock*;

 public:
  LocalSingleStoreElimPass();

  const char* name() const override { return "eliminate-local-single-store"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:




  bool LocalSingleStoreElim(Function* func);


  void InitExtensionAllowList();


  bool AllExtensionsSupported() const;

  Pass::Status ProcessImpl();





  bool ProcessVariable(Instruction* var_inst);




  void FindUses(const Instruction* var_inst,
                std::vector<Instruction*>* uses) const;




  Instruction* FindSingleStoreAndCheckUses(
      Instruction* var_inst, const std::vector<Instruction*>& users) const;




  bool FeedsAStore(Instruction* inst) const;




  bool RewriteLoads(Instruction* store_inst,
                    const std::vector<Instruction*>& uses, bool* all_rewritten);



  bool RewriteDebugDeclares(Instruction* store_inst, uint32_t var_id);


  std::unordered_set<std::string> extensions_allowlist_;
};

}
}

#endif

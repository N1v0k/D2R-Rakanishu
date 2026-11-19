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

#ifndef SOURCE_OPT_LOCAL_SINGLE_BLOCK_ELIM_PASS_H_
#define SOURCE_OPT_LOCAL_SINGLE_BLOCK_ELIM_PASS_H_

#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "source/opt/basic_block.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/mem_pass.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {


class LocalSingleBlockLoadStoreElimPass : public MemPass {
 public:
  LocalSingleBlockLoadStoreElimPass();

  const char* name() const override { return "eliminate-local-single-block"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:





  bool HasOnlySupportedRefs(uint32_t varId);







  bool LocalSingleBlockLoadStoreElim(Function* func);


  void InitExtensions();


  bool AllExtensionsSupported() const;

  void Initialize();
  Pass::Status ProcessImpl();






  std::unordered_map<uint32_t, Instruction*> var2store_;






  std::unordered_map<uint32_t, Instruction*> var2load_;






  std::unordered_set<uint32_t> pinned_vars_;


  std::unordered_set<std::string> extensions_allowlist_;



  std::unordered_set<uint32_t> supported_ref_ptrs_;
};

}
}

#endif

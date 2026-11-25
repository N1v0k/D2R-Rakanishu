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

#ifndef SOURCE_OPT_AGGRESSIVE_DEAD_CODE_ELIM_PASS_H_
#define SOURCE_OPT_AGGRESSIVE_DEAD_CODE_ELIM_PASS_H_

#include <algorithm>
#include <list>
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
#include "source/util/bit_vector.h"

namespace spvtools {
namespace opt {


class AggressiveDCEPass : public MemPass {
  using cbb_ptr = const BasicBlock*;

 public:
  using GetBlocksFunction =
      std::function<std::vector<BasicBlock*>*(const BasicBlock*)>;

  AggressiveDCEPass(bool preserve_interface = false,
                    bool remove_outputs = false)
      : preserve_interface_(preserve_interface),
        remove_outputs_(remove_outputs) {}

  const char* name() const override { return "eliminate-dead-code-aggressive"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:




  bool preserve_interface_;




  bool remove_outputs_;



  bool IsVarOfStorage(uint32_t varId, spv::StorageClass storageClass);




  bool IsLocalVar(uint32_t varId, Function* func);


  bool IsLive(const Instruction* inst) const {
    return live_insts_.Get(inst->unique_id());
  }





  Pass::Status InitializeModuleScopeLiveInstructions();


  void AddToWorklist(Instruction* inst) {
    if (!live_insts_.Set(inst->unique_id())) {
      worklist_.push(inst);
    }
  }



  void AddStores(Function* func, uint32_t ptrId);


  void InitExtensions();


  bool AllExtensionsSupported() const;




  bool IsTargetDead(Instruction* inst);


  void ProcessLoad(Function* func, uint32_t varId);


  void AddBranch(uint32_t labelId, BasicBlock* bp);



  void AddBreaksAndContinuesToWorklist(Instruction* mergeInst);



  bool ProcessGlobalValues();


  bool EliminateDeadFunctions();










  Pass::Status AggressiveDCE(Function* func);

  Pass::Status ProcessImpl();



  void InitializeWorkList(Function* func,
                          std::list<BasicBlock*>& structured_order);




  void ProcessWorkList(Function* func);







  Pass::Status ProcessDebugInformation(
      std::list<BasicBlock*>& structured_order);





  Pass::Status KillDeadInstructions(const Function* func,
                                    std::list<BasicBlock*>& structured_order);


  void AddOperandsToWorkList(const Instruction* inst);


  void MarkBlockAsLive(Instruction* inst);


  void MarkLoadedVariablesAsLive(Function* func, Instruction* inst);



  uint32_t GetVariableId(uint32_t ptr_id);


  std::vector<uint32_t> GetLoadedVariables(Instruction* inst);



  std::vector<uint32_t> GetLoadedVariablesFromFunctionCall(
      const Instruction* inst);






  uint32_t GetLoadedVariableFromNonFunctionCalls(Instruction* inst);


  void AddDecorationsToWorkList(const Instruction* inst);


  void AddDebugScopeToWorkList(const Instruction* inst);


  void AddDebugInstructionsToWorkList(const Instruction* inst);


  void MarkFunctionParameterAsLive(const Function* func);



  Instruction* GetHeaderBranch(BasicBlock* blk);



  BasicBlock* GetHeaderBlock(BasicBlock* blk) const;




  Instruction* GetBranchForNextHeader(BasicBlock* blk);



  Instruction* GetMergeInstruction(Instruction* inst);


  bool BlockIsInConstruct(BasicBlock* header_block, BasicBlock* bb);



  bool IsEntryPointWithNoCalls(Function* func);


  bool IsEntryPoint(Function* func);


  bool HasCall(Function* func);


  void MarkFirstBlockAsLive(Function* func);


  void AddUnreachable(BasicBlock*& block);



  void MarkLoopConstructAsLiveIfLoopHeader(BasicBlock* basic_block);



  std::unordered_map<uint32_t, bool> entry_point_with_no_calls_cache_;






  std::queue<Instruction*> worklist_;


  utils::BitVector live_insts_;


  std::unordered_set<uint32_t> live_local_vars_;



  std::vector<Instruction*> to_kill_;


  std::unordered_set<std::string> extensions_allowlist_;
};

}
}

#endif

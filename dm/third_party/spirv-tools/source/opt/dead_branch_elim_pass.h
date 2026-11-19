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

#ifndef SOURCE_OPT_DEAD_BRANCH_ELIM_PASS_H_
#define SOURCE_OPT_DEAD_BRANCH_ELIM_PASS_H_

#include <algorithm>
#include <map>
#include <queue>
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


class DeadBranchElimPass : public MemPass {
  using cbb_ptr = const BasicBlock*;

 public:
  DeadBranchElimPass() = default;

  const char* name() const override { return "eliminate-dead-branches"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:


  bool GetConstCondition(uint32_t condId, bool* condVal);



  bool GetConstInteger(uint32_t valId, uint32_t* value);


  void AddBranch(uint32_t labelId, BasicBlock* bp);







  bool EliminateDeadBranches(Function* func);





  BasicBlock* GetParentBlock(uint32_t id);






  bool MarkLiveBlocks(Function* func,
                      std::unordered_set<BasicBlock*>* live_blocks);








  void MarkUnreachableStructuredTargets(
      const std::unordered_set<BasicBlock*>& live_blocks,
      std::unordered_set<BasicBlock*>* unreachable_merges,
      std::unordered_map<BasicBlock*, BasicBlock*>* unreachable_continues);













  bool FixPhiNodesInLiveBlocks(
      Function* func, const std::unordered_set<BasicBlock*>& live_blocks,
      const std::unordered_map<BasicBlock*, BasicBlock*>&
          unreachable_continues);












  bool EraseDeadBlocks(
      Function* func, const std::unordered_set<BasicBlock*>& live_blocks,
      const std::unordered_set<BasicBlock*>& unreachable_merges,
      const std::unordered_map<BasicBlock*, BasicBlock*>&
          unreachable_continues);



  void FixBlockOrder();












  Instruction* FindFirstExitFromSelectionMerge(uint32_t start_block_id,
                                               uint32_t merge_block_id,
                                               uint32_t loop_merge_id,
                                               uint32_t loop_continue_id,
                                               uint32_t switch_merge_id);





  void AddBlocksWithBackEdge(
      uint32_t cont_id, uint32_t header_id, uint32_t merge_id,
      std::unordered_set<BasicBlock*>* blocks_with_back_edges);




  bool SwitchHasNestedBreak(uint32_t switch_header_id);





  bool SimplifyBranch(BasicBlock* block, uint32_t live_lab_id);
};

}
}

#endif

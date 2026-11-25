// Copyright (c) 2023 Google Inc.
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

#ifndef SOURCE_OPT_DEDUPE_INTERLOCK_INVOCATION_PASS_H_
#define SOURCE_OPT_DEDUPE_INTERLOCK_INVOCATION_PASS_H_

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "source/enum_set.h"
#include "source/extensions.h"
#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"
#include "source/spirv_target_env.h"

namespace spvtools {
namespace opt {




class InvocationInterlockPlacementPass : public Pass {
 public:
  InvocationInterlockPlacementPass() {}
  InvocationInterlockPlacementPass(const InvocationInterlockPlacementPass&) =
      delete;
  InvocationInterlockPlacementPass(InvocationInterlockPlacementPass&&) = delete;

  const char* name() const override { return "dedupe-interlock-invocation"; }
  Status Process() override;

 private:
  using BlockSet = std::unordered_set<uint32_t>;


  struct ExtractionResult {
    bool had_begin : 1;
    bool had_end : 2;
  };






  bool hasSingleNextBlock(uint32_t block_id, bool reverse_cfg);





  void forEachNext(uint32_t block_id, bool reverse_cfg,
                   std::function<void(uint32_t)> f);




  void addInstructionAtBlockBoundary(BasicBlock* block, spv::Op opcode,
                                     bool at_end);



  bool killDuplicateBegin(BasicBlock* block);


  bool killDuplicateEnd(BasicBlock* block);



  void recordBeginOrEndInFunction(Function* func);



  bool removeBeginAndEndInstructionsFromFunction(Function* func);




  bool extractInstructionsFromCalls(std::vector<BasicBlock*> blocks);




  void recordExistingBeginAndEndBlock(std::vector<BasicBlock*> blocks);







  BlockSet computeReachableBlocks(BlockSet& in_set,
                                  const BlockSet& starting_nodes,
                                  bool reverse_cfg);


  bool removeUnneededInstructions(BasicBlock* block);




  BasicBlock* splitEdge(BasicBlock* block, uint32_t succ_id);




  Status placeInstructionsForEdge(BasicBlock* block, uint32_t next_id,
                                  BlockSet& inside, BlockSet& previous_inside,
                                  spv::Op opcode, bool reverse_cfg);

  Status placeInstructions(BasicBlock* block);


  Status processFragmentShaderEntry(Function* entry_func);



  bool isFragmentShaderInterlockEnabled();



  std::unordered_map<Function*, ExtractionResult> extracted_functions_;


  BlockSet begin_;

  BlockSet end_;


  BlockSet after_begin_;


  BlockSet before_end_;

  BlockSet predecessors_after_begin_;

  BlockSet successors_before_end_;
};

}
}
#endif

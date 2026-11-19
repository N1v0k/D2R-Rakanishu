// Copyright (c) 2018 Google LLC.
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

#ifndef SOURCE_OPT_STRUCT_CFG_ANALYSIS_H_
#define SOURCE_OPT_STRUCT_CFG_ANALYSIS_H_

#include <unordered_map>
#include <unordered_set>

#include "source/opt/function.h"
#include "source/util/bit_vector.h"

namespace spvtools {
namespace opt {

class IRContext;



class StructuredCFGAnalysis {
 public:
  explicit StructuredCFGAnalysis(IRContext* ctx);




  uint32_t ContainingConstruct(uint32_t bb_id) {
    auto it = bb_to_construct_.find(bb_id);
    if (it == bb_to_construct_.end()) {
      return 0;
    }
    return it->second.containing_construct;
  }




  uint32_t ContainingConstruct(Instruction* inst);




  uint32_t MergeBlock(uint32_t bb_id);




  uint32_t NestingDepth(uint32_t block_id);




  uint32_t ContainingLoop(uint32_t bb_id) {
    auto it = bb_to_construct_.find(bb_id);
    if (it == bb_to_construct_.end()) {
      return 0;
    }
    return it->second.containing_loop;
  }




  uint32_t LoopMergeBlock(uint32_t bb_id);




  uint32_t LoopContinueBlock(uint32_t bb_id);






  uint32_t LoopNestingDepth(uint32_t bb_id);




  uint32_t ContainingSwitch(uint32_t bb_id) {
    auto it = bb_to_construct_.find(bb_id);
    if (it == bb_to_construct_.end()) {
      return 0;
    }
    return it->second.containing_switch;
  }



  uint32_t SwitchMergeBlock(uint32_t bb_id);


  bool IsContinueBlock(uint32_t bb_id);



  bool IsInContainingLoopsContinueConstruct(uint32_t bb_id);



  bool IsInContinueConstruct(uint32_t bb_id);


  bool IsMergeBlock(uint32_t bb_id);



  std::unordered_set<uint32_t> FindFuncsCalledFromContinue();

 private:
















  struct ConstructInfo {
    uint32_t containing_construct;
    uint32_t containing_loop;
    uint32_t containing_switch;
    bool in_continue;
  };



  void AddBlocksInFunction(Function* func);

  IRContext* context_;



  std::unordered_map<uint32_t, ConstructInfo> bb_to_construct_;
  utils::BitVector merge_blocks_;
};

}
}
#endif

// Copyright (c) 2017 Google Inc.
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

#ifndef SOURCE_OPT_MERGE_RETURN_PASS_H_
#define SOURCE_OPT_MERGE_RETURN_PASS_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/function.h"
#include "source/opt/mem_pass.h"

namespace spvtools {
namespace opt {




































































class MergeReturnPass : public MemPass {
 public:
  MergeReturnPass()
      : function_(nullptr),
        return_flag_(nullptr),
        return_value_(nullptr),
        constant_true_(nullptr),
        final_return_block_(nullptr) {}

  const char* name() const override { return "merge-return"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:



  class StructuredControlState {
   public:
    StructuredControlState(Instruction* break_merge, Instruction* merge)
        : break_merge_(break_merge), current_merge_(merge) {}

    bool InBreakable() const { return break_merge_; }
    bool InStructuredFlow() const { return CurrentMergeId() != 0; }

    uint32_t CurrentMergeId() const {
      return current_merge_ ? current_merge_->GetSingleWordInOperand(0u) : 0u;
    }

    uint32_t CurrentMergeHeader() const {
      return current_merge_ ? current_merge_->context()
                                  ->get_instr_block(current_merge_)
                                  ->id()
                            : 0;
    }

    uint32_t BreakMergeId() const {
      return break_merge_ ? break_merge_->GetSingleWordInOperand(0u) : 0u;
    }

    Instruction* BreakMergeInst() const { return break_merge_; }

   private:
    Instruction* break_merge_;
    Instruction* current_merge_;
  };



  std::vector<BasicBlock*> CollectReturnBlocks(Function* function);





  bool MergeReturnBlocks(Function* function,
                         const std::vector<BasicBlock*>& returnBlocks);


  void GenerateState(BasicBlock* block);





  bool ProcessStructured(Function* function,
                         const std::vector<BasicBlock*>& return_blocks);










  bool ProcessStructuredBlock(BasicBlock* block);





  bool AddReturnFlag();



  bool AddReturnValue();




  bool RecordReturned(BasicBlock* block);








  void RecordReturnValue(BasicBlock* block);




  bool BranchToBlock(BasicBlock* block, uint32_t target);










  bool PredicateBlocks(BasicBlock* return_block,
                       std::unordered_set<BasicBlock*>* pSet,
                       std::list<BasicBlock*>* order);










  bool BreakFromConstruct(BasicBlock* block,
                          std::unordered_set<BasicBlock*>* predicated,
                          std::list<BasicBlock*>* order,
                          Instruction* break_merge_inst);




  bool CreateReturn(BasicBlock* block);



  bool CreateReturnBlock();




  bool CreatePhiNodesForInst(BasicBlock* merge_block, Instruction& inst);




  bool AddNewPhiNodes();




  bool AddNewPhiNodes(BasicBlock* bb);



  void RecordImmediateDominators(Function* function);



  bool UpdatePhiNodes(BasicBlock* new_source, BasicBlock* target);

  StructuredControlState& CurrentState() { return state_.back(); }



  void InsertAfterElement(BasicBlock* element, BasicBlock* new_element,
                          std::list<BasicBlock*>* list);





  bool AddSingleCaseSwitchAroundFunction();




  BasicBlock* CreateContinueTarget(uint32_t header_label_id);



  bool CreateSingleCaseSwitch(BasicBlock* merge_target);




  bool HasNontrivialUnreachableBlocks(Function* function);



  std::vector<StructuredControlState> state_;


  Function* function_;



  Instruction* return_flag_;




  Instruction* return_value_;


  Instruction* constant_true_;



  BasicBlock* final_return_block_;




  std::unordered_map<BasicBlock*, Instruction*> original_dominator_;



  std::unordered_map<BasicBlock*, std::set<uint32_t>> new_edges_;




  std::unordered_set<uint32_t> return_blocks_;
};

}
}

#endif

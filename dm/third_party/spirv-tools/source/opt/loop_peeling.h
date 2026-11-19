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

#ifndef SOURCE_OPT_LOOP_PEELING_H_
#define SOURCE_OPT_LOOP_PEELING_H_

#include <algorithm>
#include <limits>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/loop_utils.h"
#include "source/opt/pass.h"
#include "source/opt/scalar_analysis.h"

namespace spvtools {
namespace opt {
































class LoopPeeling {
 public:










  LoopPeeling(Loop* loop, Instruction* loop_iteration_count,
              Instruction* canonical_induction_variable = nullptr)
      : context_(loop->GetContext()),
        loop_utils_(loop->GetContext(), loop),
        loop_(loop),
        loop_iteration_count_(!loop->IsInsideLoop(loop_iteration_count)
                                  ? loop_iteration_count
                                  : nullptr),
        int_type_(nullptr),
        original_loop_canonical_induction_variable_(
            canonical_induction_variable),
        canonical_induction_variable_(nullptr) {
    if (loop_iteration_count_) {
      int_type_ = context_->get_type_mgr()
                      ->GetType(loop_iteration_count_->type_id())
                      ->AsInteger();
      if (canonical_induction_variable_) {
        assert(canonical_induction_variable_->type_id() ==
                   loop_iteration_count_->type_id() &&
               "loop_iteration_count and canonical_induction_variable do not "
               "have the same type");
      }
    }
    GetIteratingExitValues();
  }














  bool CanPeelLoop() const {
    CFG& cfg = *context_->cfg();

    if (!loop_iteration_count_) {
      return false;
    }
    if (!int_type_) {
      return false;
    }
    if (int_type_->width() != 32) {
      return false;
    }
    if (!loop_->IsLCSSA()) {
      return false;
    }
    if (!loop_->GetMergeBlock()) {
      return false;
    }
    if (cfg.preds(loop_->GetMergeBlock()->id()).size() != 1) {
      return false;
    }
    if (!IsConditionCheckSideEffectFree()) {
      return false;
    }

    return !std::any_of(exit_value_.cbegin(), exit_value_.cend(),
                        [](std::pair<uint32_t, Instruction*> it) {
                          return it.second == nullptr;
                        });
  }



  bool PeelBefore(uint32_t factor);



  bool PeelAfter(uint32_t factor);


  Loop* GetClonedLoop() { return cloned_loop_; }

  Loop* GetOriginalLoop() { return loop_; }

 private:
  IRContext* context_;
  LoopUtils loop_utils_;

  Loop* loop_;

  Instruction* loop_iteration_count_;

  analysis::Integer* int_type_;

  Loop* cloned_loop_;


  bool do_while_form_;

  Instruction* original_loop_canonical_induction_variable_;


  Instruction* canonical_induction_variable_;

  std::unordered_map<uint32_t, Instruction*> exit_value_;




  bool DuplicateAndConnectLoop(LoopUtils::LoopCloningResult* clone_results);



  bool InsertCanonicalInductionVariable(
      LoopUtils::LoopCloningResult* clone_results);






  bool FixExitCondition(
      const std::function<uint32_t(Instruction*)>& condition_builder);



  void GetIteratorUpdateOperations(
      const Loop* loop, Instruction* iterator,
      std::unordered_set<Instruction*>* operations);





  void GetIteratingExitValues();



  bool IsConditionCheckSideEffectFree() const;



  BasicBlock* CreateBlockBefore(BasicBlock* bb);





  BasicBlock* ProtectLoop(Loop* loop, Instruction* condition,
                          BasicBlock* if_merge);
};





class LoopPeelingPass : public Pass {
 public:

  enum class PeelDirection {
    kNone,
    kBefore,
    kAfter
  };


  struct LoopPeelingStats {
    std::vector<std::tuple<const Loop*, PeelDirection, uint32_t>> peeled_loops_;
  };

  LoopPeelingPass(LoopPeelingStats* stats = nullptr) : stats_(stats) {}




  static void SetLoopPeelingThreshold(size_t code_grow_threshold) {
    code_grow_threshold_ = code_grow_threshold;
  }


  static size_t GetLoopPeelingThreshold() { return code_grow_threshold_; }

  const char* name() const override { return "loop-peeling"; }




  Pass::Status Process() override;

 private:

  enum class CmpOperator {
    kLT,
    kGT,
    kLE,
    kGE,
  };

  class LoopPeelingInfo {
   public:
    using Direction = std::pair<PeelDirection, uint32_t>;

    LoopPeelingInfo(Loop* loop, size_t loop_max_iterations,
                    ScalarEvolutionAnalysis* scev_analysis)
        : context_(loop->GetContext()),
          loop_(loop),
          scev_analysis_(scev_analysis),
          loop_max_iterations_(loop_max_iterations) {}





    Direction GetPeelingInfo(BasicBlock* bb) const;

   private:


    uint32_t GetFirstLoopInvariantOperand(Instruction* condition) const;


    uint32_t GetFirstNonLoopInvariantOperand(Instruction* condition) const;


    SExpression GetValueAtFirstIteration(SERecurrentNode* rec) const;

    SExpression GetValueAtIteration(SERecurrentNode* rec,
                                    int64_t iteration) const;

    SExpression GetValueAtLastIteration(SERecurrentNode* rec) const;

    bool EvalOperator(CmpOperator cmp_op, SExpression lhs, SExpression rhs,
                      bool* result) const;

    Direction HandleEquality(SExpression lhs, SExpression rhs) const;
    Direction HandleInequality(CmpOperator cmp_op, SExpression lhs,
                               SERecurrentNode* rhs) const;

    static Direction GetNoneDirection() {
      return Direction{LoopPeelingPass::PeelDirection::kNone, 0};
    }
    IRContext* context_;
    Loop* loop_;
    ScalarEvolutionAnalysis* scev_analysis_;
    size_t loop_max_iterations_;
  };


  Pass::Status ProcessFunction(Function* f);



  std::tuple<Pass::Status, Loop*> ProcessLoop(Loop* loop,
                                              CodeMetrics* loop_size);

  static size_t code_grow_threshold_;
  LoopPeelingStats* stats_;
};

}
}

#endif

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

#ifndef SOURCE_OPT_FOLD_H_
#define SOURCE_OPT_FOLD_H_

#include <cstdint>
#include <vector>

#include "source/opt/const_folding_rules.h"
#include "source/opt/constants.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/folding_rules.h"

namespace spvtools {
namespace opt {

class InstructionFolder {
 public:
  explicit InstructionFolder(IRContext* context)
      : context_(context),
        const_folding_rules_(new ConstantFoldingRules(context)),
        folding_rules_(new FoldingRules(context)) {
    folding_rules_->AddFoldingRules();
    const_folding_rules_->AddFoldingRules();
  }

  explicit InstructionFolder(
      IRContext* context, std::unique_ptr<FoldingRules>&& folding_rules,
      std::unique_ptr<ConstantFoldingRules>&& constant_folding_rules)
      : context_(context),
        const_folding_rules_(std::move(constant_folding_rules)),
        folding_rules_(std::move(folding_rules)) {
    folding_rules_->AddFoldingRules();
    const_folding_rules_->AddFoldingRules();
  }









  uint32_t FoldScalars(
      spv::Op opcode,
      const std::vector<const analysis::Constant*>& operands) const;














  std::vector<uint32_t> FoldVectors(
      spv::Op opcode, uint32_t num_dims,
      const std::vector<const analysis::Constant*>& operands) const;



  bool IsFoldableOpcode(spv::Op opcode) const;


  bool IsFoldableConstant(const analysis::Constant* cst) const;



  bool IsFoldableType(Instruction* type_inst) const;



  bool IsFoldableScalarType(Instruction* type_inst) const;



  bool IsFoldableVectorType(Instruction* type_inst) const;










  Instruction* FoldInstructionToConstant(
      Instruction* inst, std::function<uint32_t(uint32_t)> id_map) const;











  bool FoldInstruction(Instruction* inst) const;


  bool HasConstFoldingRule(const Instruction* inst) const {
    return GetConstantFoldingRules().HasFoldingRule(inst);
  }

 private:

  const ConstantFoldingRules& GetConstantFoldingRules() const {
    return *const_folding_rules_;
  }


  const FoldingRules& GetFoldingRules() const { return *folding_rules_; }



  uint32_t UnaryOperate(spv::Op opcode, uint32_t operand) const;



  uint32_t BinaryOperate(spv::Op opcode, uint32_t a, uint32_t b) const;



  uint32_t TernaryOperate(spv::Op opcode, uint32_t a, uint32_t b,
                          uint32_t c) const;





  uint32_t OperateWords(spv::Op opcode,
                        const std::vector<uint32_t>& operand_words) const;

  bool FoldInstructionInternal(Instruction* inst) const;







  bool FoldBinaryIntegerOpToConstant(
      Instruction* inst, const std::function<uint32_t(uint32_t)>& id_map,
      uint32_t* result) const;





  bool FoldBinaryBooleanOpToConstant(
      Instruction* inst, const std::function<uint32_t(uint32_t)>& id_map,
      uint32_t* result) const;





  bool FoldIntegerOpToConstant(Instruction* inst,
                               const std::function<uint32_t(uint32_t)>& id_map,
                               uint32_t* result) const;

  IRContext* context_;


  std::unique_ptr<ConstantFoldingRules> const_folding_rules_;


  std::unique_ptr<FoldingRules> folding_rules_;
};

}
}

#endif

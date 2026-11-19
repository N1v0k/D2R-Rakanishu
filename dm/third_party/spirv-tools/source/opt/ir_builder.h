// Copyright (c) 2018 Google Inc.
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

#ifndef SOURCE_OPT_IR_BUILDER_H_
#define SOURCE_OPT_IR_BUILDER_H_

#include <cassert>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/constants.h"
#include "source/opt/instruction.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {



constexpr uint32_t kInvalidId = std::numeric_limits<uint32_t>::max();






class InstructionBuilder {
 public:
  using InsertionPointTy = BasicBlock::iterator;



  InstructionBuilder(
      IRContext* context, Instruction* insert_before,
      IRContext::Analysis preserved_analyses = IRContext::kAnalysisNone)
      : InstructionBuilder(context, context->get_instr_block(insert_before),
                           InsertionPointTy(insert_before),
                           preserved_analyses) {}



  InstructionBuilder(
      IRContext* context, BasicBlock* parent_block,
      IRContext::Analysis preserved_analyses = IRContext::kAnalysisNone)
      : InstructionBuilder(context, parent_block, parent_block->end(),
                           preserved_analyses) {}

  Instruction* AddNullaryOp(uint32_t type_id, spv::Op opcode) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), opcode, type_id, result_id, {}));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddUnaryOp(uint32_t type_id, spv::Op opcode, uint32_t operand1) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> newUnOp(new Instruction(
        GetContext(), opcode, type_id, result_id,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand1}}}));
    return AddInstruction(std::move(newUnOp));
  }

  Instruction* AddBinaryOp(uint32_t type_id, spv::Op opcode, uint32_t operand1,
                           uint32_t operand2) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> newBinOp(new Instruction(
        GetContext(), opcode, type_id,
        opcode == spv::Op::OpStore ? 0 : result_id,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand1}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand2}}}));
    return AddInstruction(std::move(newBinOp));
  }

  Instruction* AddTernaryOp(uint32_t type_id, spv::Op opcode, uint32_t operand1,
                            uint32_t operand2, uint32_t operand3) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> newTernOp(new Instruction(
        GetContext(), opcode, type_id, result_id,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand1}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand2}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand3}}}));
    return AddInstruction(std::move(newTernOp));
  }

  Instruction* AddQuadOp(uint32_t type_id, spv::Op opcode, uint32_t operand1,
                         uint32_t operand2, uint32_t operand3,
                         uint32_t operand4) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> newQuadOp(new Instruction(
        GetContext(), opcode, type_id, result_id,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand1}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand2}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand3}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {operand4}}}));
    return AddInstruction(std::move(newQuadOp));
  }

  Instruction* AddIdLiteralOp(uint32_t type_id, spv::Op opcode, uint32_t id,
                              uint32_t uliteral) {
    uint32_t result_id = 0;
    if (type_id != 0) {
      result_id = GetContext()->TakeNextId();
      if (result_id == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> newBinOp(new Instruction(
        GetContext(), opcode, type_id, result_id,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER, {uliteral}}}));
    return AddInstruction(std::move(newBinOp));
  }





  Instruction* AddNaryOp(uint32_t type_id, spv::Op opcode,
                         const std::vector<uint32_t>& operands,
                         uint32_t result = 0) {
    std::vector<Operand> ops;
    for (size_t i = 0; i < operands.size(); i++) {
      ops.push_back({SPV_OPERAND_TYPE_ID, {operands[i]}});
    }
    if (result == 0) {
      result = GetContext()->TakeNextId();
      if (result == 0) {
        return nullptr;
      }
    }
    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), opcode, type_id, result, ops));
    return AddInstruction(std::move(new_inst));
  }



  Instruction* AddSelectionMerge(
      uint32_t merge_id, uint32_t selection_control = static_cast<uint32_t>(
                             spv::SelectionControlMask::MaskNone)) {
    std::unique_ptr<Instruction> new_branch_merge(new Instruction(
        GetContext(), spv::Op::OpSelectionMerge, 0, 0,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {merge_id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_SELECTION_CONTROL,
          {selection_control}}}));
    return AddInstruction(std::move(new_branch_merge));
  }





  Instruction* AddLoopMerge(uint32_t merge_id, uint32_t continue_id,
                            uint32_t loop_control = static_cast<uint32_t>(
                                spv::LoopControlMask::MaskNone)) {
    std::unique_ptr<Instruction> new_branch_merge(new Instruction(
        GetContext(), spv::Op::OpLoopMerge, 0, 0,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {merge_id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {continue_id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_LOOP_CONTROL, {loop_control}}}));
    return AddInstruction(std::move(new_branch_merge));
  }




  Instruction* AddBranch(uint32_t label_id) {
    std::unique_ptr<Instruction> new_branch(new Instruction(
        GetContext(), spv::Op::OpBranch, 0, 0,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {label_id}}}));
    return AddInstruction(std::move(new_branch));
  }
















  Instruction* AddConditionalBranch(
      uint32_t cond_id, uint32_t true_id, uint32_t false_id,
      uint32_t merge_id = kInvalidId,
      uint32_t selection_control =
          static_cast<uint32_t>(spv::SelectionControlMask::MaskNone)) {
    if (merge_id != kInvalidId) {
      AddSelectionMerge(merge_id, selection_control);
    }
    std::unique_ptr<Instruction> new_branch(new Instruction(
        GetContext(), spv::Op::OpBranchConditional, 0, 0,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {cond_id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {true_id}},
         {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {false_id}}}));
    return AddInstruction(std::move(new_branch));
  }














  Instruction* AddSwitch(
      uint32_t selector_id, uint32_t default_id,
      const std::vector<std::pair<Operand::OperandData, uint32_t>>& targets,
      uint32_t merge_id = kInvalidId,
      uint32_t selection_control =
          static_cast<uint32_t>(spv::SelectionControlMask::MaskNone)) {
    if (merge_id != kInvalidId) {
      AddSelectionMerge(merge_id, selection_control);
    }
    std::vector<Operand> operands;
    operands.emplace_back(
        Operand{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {selector_id}});
    operands.emplace_back(
        Operand{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {default_id}});
    for (auto& target : targets) {
      operands.emplace_back(
          Operand{spv_operand_type_t::SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER,
                  target.first});
      operands.emplace_back(
          Operand{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {target.second}});
    }
    std::unique_ptr<Instruction> new_switch(
        new Instruction(GetContext(), spv::Op::OpSwitch, 0, 0, operands));
    return AddInstruction(std::move(new_switch));
  }





  Instruction* AddPhi(uint32_t type, const std::vector<uint32_t>& incomings,
                      uint32_t result = 0) {
    assert(incomings.size() % 2 == 0 && "A sequence of pairs is expected");
    return AddNaryOp(type, spv::Op::OpPhi, incomings, result);
  }






  Instruction* AddIAdd(uint32_t type, uint32_t op1, uint32_t op2) {
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> inst(new Instruction(
        GetContext(), spv::Op::OpIAdd, type, result_id,
        {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}}));
    return AddInstruction(std::move(inst));
  }





  Instruction* AddULessThan(uint32_t op1, uint32_t op2) {
    analysis::Bool bool_type;
    uint32_t type = GetContext()->get_type_mgr()->GetId(&bool_type);
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> inst(new Instruction(
        GetContext(), spv::Op::OpULessThan, type, result_id,
        {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}}));
    return AddInstruction(std::move(inst));
  }





  Instruction* AddSLessThan(uint32_t op1, uint32_t op2) {
    analysis::Bool bool_type;
    uint32_t type = GetContext()->get_type_mgr()->GetId(&bool_type);
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> inst(new Instruction(
        GetContext(), spv::Op::OpSLessThan, type, result_id,
        {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}}));
    return AddInstruction(std::move(inst));
  }





  Instruction* AddLessThan(uint32_t op1, uint32_t op2) {
    Instruction* op1_insn = context_->get_def_use_mgr()->GetDef(op1);
    analysis::Type* type =
        GetContext()->get_type_mgr()->GetType(op1_insn->type_id());
    analysis::Integer* int_type = type->AsInteger();
    assert(int_type && "Operand is not of int type");

    if (int_type->IsSigned())
      return AddSLessThan(op1, op2);
    else
      return AddULessThan(op1, op2);
  }





  Instruction* AddSelect(uint32_t type, uint32_t cond, uint32_t true_value,
                         uint32_t false_value) {
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> select(new Instruction(
        GetContext(), spv::Op::OpSelect, type, result_id,
        std::initializer_list<Operand>{{SPV_OPERAND_TYPE_ID, {cond}},
                                       {SPV_OPERAND_TYPE_ID, {true_value}},
                                       {SPV_OPERAND_TYPE_ID, {false_value}}}));
    return AddInstruction(std::move(select));
  }




  Instruction* GetSintConstant(int32_t value) {
    return GetIntConstant<int32_t>(value, true);
  }




  Instruction* AddCompositeConstruct(uint32_t type,
                                     const std::vector<uint32_t>& ids) {
    std::vector<Operand> ops;
    for (auto id : ids) {
      ops.emplace_back(SPV_OPERAND_TYPE_ID,
                       std::initializer_list<uint32_t>{id});
    }
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> construct(new Instruction(
        GetContext(), spv::Op::OpCompositeConstruct, type, result_id, ops));
    return AddInstruction(std::move(construct));
  }




  Instruction* GetUintConstant(uint32_t value) {
    return GetIntConstant<uint32_t>(value, false);
  }

  uint32_t GetUintConstantId(uint32_t value) {
    Instruction* uint_inst = GetUintConstant(value);
    return (uint_inst != nullptr ? uint_inst->result_id() : 0);
  }






  template <typename T>
  Instruction* GetIntConstant(T value, bool sign) {


    if (!sign)
      assert(value >= 0 &&
             "Trying to add a signed integer with an unsigned type!");

    analysis::Integer int_type{32, sign};



    uint32_t type_id =
        GetContext()->get_type_mgr()->GetTypeInstruction(&int_type);

    if (type_id == 0) {
      return nullptr;
    }



    analysis::Type* rebuilt_type =
        GetContext()->get_type_mgr()->GetType(type_id);



    uint32_t word = value;


    const analysis::Constant* constant =
        GetContext()->get_constant_mgr()->GetConstant(rebuilt_type, {word});


    return GetContext()->get_constant_mgr()->GetDefiningInstruction(constant);
  }

  Instruction* GetBoolConstant(bool value) {
    analysis::Bool type;
    uint32_t type_id = GetContext()->get_type_mgr()->GetTypeInstruction(&type);
    analysis::Type* rebuilt_type =
        GetContext()->get_type_mgr()->GetType(type_id);
    uint32_t word = value;
    const analysis::Constant* constant =
        GetContext()->get_constant_mgr()->GetConstant(rebuilt_type, {word});
    return GetContext()->get_constant_mgr()->GetDefiningInstruction(constant);
  }

  uint32_t GetBoolConstantId(bool value) {
    Instruction* inst = GetBoolConstant(value);
    return (inst != nullptr ? inst->result_id() : 0);
  }

  Instruction* AddCompositeExtract(uint32_t type, uint32_t id_of_composite,
                                   const std::vector<uint32_t>& index_list) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {id_of_composite}});

    for (uint32_t index : index_list) {
      operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {index}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(new Instruction(
        GetContext(), spv::Op::OpCompositeExtract, type, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }


  Instruction* AddUnreachable() {
    std::unique_ptr<Instruction> select(
        new Instruction(GetContext(), spv::Op::OpUnreachable, 0, 0,
                        std::initializer_list<Operand>{}));
    return AddInstruction(std::move(select));
  }

  Instruction* AddOpcodeAccessChain(spv::Op opcode, uint32_t type_id,
                                    uint32_t base_ptr_id,
                                    const std::vector<uint32_t>& ids) {
    assert(opcode == spv::Op::OpAccessChain ||
           opcode == spv::Op::OpInBoundsAccessChain);
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {base_ptr_id}});

    for (uint32_t index_id : ids) {
      operands.push_back({SPV_OPERAND_TYPE_ID, {index_id}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), opcode, type_id, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddAccessChain(uint32_t type_id, uint32_t base_ptr_id,
                              const std::vector<uint32_t>& ids) {
    return AddOpcodeAccessChain(spv::Op::OpAccessChain, type_id, base_ptr_id,
                                ids);
  }
  Instruction* AddInBoundsAccessChain(uint32_t type_id, uint32_t base_ptr_id,
                                      const std::vector<uint32_t>& ids) {
    return AddOpcodeAccessChain(spv::Op::OpInBoundsAccessChain, type_id,
                                base_ptr_id, ids);
  }

  Instruction* AddLoad(uint32_t type_id, uint32_t base_ptr_id,
                       uint32_t alignment = 0) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {base_ptr_id}});
    if (alignment != 0) {
      operands.push_back(
          {SPV_OPERAND_TYPE_MEMORY_ACCESS,
           {static_cast<uint32_t>(spv::MemoryAccessMask::Aligned)}});
      operands.push_back({SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER, {alignment}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(new Instruction(
        GetContext(), spv::Op::OpLoad, type_id, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddCopyObject(uint32_t type_id, uint32_t value_id) {
    std::vector<Operand> operands{{SPV_OPERAND_TYPE_ID, {value_id}}};

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(new Instruction(
        GetContext(), spv::Op::OpCopyObject, type_id, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddVariable(uint32_t type_id, uint32_t storage_class) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_STORAGE_CLASS, {storage_class}});
    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(new Instruction(
        GetContext(), spv::Op::OpVariable, type_id, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddStore(uint32_t ptr_id, uint32_t obj_id) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {ptr_id}});
    operands.push_back({SPV_OPERAND_TYPE_ID, {obj_id}});

    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), spv::Op::OpStore, 0, 0, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddFunctionCall(uint32_t result_type, uint32_t function,
                               const std::vector<uint32_t>& parameters) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {function}});
    for (uint32_t id : parameters) {
      operands.push_back({SPV_OPERAND_TYPE_ID, {id}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }
    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), spv::Op::OpFunctionCall, result_type,
                        result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddVectorShuffle(uint32_t result_type, uint32_t vec1,
                                uint32_t vec2,
                                const std::vector<uint32_t>& components) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {vec1}});
    operands.push_back({SPV_OPERAND_TYPE_ID, {vec2}});
    for (uint32_t id : components) {
      operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {id}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }

    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), spv::Op::OpVectorShuffle, result_type,
                        result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddDecoration(uint32_t target_id, spv::Decoration d,
                             const std::vector<uint32_t>& literals) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {target_id}});
    operands.push_back({SPV_OPERAND_TYPE_DECORATION, {uint32_t(d)}});
    for (uint32_t literal : literals) {
      operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {literal}});
    }

    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), spv::Op::OpDecorate, 0, 0, operands));




    Instruction* result = new_inst.get();
    GetContext()->AddAnnotationInst(std::move(new_inst));
    return result;
  }

  Instruction* AddNaryExtendedInstruction(
      uint32_t result_type, uint32_t set, uint32_t instruction,
      const std::vector<uint32_t>& ext_operands) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {set}});
    operands.push_back(
        {SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, {instruction}});
    for (uint32_t id : ext_operands) {
      operands.push_back({SPV_OPERAND_TYPE_ID, {id}});
    }

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }

    std::unique_ptr<Instruction> new_inst(new Instruction(
        GetContext(), spv::Op::OpExtInst, result_type, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }

  Instruction* AddSampledImage(uint32_t sampled_image_type_id,
                               uint32_t image_id, uint32_t sampler_id) {
    std::vector<Operand> operands;
    operands.push_back({SPV_OPERAND_TYPE_ID, {image_id}});
    operands.push_back({SPV_OPERAND_TYPE_ID, {sampler_id}});

    uint32_t result_id = GetContext()->TakeNextId();
    if (result_id == 0) {
      return nullptr;
    }

    std::unique_ptr<Instruction> new_inst(
        new Instruction(GetContext(), spv::Op::OpSampledImage,
                        sampled_image_type_id, result_id, operands));
    return AddInstruction(std::move(new_inst));
  }


  Instruction* AddInstruction(std::unique_ptr<Instruction>&& insn) {
    Instruction* insn_ptr = &*insert_before_.InsertBefore(std::move(insn));
    UpdateInstrToBlockMapping(insn_ptr);
    UpdateDefUseMgr(insn_ptr);
    return insn_ptr;
  }


  InsertionPointTy GetInsertPoint() { return insert_before_; }



  void SetInsertPoint(Instruction* insert_before) {
    parent_ = context_->get_instr_block(insert_before);
    insert_before_ = InsertionPointTy(insert_before);
  }



  void SetInsertPoint(BasicBlock* parent_block) {
    parent_ = parent_block;
    insert_before_ = parent_block->end();
  }


  IRContext* GetContext() const { return context_; }


  inline IRContext::Analysis GetPreservedAnalysis() const {
    return preserved_analyses_;
  }

 private:
  InstructionBuilder(IRContext* context, BasicBlock* parent,
                     InsertionPointTy insert_before,
                     IRContext::Analysis preserved_analyses)
      : context_(context),
        parent_(parent),
        insert_before_(insert_before),
        preserved_analyses_(preserved_analyses) {
    assert(!(preserved_analyses_ & ~(IRContext::kAnalysisDefUse |
                                     IRContext::kAnalysisInstrToBlockMapping)));
  }


  inline bool IsAnalysisUpdateRequested(IRContext::Analysis analysis) const {
    if (!GetContext()->AreAnalysesValid(analysis)) {

      return false;
    }
    return preserved_analyses_ & analysis;
  }



  inline void UpdateDefUseMgr(Instruction* insn) {
    if (IsAnalysisUpdateRequested(IRContext::kAnalysisDefUse))
      GetContext()->get_def_use_mgr()->AnalyzeInstDefUse(insn);
  }



  inline void UpdateInstrToBlockMapping(Instruction* insn) {
    if (IsAnalysisUpdateRequested(IRContext::kAnalysisInstrToBlockMapping) &&
        parent_)
      GetContext()->set_instr_block(insn, parent_);
  }

  IRContext* context_;
  BasicBlock* parent_;
  InsertionPointTy insert_before_;
  const IRContext::Analysis preserved_analyses_;
};

}
}

#endif

// Copyright (c) 2016 Google Inc.
// Copyright (c) 2025 Arm Ltd.
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

#include "source/opt/fold_spec_constant_op_and_composite_pass.h"

#include <algorithm>
#include <tuple>

#include "source/opt/constants.h"
#include "source/util/make_unique.h"

namespace spvtools {
namespace opt {

Pass::Status FoldSpecConstantOpAndCompositePass::Process() {
  bool modified = false;
  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();



















  Module::inst_iterator next_inst = context()->types_values_begin();
  for (Module::inst_iterator inst_iter = next_inst;


       inst_iter != context()->types_values_end(); inst_iter = next_inst) {
    ++next_inst;
    Instruction* inst = &*inst_iter;








    if (const_mgr->GetType(inst) &&
        !const_mgr->GetType(inst)->decoration_empty())
      continue;
    switch (spv::Op opcode = inst->opcode()) {

      case spv::Op::OpConstantTrue:
      case spv::Op::OpConstantFalse:
      case spv::Op::OpConstant:
      case spv::Op::OpConstantNull:
      case spv::Op::OpConstantComposite:
      case spv::Op::OpSpecConstantComposite:
      case spv::Op::OpSpecConstantCompositeReplicateEXT: {








        if (auto const_value = const_mgr->GetConstantFromInst(inst)) {




          if (opcode == spv::Op::OpSpecConstantComposite) {
            inst->SetOpcode(spv::Op::OpConstantComposite);
            modified = true;
          }
          if (opcode == spv::Op::OpSpecConstantCompositeReplicateEXT) {
            inst->SetOpcode(spv::Op::OpConstantCompositeReplicateEXT);
            modified = true;
          }
          const_mgr->MapConstantToInst(const_value, inst);
        }
        break;
      }







      case spv::Op::OpSpecConstantOp: {
        const auto status = ProcessOpSpecConstantOp(&inst_iter);
        if (status == Status::Failure) {
          return Status::Failure;
        }
        if (status == Status::SuccessWithChange) {
          modified = true;
        }
        break;
      }
      default:
        break;
    }
  }
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

Pass::Status FoldSpecConstantOpAndCompositePass::ProcessOpSpecConstantOp(
    Module::inst_iterator* pos) {
  Instruction* inst = &**pos;
  Instruction* folded_inst = nullptr;
  assert(inst->GetInOperand(0).type ==
             SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER &&
         "The first in-operand of OpSpecConstantOp instruction must be of "
         "SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER type");

  folded_inst = FoldWithInstructionFolder(pos);
  if (context()->id_overflow()) {
    return Status::Failure;
  }

  if (!folded_inst) {
    folded_inst = DoComponentWiseOperation(pos);
    if (context()->id_overflow()) {
      return Status::Failure;
    }
  }
  if (!folded_inst) return Status::SuccessWithoutChange;



  uint32_t new_id = folded_inst->result_id();
  uint32_t old_id = inst->result_id();
  context()->ReplaceAllUsesWith(old_id, new_id);
  context()->KillDef(old_id);
  return Status::SuccessWithChange;
}

Instruction* FoldSpecConstantOpAndCompositePass::FoldWithInstructionFolder(
    Module::inst_iterator* inst_iter_ptr) {
  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();


  for (uint32_t i = 1; i < (*inst_iter_ptr)->NumInOperands(); i++) {
    const Operand& operand = (*inst_iter_ptr)->GetInOperand(i);
    if (operand.type != SPV_OPERAND_TYPE_ID &&
        operand.type != SPV_OPERAND_TYPE_OPTIONAL_ID) {
      continue;
    }
    uint32_t id = operand.words[0];
    if (const_mgr->FindDeclaredConstant(id) == nullptr) {
      return nullptr;
    }
  }



  std::unique_ptr<Instruction> inst((*inst_iter_ptr)->Clone(context()));
  inst->SetOpcode(
      static_cast<spv::Op>((*inst_iter_ptr)->GetSingleWordInOperand(0)));
  inst->RemoveOperand(2);




  auto last_type_value_iter = (context()->types_values_end());
  --last_type_value_iter;
  Instruction* last_type_value = &*last_type_value_iter;

  auto identity_map = [](uint32_t id) { return id; };
  Instruction* new_const_inst =
      context()->get_instruction_folder().FoldInstructionToConstant(
          inst.get(), identity_map);


  if (!new_const_inst) return nullptr;



  Instruction* insert_pos = (*inst_iter_ptr)->PreviousNode();
  assert(insert_pos != nullptr &&
         "pos is the first instruction in the types and values.");
  bool need_to_clone = true;
  for (Instruction* i = last_type_value->NextNode(); i != nullptr;
       i = last_type_value->NextNode()) {
    if (i == new_const_inst) {
      need_to_clone = false;
    }
    i->InsertAfter(insert_pos);
    insert_pos = insert_pos->NextNode();
  }

  if (need_to_clone) {
    new_const_inst = new_const_inst->Clone(context());
    uint32_t new_id = TakeNextId();
    if (new_id == 0) {
      return nullptr;
    }
    new_const_inst->SetResultId(new_id);
    new_const_inst->InsertAfter(insert_pos);
    get_def_use_mgr()->AnalyzeInstDefUse(new_const_inst);
  }
  const_mgr->MapInst(new_const_inst);
  return new_const_inst;
}

namespace {







bool IsValidTypeForComponentWiseOperation(const analysis::Type* type) {
  if (type->AsBool()) {
    return true;
  } else if (auto* it = type->AsInteger()) {
    if (it->width() == 32) return true;
  } else if (auto* vt = type->AsVector()) {
    if (vt->element_type()->AsBool()) {
      return true;
    } else if (auto* vit = vt->element_type()->AsInteger()) {
      if (vit->width() == 32) return true;
    }
  }
  return false;
}







utils::SmallVector<uint32_t, 2> EncodeIntegerAsWords(const analysis::Type& type,
                                                     uint32_t value) {
  const uint32_t all_ones = ~0;
  uint32_t bit_width = 0;
  uint32_t pad_value = 0;
  bool result_type_signed = false;
  if (auto* int_ty = type.AsInteger()) {
    bit_width = int_ty->width();
    result_type_signed = int_ty->IsSigned();
    if (result_type_signed && static_cast<int32_t>(value) < 0) {
      pad_value = all_ones;
    }
  } else if (type.AsBool()) {
    bit_width = 1;
  } else {
    assert(false && "type must be Integer or Bool");
  }

  assert(bit_width > 0);
  uint32_t first_word = value;
  const uint32_t bits_per_word = 32;


  if (bit_width < bits_per_word) {
    first_word = utils::SignExtendValue(first_word, bit_width);
  }

  utils::SmallVector<uint32_t, 2> words = {first_word};
  for (uint32_t current_bit = bits_per_word; current_bit < bit_width;
       current_bit += bits_per_word) {
    words.push_back(pad_value);
  }

  return words;
}
}

Instruction* FoldSpecConstantOpAndCompositePass::DoComponentWiseOperation(
    Module::inst_iterator* pos) {
  const Instruction* inst = &**pos;
  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();
  const analysis::Type* result_type = const_mgr->GetType(inst);
  spv::Op spec_opcode = static_cast<spv::Op>(inst->GetSingleWordInOperand(0));

  std::vector<const analysis::Constant*> operands;

  if (!std::all_of(
          inst->cbegin(), inst->cend(), [&operands, this](const Operand& o) {

            if (o.type != spv_operand_type_t::SPV_OPERAND_TYPE_ID) return true;
            uint32_t id = o.words.front();
            if (auto c =
                    context()->get_constant_mgr()->FindDeclaredConstant(id)) {
              if (IsValidTypeForComponentWiseOperation(c->type())) {
                operands.push_back(c);
                return true;
              }
            }
            return false;
          }))
    return nullptr;

  if (result_type->AsInteger() || result_type->AsBool()) {

    const uint32_t result_val =
        context()->get_instruction_folder().FoldScalars(spec_opcode, operands);
    auto result_const = const_mgr->GetConstant(
        result_type, EncodeIntegerAsWords(*result_type, result_val));
    return const_mgr->BuildInstructionAndAddToModule(result_const, pos);
  } else if (result_type->AsVector()) {

    const analysis::Type* element_type =
        result_type->AsVector()->element_type();
    uint32_t num_dims = result_type->AsVector()->element_count();
    std::vector<uint32_t> result_vec =
        context()->get_instruction_folder().FoldVectors(spec_opcode, num_dims,
                                                        operands);
    std::vector<const analysis::Constant*> result_vector_components;
    for (const uint32_t r : result_vec) {
      if (auto rc = const_mgr->GetConstant(
              element_type, EncodeIntegerAsWords(*element_type, r))) {
        result_vector_components.push_back(rc);
        if (!const_mgr->BuildInstructionAndAddToModule(rc, pos)) {
          assert(false &&
                 "Failed to build and insert constant declaring instruction "
                 "for the given vector component constant");
        }
      } else {
        assert(false && "Failed to create constants with 32-bit word");
      }
    }
    auto new_vec_const = MakeUnique<analysis::VectorConstant>(
        result_type->AsVector(), result_vector_components);
    auto reg_vec_const = const_mgr->RegisterConstant(std::move(new_vec_const));
    return const_mgr->BuildInstructionAndAddToModule(reg_vec_const, pos);
  } else {



    return nullptr;
  }
}

}
}

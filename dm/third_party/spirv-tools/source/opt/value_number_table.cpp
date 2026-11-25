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

#include "source/opt/value_number_table.h"

#include <algorithm>

#include "source/opt/cfg.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {

uint32_t ValueNumberTable::GetValueNumber(Instruction* inst) const {
  assert(inst->result_id() != 0 &&
         "inst must have a result id to get a value number.");


  auto result_id_to_val = id_to_value_.find(inst->result_id());
  if (result_id_to_val != id_to_value_.end()) {
    return result_id_to_val->second;
  }
  return 0;
}

uint32_t ValueNumberTable::GetValueNumber(uint32_t id) const {
  return GetValueNumber(context()->get_def_use_mgr()->GetDef(id));
}

bool ValueNumberTable::IsReadOnlyLoad(Instruction* inst) {
  if (!inst->IsLoad()) {
    return false;
  }

  Instruction* address_def = inst->GetBaseAddress();
  if (!address_def) {
    return false;
  }

  auto cached_result = read_only_variable_cache_.find(address_def->result_id());
  if (cached_result != read_only_variable_cache_.end()) {
    return cached_result->second;
  }

  bool is_read_only = IsReadOnlyVariable(address_def);
  read_only_variable_cache_[address_def->result_id()] = is_read_only;
  return is_read_only;
}

bool ValueNumberTable::IsReadOnlyVariable(Instruction* address_def) {
  if (address_def->opcode() == spv::Op::OpVariable) {
    if (address_def->IsReadOnlyPointer()) {
      return true;
    }
  }

  if (address_def->opcode() == spv::Op::OpLoad) {
    const analysis::Type* address_type =
        context()->get_type_mgr()->GetType(address_def->type_id());
    if (address_type->AsSampledImage() != nullptr) {
      const auto* image_type =
          address_type->AsSampledImage()->image_type()->AsImage();
      return image_type->sampled() == 1;
    }
  }
  return false;
}

uint32_t ValueNumberTable::AssignValueNumber(Instruction* inst) {

  uint32_t value = GetValueNumber(inst);
  if (value != 0) {
    return value;
  }

  auto assign_new_number = [this](Instruction* i) {
    const auto new_value = TakeNextValueNumber();
    id_to_value_[i->result_id()] = new_value;
    return new_value;
  };



  if (!context()->IsCombinatorInstruction(inst) &&
      !inst->IsCommonDebugInstr()) {
    return assign_new_number(inst);
  }



  switch (inst->opcode()) {
    case spv::Op::OpSampledImage:
    case spv::Op::OpImage:
    case spv::Op::OpVariable:
      return assign_new_number(inst);
    default:
      break;
  }



  if (inst->IsLoad()) {
    switch (context()->get_def_use_mgr()->GetDef(inst->type_id())->opcode()) {
      case spv::Op::OpTypeSampledImage:
      case spv::Op::OpTypeImage:
      case spv::Op::OpTypeSampler:
        return assign_new_number(inst);
      default:
        break;
    }
  }







  if (inst->IsLoad() && !IsReadOnlyLoad(inst)) {
    return assign_new_number(inst);
  }

  analysis::DecorationManager* dec_mgr = context()->get_decoration_mgr();


  if (inst->opcode() == spv::Op::OpCopyObject &&
      dec_mgr->HaveTheSameDecorations(inst->result_id(),
                                      inst->GetSingleWordInOperand(0))) {
    value = GetValueNumber(inst->GetSingleWordInOperand(0));
    if (value != 0) {
      id_to_value_[inst->result_id()] = value;
      return value;
    }
  }



  if (inst->opcode() == spv::Op::OpPhi && inst->NumInOperands() > 0 &&
      dec_mgr->HaveTheSameDecorations(inst->result_id(),
                                      inst->GetSingleWordInOperand(0))) {
    value = GetValueNumber(inst->GetSingleWordInOperand(0));
    if (value != 0) {
      for (uint32_t op = 2; op < inst->NumInOperands(); op += 2) {
        if (value != GetValueNumber(inst->GetSingleWordInOperand(op))) {
          value = 0;
          break;
        }
      }
      if (value != 0) {
        id_to_value_[inst->result_id()] = value;
        return value;
      }
    }
  }



  Instruction value_ins(context(), inst->opcode(), inst->type_id(),
                        inst->result_id(), {});
  for (uint32_t o = 0; o < inst->NumInOperands(); ++o) {
    const Operand& op = inst->GetInOperand(o);
    if (spvIsIdType(op.type)) {
      uint32_t id_value = op.words[0];
      auto use_id_to_val = id_to_value_.find(id_value);
      if (use_id_to_val != id_to_value_.end()) {
        id_value = (1 << 31) | use_id_to_val->second;
      }
      value_ins.AddOperand(Operand(op.type, {id_value}));
    } else {
      value_ins.AddOperand(Operand(op.type, op.words));
    }
  }


  if (spvOpcodeIsCommutativeBinaryOperator(value_ins.opcode())) {
    if (value_ins.GetSingleWordInOperand(0) >
        value_ins.GetSingleWordInOperand(1)) {
      value_ins.SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {value_ins.GetSingleWordInOperand(1)}},
           {SPV_OPERAND_TYPE_ID, {value_ins.GetSingleWordInOperand(0)}}});
    }
  }


  auto value_iterator = instruction_to_value_.find(value_ins);
  if (value_iterator != instruction_to_value_.end()) {
    value = id_to_value_[value_iterator->first.result_id()];
    id_to_value_[inst->result_id()] = value;
    return value;
  }


  value = TakeNextValueNumber();
  id_to_value_[inst->result_id()] = value;
  instruction_to_value_[value_ins] = value;
  return value;
}

void ValueNumberTable::BuildDominatorTreeValueNumberTable() {

  for (auto& inst : context()->annotations()) {
    if (inst.result_id() != 0) {
      AssignValueNumber(&inst);
    }
  }

  for (auto& inst : context()->capabilities()) {
    if (inst.result_id() != 0) {
      AssignValueNumber(&inst);
    }
  }

  for (auto& inst : context()->types_values()) {
    if (inst.result_id() != 0) {
      AssignValueNumber(&inst);
    }
  }

  for (auto& inst : context()->module()->ext_inst_imports()) {
    if (inst.result_id() != 0) {
      AssignValueNumber(&inst);
    }
  }

  for (auto& inst : context()->module()->ext_inst_debuginfo()) {
    if (inst.result_id() != 0) {
      AssignValueNumber(&inst);
    }
  }

  for (Function& func : *context()->module()) {


    for (BasicBlock& block : func) {
      for (Instruction& inst : block) {
        if (inst.result_id() != 0) {
          AssignValueNumber(&inst);
        }
      }
    }
  }
}

bool ComputeSameValue::operator()(const Instruction& lhs,
                                  const Instruction& rhs) const {
  if (lhs.result_id() == 0 || rhs.result_id() == 0) {
    return false;
  }

  if (lhs.opcode() != rhs.opcode()) {
    return false;
  }

  if (lhs.type_id() != rhs.type_id()) {
    return false;
  }

  if (lhs.NumInOperands() != rhs.NumInOperands()) {
    return false;
  }

  for (uint32_t i = 0; i < lhs.NumInOperands(); ++i) {
    if (lhs.GetInOperand(i) != rhs.GetInOperand(i)) {
      return false;
    }
  }

  return lhs.context()->get_decoration_mgr()->HaveTheSameDecorations(
      lhs.result_id(), rhs.result_id());
}

std::size_t ValueTableHash::operator()(const Instruction& inst) const {



  std::u32string h;
  h.push_back(uint32_t(inst.opcode()));
  h.push_back(inst.type_id());
  for (uint32_t i = 0; i < inst.NumInOperands(); ++i) {
    const auto& opnd = inst.GetInOperand(i);
    for (uint32_t word : opnd.words) {
      h.push_back(word);
    }
  }
  return std::hash<std::u32string>()(h);
}
}
}

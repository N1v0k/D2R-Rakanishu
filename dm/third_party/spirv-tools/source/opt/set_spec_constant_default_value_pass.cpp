// Copyright (c) 2016 Google Inc.
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

#include "source/opt/set_spec_constant_default_value_pass.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "source/opt/def_use_manager.h"
#include "source/opt/types.h"
#include "source/util/make_unique.h"
#include "source/util/parse_number.h"
#include "spirv-tools/libspirv.h"

namespace spvtools {
namespace opt {
namespace {
using utils::EncodeNumberStatus;
using utils::NumberType;
using utils::ParseAndEncodeNumber;
using utils::ParseNumber;







std::vector<uint32_t> ParseDefaultValueStr(const char* text,
                                           const analysis::Type* type) {
  std::vector<uint32_t> result;
  if (!strcmp(text, "true") && type->AsBool()) {
    result.push_back(1u);
  } else if (!strcmp(text, "false") && type->AsBool()) {
    result.push_back(0u);
  } else {
    NumberType number_type = {32, SPV_NUMBER_UNSIGNED_INT};
    if (const auto* IT = type->AsInteger()) {
      number_type.bitwidth = IT->width();
      number_type.kind =
          IT->IsSigned() ? SPV_NUMBER_SIGNED_INT : SPV_NUMBER_UNSIGNED_INT;
    } else if (const auto* FT = type->AsFloat()) {
      number_type.bitwidth = FT->width();
      number_type.kind = SPV_NUMBER_FLOATING;
    } else {


      result.clear();
      return result;
    }
    EncodeNumberStatus rc = ParseAndEncodeNumber(
        text, number_type, [&result](uint32_t word) { result.push_back(word); },
        nullptr);

    if (rc != EncodeNumberStatus::kSuccess) {
      result.clear();
    }
  }
  return result;
}














std::vector<uint32_t> ParseDefaultValueBitPattern(
    const std::vector<uint32_t>& input_bit_pattern,
    const analysis::Type* type) {
  std::vector<uint32_t> result;
  if (type->AsBool()) {
    if (std::any_of(input_bit_pattern.begin(), input_bit_pattern.end(),
                    [](uint32_t i) { return i != 0; })) {
      result.push_back(1u);
    } else {
      result.push_back(0u);
    }
    return result;
  } else if (const auto* IT = type->AsInteger()) {
    const auto width = IT->width();
    assert(width > 0);
    const auto adjusted_width = std::max(32u, width);
    if (adjusted_width == input_bit_pattern.size() * sizeof(uint32_t) * 8) {
      result = std::vector<uint32_t>(input_bit_pattern);
      if (width < 32) {
        const uint32_t high_active_bit = (1u << width) >> 1;
        if (IT->IsSigned() && (high_active_bit & result[0])) {

          result[0] = result[0] | ~(high_active_bit - 1);
        } else {

          result[0] = result[0] & ((1u << width) - 1);
        }
      }
      return result;
    }
  } else if (const auto* FT = type->AsFloat()) {
    const auto width = FT->width();
    const auto adjusted_width = std::max(32u, width);
    if (adjusted_width == input_bit_pattern.size() * sizeof(uint32_t) * 8) {
      result = std::vector<uint32_t>(input_bit_pattern);
      if (width < 32) {

        result[0] = result[0] & ((1u << width) - 1);
      }
      return result;
    }
  } else if (type->AsArray()) {



    return std::vector<uint32_t>(input_bit_pattern);
  }
  result.clear();
  return result;
}



bool CanHaveSpecIdDecoration(const Instruction& inst) {
  switch (inst.opcode()) {
    case spv::Op::OpSpecConstant:
    case spv::Op::OpSpecConstantFalse:
    case spv::Op::OpSpecConstantTrue:
    case spv::Op::OpSpecConstantDataKHR:
      return true;
    default:
      return false;
  }
}





Instruction* GetSpecIdTargetFromDecorationGroup(
    const Instruction& decoration_group_defining_inst,
    analysis::DefUseManager* def_use_mgr) {






  Instruction* group_decorate_inst = nullptr;
  if (def_use_mgr->WhileEachUser(&decoration_group_defining_inst,
                                 [&group_decorate_inst](Instruction* user) {
                                   if (user->opcode() ==
                                       spv::Op::OpGroupDecorate) {
                                     group_decorate_inst = user;
                                     return false;
                                   }
                                   return true;
                                 }))
    return nullptr;








  Instruction* target_inst = nullptr;
  for (uint32_t i = 1; i < group_decorate_inst->NumInOperands(); i++) {


    uint32_t candidate_id = group_decorate_inst->GetSingleWordInOperand(i);
    Instruction* candidate_inst = def_use_mgr->GetDef(candidate_id);

    if (!candidate_inst) {
      continue;
    }

    if (!target_inst) {


      if (CanHaveSpecIdDecoration(*candidate_inst)) {
        target_inst = candidate_inst;
      } else {



        return nullptr;
      }
    } else {





      if (candidate_inst != target_inst) return nullptr;
    }
  }
  return target_inst;
}
}

Pass::Status SetSpecConstantDefaultValuePass::Process() {

  constexpr uint32_t kTargetIdOperandIndex = 0;

  constexpr uint32_t kDecorationOperandIndex = 1;


  constexpr uint32_t kSpecIdLiteralOperandIndex = 2;

  constexpr uint32_t kOpDecorateSpecIdNumOperands = 3;

  constexpr uint32_t kOpSpecConstantLiteralInOperandIndex = 0;


  constexpr uint32_t kOpSpecConstantDataLiteralInOperandIndex = 0;

  bool modified = false;









  for (Instruction& inst : context()->annotations()) {

    if (inst.opcode() != spv::Op::OpDecorate) continue;
    if (inst.NumOperands() != kOpDecorateSpecIdNumOperands) continue;
    if (inst.GetSingleWordInOperand(kDecorationOperandIndex) !=
        uint32_t(spv::Decoration::SpecId)) {
      continue;
    }


    uint32_t spec_id = inst.GetSingleWordOperand(kSpecIdLiteralOperandIndex);
    uint32_t target_id = inst.GetSingleWordOperand(kTargetIdOperandIndex);



    Instruction* spec_inst = nullptr;
    if (Instruction* target_inst = get_def_use_mgr()->GetDef(target_id)) {
      if (target_inst->opcode() == spv::Op::OpDecorationGroup) {
        spec_inst =
            GetSpecIdTargetFromDecorationGroup(*target_inst, get_def_use_mgr());
      } else {
        spec_inst = target_inst;
      }
    } else {
      continue;
    }
    if (!spec_inst) continue;


    std::vector<uint32_t> bit_pattern;

    if (spec_id_to_value_str_.size() != 0) {

      auto iter = spec_id_to_value_str_.find(spec_id);
      if (iter == spec_id_to_value_str_.end()) {
        continue;
      }



      const std::string& default_value_str = iter->second;
      bit_pattern = ParseDefaultValueStr(
          default_value_str.c_str(),
          context()->get_type_mgr()->GetType(spec_inst->type_id()));

    } else {

      auto iter = spec_id_to_value_bit_pattern_.find(spec_id);
      if (iter == spec_id_to_value_bit_pattern_.end()) {
        continue;
      }


      bit_pattern = ParseDefaultValueBitPattern(
          iter->second,
          context()->get_type_mgr()->GetType(spec_inst->type_id()));
    }

    if (bit_pattern.empty()) continue;



    switch (spec_inst->opcode()) {
      case spv::Op::OpSpecConstant:


        if (spec_inst->GetInOperand(kOpSpecConstantLiteralInOperandIndex)
                .words != bit_pattern) {
          spec_inst->SetInOperand(kOpSpecConstantLiteralInOperandIndex,
                                  std::move(bit_pattern));
          modified = true;
        }
        break;
      case spv::Op::OpSpecConstantTrue:


        if (!static_cast<bool>(bit_pattern.front())) {
          spec_inst->SetOpcode(spv::Op::OpSpecConstantFalse);
          modified = true;
        }
        break;
      case spv::Op::OpSpecConstantFalse:


        if (static_cast<bool>(bit_pattern.front())) {
          spec_inst->SetOpcode(spv::Op::OpSpecConstantTrue);
          modified = true;
        }
        break;
      case spv::Op::OpSpecConstantDataKHR: {
        if (spec_inst->GetInOperand(kOpSpecConstantDataLiteralInOperandIndex)
                .words != bit_pattern) {
          std::vector<Operand> operands;

          operands.push_back(spec_inst->GetOperand(0u));
          operands.push_back(spec_inst->GetOperand(1u));

          for (uint32_t word : bit_pattern) {
            operands.emplace_back(SPV_OPERAND_TYPE_LITERAL_INTEGER,
                                  std::initializer_list<uint32_t>{word});
          }
          spec_inst->ReplaceOperands(operands);
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



bool IsSeparator(char ch) {
  return std::strchr(":\0", ch) ||
         std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::unique_ptr<SetSpecConstantDefaultValuePass::SpecIdToValueStrMap>
SetSpecConstantDefaultValuePass::ParseDefaultValuesString(const char* str) {
  if (!str) return nullptr;

  auto spec_id_to_value = MakeUnique<SpecIdToValueStrMap>();


  while (*str) {

    while (std::isspace(static_cast<unsigned char>(*str)))
      str++;
    const char* entry_begin = str;
    while (!IsSeparator(*str)) str++;
    const char* entry_end = str;
    std::string spec_id_str(entry_begin, entry_end - entry_begin);
    uint32_t spec_id = 0;
    if (!ParseNumber(spec_id_str.c_str(), &spec_id)) {

      return nullptr;
    }
    auto iter = spec_id_to_value->find(spec_id);
    if (iter != spec_id_to_value->end()) {

      return nullptr;
    }

    if (*str++ != ':') {

      return nullptr;
    }

    const char* val_begin = str;
    while (!IsSeparator(*str)) str++;
    const char* val_end = str;
    if (val_end == val_begin) {

      return nullptr;
    }

    (*spec_id_to_value)[spec_id] = std::string(val_begin, val_end - val_begin);


    while (std::isspace(static_cast<unsigned char>(*str))) str++;
  }

  return spec_id_to_value;
}

}
}

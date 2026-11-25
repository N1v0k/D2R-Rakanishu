// Copyright (c) 2019 Google LLC
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
































































































































#include "graphics_robust_access_pass.h"

#include <functional>
#include <initializer_list>
#include <utility>

#include "function.h"
#include "ir_context.h"
#include "pass.h"
#include "source/diagnostic.h"
#include "source/util/make_unique.h"
#include "spirv-tools/libspirv.h"
#include "spirv/unified1/GLSL.std.450.h"
#include "type_manager.h"
#include "types.h"

namespace spvtools {
namespace opt {

using opt::Instruction;
using opt::Operand;
using spvtools::MakeUnique;

GraphicsRobustAccessPass::GraphicsRobustAccessPass() : module_status_() {}

Pass::Status GraphicsRobustAccessPass::Process() {
  module_status_ = PerModuleState();

  ProcessCurrentModule();

  auto result = module_status_.failed
                    ? Status::Failure
                    : (module_status_.modified ? Status::SuccessWithChange
                                               : Status::SuccessWithoutChange);

  return result;
}

spvtools::DiagnosticStream GraphicsRobustAccessPass::Fail() {
  module_status_.failed = true;

  return std::move(
      spvtools::DiagnosticStream({}, consumer(), "", SPV_ERROR_INVALID_BINARY)
      << name() << ": ");
}

spv_result_t GraphicsRobustAccessPass::IsCompatibleModule() {
  auto* feature_mgr = context()->get_feature_mgr();
  if (!feature_mgr->HasCapability(spv::Capability::Shader))
    return Fail() << "Can only process Shader modules";
  if (feature_mgr->HasCapability(spv::Capability::VariablePointers))
    return Fail() << "Can't process modules with VariablePointers capability";
  if (feature_mgr->HasCapability(
          spv::Capability::VariablePointersStorageBuffer))
    return Fail() << "Can't process modules with VariablePointersStorageBuffer "
                     "capability";
  if (feature_mgr->HasCapability(spv::Capability::RuntimeDescriptorArrayEXT)) {


    return Fail() << "Can't process modules with RuntimeDescriptorArrayEXT "
                     "capability";
  }

  {
    auto* inst = context()->module()->GetMemoryModel();
    const auto addressing_model =
        spv::AddressingModel(inst->GetSingleWordOperand(0));
    if (addressing_model != spv::AddressingModel::Logical)
      return Fail() << "Addressing model must be Logical.  Found "
                    << inst->PrettyPrint();
  }
  return SPV_SUCCESS;
}

spv_result_t GraphicsRobustAccessPass::ProcessCurrentModule() {
  auto err = IsCompatibleModule();
  if (err != SPV_SUCCESS) return err;

  ProcessFunction fn = [this](opt::Function* f) { return ProcessAFunction(f); };
  module_status_.modified |= context()->ProcessReachableCallTree(fn);


  return SPV_SUCCESS;
}

bool GraphicsRobustAccessPass::ProcessAFunction(opt::Function* function) {


  std::vector<Instruction*> access_chains;
  std::vector<Instruction*> image_texel_pointers;
  for (auto& block : *function) {
    for (auto& inst : block) {
      switch (inst.opcode()) {
        case spv::Op::OpAccessChain:
        case spv::Op::OpInBoundsAccessChain:
          access_chains.push_back(&inst);
          break;
        case spv::Op::OpImageTexelPointer:
          image_texel_pointers.push_back(&inst);
          break;
        default:
          break;
      }
    }
  }
  for (auto* inst : access_chains) {
    ClampIndicesForAccessChain(inst);
    if (module_status_.failed) return module_status_.modified;
  }

  for (auto* inst : image_texel_pointers) {
    if (SPV_SUCCESS != ClampCoordinateForImageTexelPointer(inst)) break;
  }
  return module_status_.modified;
}

void GraphicsRobustAccessPass::ClampIndicesForAccessChain(
    Instruction* access_chain) {
  Instruction& inst = *access_chain;

  auto* constant_mgr = context()->get_constant_mgr();
  auto* def_use_mgr = context()->get_def_use_mgr();
  auto* type_mgr = context()->get_type_mgr();
  const bool have_int64_cap =
      context()->get_feature_mgr()->HasCapability(spv::Capability::Int64);



  auto replace_index = [this, &inst, def_use_mgr](uint32_t operand_index,
                                                  Instruction* new_value) {
    inst.SetOperand(operand_index, {new_value->result_id()});
    def_use_mgr->AnalyzeInstUse(&inst);
    module_status_.modified = true;
    return SPV_SUCCESS;
  };







  auto clamp_index = [&inst, type_mgr, this, &replace_index](
                         uint32_t operand_index, Instruction* old_value,
                         Instruction* min_value,
                         Instruction* max_value) -> spv_result_t {
    auto* clamp_inst =
        MakeSClampInst(*type_mgr, old_value, min_value, max_value, &inst);
    if (clamp_inst == nullptr) {
      Fail();
      return SPV_ERROR_INTERNAL;
    }
    return replace_index(operand_index, clamp_inst);
  };




  auto clamp_to_literal_count =
      [&inst, this, &constant_mgr, &type_mgr, have_int64_cap, &replace_index,
       &clamp_index](uint32_t operand_index, uint64_t count) -> spv_result_t {
    Instruction* index_inst =
        this->GetDef(inst.GetSingleWordOperand(operand_index));
    const auto* index_type =
        type_mgr->GetType(index_inst->type_id())->AsInteger();
    assert(index_type);
    const auto index_width = index_type->width();

    if (count <= 1) {

      Instruction* new_value = GetValueForType(0, index_type);
      if (new_value == nullptr) {
        return Fail();
      }
      return replace_index(operand_index, new_value);
    }

    uint64_t maxval = count - 1;



    uint32_t maxval_width = index_width;
    while ((maxval_width < 64) && (0 != (maxval >> maxval_width))) {
      maxval_width *= 2;
    }

    uint32_t next_id = context()->module()->IdBound();
    analysis::Integer signed_type_for_query(maxval_width, true);
    auto* maxval_type_registered =
        type_mgr->GetRegisteredType(&signed_type_for_query);
    if (maxval_type_registered == nullptr) {
      return Fail();
    }
    auto* maxval_type = maxval_type_registered->AsInteger();
    if (maxval_type == nullptr) {
      return Fail();
    }
    if (next_id != context()->module()->IdBound()) {
      module_status_.modified = true;
    }


    maxval = std::min(maxval, ((uint64_t(1) << (maxval_width - 1)) - 1));

    if (index_width > 64) {
      return this->Fail() << "Can't handle indices wider than 64 bits, found "
                             "constant index with "
                          << index_width << " bits as index number "
                          << operand_index << " of access chain "
                          << inst.PrettyPrint();
    }







    if (auto* index_constant = constant_mgr->GetConstantFromInst(index_inst)) {
      auto* int_index_constant = index_constant->AsIntConstant();
      int64_t value = 0;


      if (index_width <= 32) {
        value = int64_t(int_index_constant->GetS32BitValue());
      } else if (index_width <= 64) {
        value = int_index_constant->GetS64BitValue();
      }
      if (value < 0) {
        Instruction* new_value = GetValueForType(0, index_type);
        if (new_value == nullptr) {
          return Fail();
        }
        return replace_index(operand_index, new_value);
      } else if (uint64_t(value) <= maxval) {

        return SPV_SUCCESS;
      } else {

        assert(count > 0);
        Instruction* new_value = GetValueForType(maxval, maxval_type);
        if (new_value == nullptr) {
          return Fail();
        }
        return replace_index(operand_index, new_value);
      }
    } else {

      assert(maxval >= 1);
      assert(index_width <= 64);
      if (index_width >= 64 && !have_int64_cap) {

        return Fail() << "Access chain index is wider than 64 bits, but Int64 "
                         "is not declared: "
                      << index_inst->PrettyPrint();
      }

      if (maxval_width > index_width) {





        assert(have_int64_cap || maxval_width <= 32);
        if (!have_int64_cap && maxval_width >= 64) {

          return this->Fail()
                 << "Clamping index would require adding Int64 capability. "
                 << "Can't clamp 32-bit index " << operand_index
                 << " of access chain " << inst.PrettyPrint();
        }
        index_inst = WidenInteger(index_type->IsSigned(), maxval_width,
                                  index_inst, &inst);
        if (index_inst == nullptr) {
          return Fail();
        }
      }


      return clamp_index(operand_index, index_inst,
                         GetValueForType(0, maxval_type),
                         GetValueForType(maxval, maxval_type));
    }
    return SPV_SUCCESS;
  };




  auto clamp_to_count = [&inst, this, &constant_mgr, &clamp_to_literal_count,
                         &clamp_index,
                         &type_mgr](uint32_t operand_index,
                                    Instruction* count_inst) -> spv_result_t {
    Instruction* index_inst =
        this->GetDef(inst.GetSingleWordOperand(operand_index));
    const auto* index_type =
        type_mgr->GetType(index_inst->type_id())->AsInteger();
    const auto* count_type =
        type_mgr->GetType(count_inst->type_id())->AsInteger();
    assert(index_type);
    if (const auto* count_constant =
            constant_mgr->GetConstantFromInst(count_inst)) {
      uint64_t value = 0;
      const auto width = count_constant->type()->AsInteger()->width();
      if (width <= 32) {
        value = count_constant->AsIntConstant()->GetU32BitValue();
      } else if (width <= 64) {
        value = count_constant->AsIntConstant()->GetU64BitValue();
      } else {
        return this->Fail() << "Can't handle indices wider than 64 bits, found "
                               "constant index with "
                            << index_type->width() << "bits";
      }
      return clamp_to_literal_count(operand_index, value);
    } else {

      const auto index_width = index_type->width();
      const auto count_width = count_type->width();
      const auto target_width = std::max(index_width, count_width);


      auto* wider_type = index_width < count_width ? count_type : index_type;
      if (index_type->width() < target_width) {

        index_inst = WidenInteger(true, target_width, index_inst, &inst);
        if (index_inst == nullptr) {
          return Fail();
        }
      } else if (count_type->width() < target_width) {

        count_inst = WidenInteger(false, target_width, count_inst, &inst);
        if (count_inst == nullptr) {
          return Fail();
        }
      }


      auto* one = GetValueForType(1, wider_type);
      if (!one) {
        return Fail();
      }
      auto* count_minus_1 =
          InsertInst(&inst, spv::Op::OpISub, type_mgr->GetId(wider_type),
                     context()->TakeNextId(),
                     {{SPV_OPERAND_TYPE_ID, {count_inst->result_id()}},
                      {SPV_OPERAND_TYPE_ID, {one->result_id()}}});
      if (count_minus_1 == nullptr) {
        return Fail();
      }
      auto* zero = GetValueForType(0, wider_type);
      if (!zero) {
        return Fail();
      }


      const uint64_t max_signed_value =
          ((uint64_t(1) << (target_width - 1)) - 1);
      Instruction* max_signed_inst =
          GetValueForType(max_signed_value, wider_type);
      if (!max_signed_inst) {
        return Fail();
      }



      auto* upper_bound =
          MakeUMinInst(*type_mgr, count_minus_1, max_signed_inst, &inst);
      if (upper_bound == nullptr) {
        return Fail();
      }

      return clamp_index(operand_index, index_inst, zero, upper_bound);
    }
    return SPV_SUCCESS;
  };

  const Instruction* base_inst = GetDef(inst.GetSingleWordInOperand(0));
  const Instruction* base_type = GetDef(base_inst->type_id());
  Instruction* pointee_type = GetDef(base_type->GetSingleWordInOperand(1));






  const uint32_t num_operands = inst.NumOperands();
  for (uint32_t idx = 3; !module_status_.failed && idx < num_operands; ++idx) {
    const uint32_t index_id = inst.GetSingleWordOperand(idx);
    Instruction* index_inst = GetDef(index_id);

    switch (pointee_type->opcode()) {
      case spv::Op::OpTypeMatrix:
      case spv::Op::OpTypeVector:
      {
        const uint32_t count = pointee_type->GetSingleWordOperand(2);
        if (clamp_to_literal_count(idx, count) != SPV_SUCCESS) return;
        pointee_type = GetDef(pointee_type->GetSingleWordOperand(1));
      } break;

      case spv::Op::OpTypeArray: {


        Instruction* array_len = GetDef(pointee_type->GetSingleWordOperand(2));
        if (clamp_to_count(idx, array_len) != SPV_SUCCESS) return;
        pointee_type = GetDef(pointee_type->GetSingleWordOperand(1));
      } break;

      case spv::Op::OpTypeStruct: {



        if (index_inst->opcode() != spv::Op::OpConstant ||
            !constant_mgr->GetConstantFromInst(index_inst)
                 ->type()
                 ->AsInteger()) {
          Fail() << "Member index into struct is not a constant integer: "
                 << index_inst->PrettyPrint(
                        SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
                 << "\nin access chain: "
                 << inst.PrettyPrint(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
          return;
        }
        const auto num_members = pointee_type->NumInOperands();
        const auto* index_constant =
            constant_mgr->GetConstantFromInst(index_inst);


        const auto index_value = index_constant->GetSignExtendedValue();
        if (index_value < 0 || index_value >= num_members) {
          Fail() << "Member index " << index_value
                 << " is out of bounds for struct type: "
                 << pointee_type->PrettyPrint(
                        SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
                 << "\nin access chain: "
                 << inst.PrettyPrint(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
          return;
        }
        pointee_type = GetDef(pointee_type->GetSingleWordInOperand(
            static_cast<uint32_t>(index_value)));

      } break;

      case spv::Op::OpTypeRuntimeArray: {
        auto* array_len = MakeRuntimeArrayLengthInst(&inst, idx);
        if (!array_len) {
          return;
        }
        if (clamp_to_count(idx, array_len) != SPV_SUCCESS) return;
        if (module_status_.failed) return;
        pointee_type = GetDef(pointee_type->GetSingleWordOperand(1));
      } break;

      default:
        Fail() << " Unhandled pointee type for access chain "
               << pointee_type->PrettyPrint(
                      SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
    }
  }
}

uint32_t GraphicsRobustAccessPass::GetGlslInsts() {
  if (module_status_.glsl_insts_id == 0) {


    const char glsl[] = "GLSL.std.450";

    for (auto& inst : context()->module()->ext_inst_imports()) {
      if (inst.GetInOperand(0).AsString() == glsl) {
        module_status_.glsl_insts_id = inst.result_id();
      }
    }
    if (module_status_.glsl_insts_id == 0) {

      module_status_.glsl_insts_id = context()->TakeNextId();
      if (module_status_.glsl_insts_id == 0) {
        return 0;
      }
      std::vector<uint32_t> words = spvtools::utils::MakeVector(glsl);
      auto import_inst = MakeUnique<Instruction>(
          context(), spv::Op::OpExtInstImport, 0, module_status_.glsl_insts_id,
          std::initializer_list<Operand>{
              Operand{SPV_OPERAND_TYPE_LITERAL_STRING, std::move(words)}});
      Instruction* inst = import_inst.get();
      context()->module()->AddExtInstImport(std::move(import_inst));
      module_status_.modified = true;
      context()->AnalyzeDefUse(inst);


      context()->ResetFeatureManager();
    }
  }
  return module_status_.glsl_insts_id;
}

opt::Instruction* opt::GraphicsRobustAccessPass::GetValueForType(
    uint64_t value, const analysis::Integer* type) {
  auto* mgr = context()->get_constant_mgr();
  assert(type->width() <= 64);
  std::vector<uint32_t> words;
  words.push_back(uint32_t(value));
  if (type->width() > 32) {
    words.push_back(uint32_t(value >> 32u));
  }
  const auto* constant = mgr->GetConstant(type, words);
  return mgr->GetDefiningInstruction(
      constant, context()->get_type_mgr()->GetTypeInstruction(type));
}

opt::Instruction* opt::GraphicsRobustAccessPass::WidenInteger(
    bool sign_extend, uint32_t bit_width, Instruction* value,
    Instruction* before_inst) {
  analysis::Integer unsigned_type_for_query(bit_width, false);
  auto* type_mgr = context()->get_type_mgr();
  auto* unsigned_type = type_mgr->GetRegisteredType(&unsigned_type_for_query);
  auto type_id = context()->get_type_mgr()->GetId(unsigned_type);
  auto conversion_id = context()->TakeNextId();
  if (conversion_id == 0) {
    return nullptr;
  }
  auto* conversion = InsertInst(
      before_inst, (sign_extend ? spv::Op::OpSConvert : spv::Op::OpUConvert),
      type_id, conversion_id, {{SPV_OPERAND_TYPE_ID, {value->result_id()}}});
  return conversion;
}

Instruction* GraphicsRobustAccessPass::MakeUMinInst(
    const analysis::TypeManager& tm, Instruction* x, Instruction* y,
    Instruction* where) {



  const uint32_t glsl_insts_id = GetGlslInsts();
  if (glsl_insts_id == 0) {
    return nullptr;
  }
  uint32_t smin_id = context()->TakeNextId();
  if (smin_id == 0) {
    return nullptr;
  }
  const auto xwidth = tm.GetType(x->type_id())->AsInteger()->width();
  const auto ywidth = tm.GetType(y->type_id())->AsInteger()->width();
  assert(xwidth == ywidth);
  (void)xwidth;
  (void)ywidth;
  auto* smin_inst = InsertInst(
      where, spv::Op::OpExtInst, x->type_id(), smin_id,
      {
          {SPV_OPERAND_TYPE_ID, {glsl_insts_id}},
          {SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, {GLSLstd450UMin}},
          {SPV_OPERAND_TYPE_ID, {x->result_id()}},
          {SPV_OPERAND_TYPE_ID, {y->result_id()}},
      });
  return smin_inst;
}

Instruction* GraphicsRobustAccessPass::MakeSClampInst(
    const analysis::TypeManager& tm, Instruction* x, Instruction* min,
    Instruction* max, Instruction* where) {



  const uint32_t glsl_insts_id = GetGlslInsts();
  if (glsl_insts_id == 0) {
    return nullptr;
  }
  uint32_t clamp_id = context()->TakeNextId();
  if (clamp_id == 0) {
    return nullptr;
  }
  const auto xwidth = tm.GetType(x->type_id())->AsInteger()->width();
  const auto minwidth = tm.GetType(min->type_id())->AsInteger()->width();
  const auto maxwidth = tm.GetType(max->type_id())->AsInteger()->width();
  assert(xwidth == minwidth);
  assert(xwidth == maxwidth);
  (void)xwidth;
  (void)minwidth;
  (void)maxwidth;
  auto* clamp_inst = InsertInst(
      where, spv::Op::OpExtInst, x->type_id(), clamp_id,
      {
          {SPV_OPERAND_TYPE_ID, {glsl_insts_id}},
          {SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, {GLSLstd450SClamp}},
          {SPV_OPERAND_TYPE_ID, {x->result_id()}},
          {SPV_OPERAND_TYPE_ID, {min->result_id()}},
          {SPV_OPERAND_TYPE_ID, {max->result_id()}},
      });
  return clamp_inst;
}

Instruction* GraphicsRobustAccessPass::MakeRuntimeArrayLengthInst(
    Instruction* access_chain, uint32_t operand_index) {






  auto* type_mgr = context()->get_type_mgr();



  uint32_t steps_remaining = 2;






  Instruction* current_access_chain = access_chain;
  Instruction* pointer_to_containing_struct = nullptr;
  while (steps_remaining > 0) {
    switch (current_access_chain->opcode()) {
      case spv::Op::OpCopyObject:

        current_access_chain =
            GetDef(current_access_chain->GetSingleWordInOperand(0));
        break;
      case spv::Op::OpAccessChain:
      case spv::Op::OpInBoundsAccessChain: {
        const int first_index_operand = 3;


        const auto num_contributing_indices =
            current_access_chain == access_chain
                ? operand_index - (first_index_operand - 1)
                : current_access_chain->NumInOperands() - 1                    ;
        Instruction* base =
            GetDef(current_access_chain->GetSingleWordInOperand(0));
        if (num_contributing_indices == steps_remaining) {

          pointer_to_containing_struct = base;
          steps_remaining = 0;
          break;
        } else if (num_contributing_indices < steps_remaining) {

          steps_remaining -= num_contributing_indices;
          current_access_chain = base;
        } else {


          const int base_operand = 2;


          Instruction::OperandList ops;

          ops.push_back(current_access_chain->GetOperand(base_operand));
          const uint32_t num_indices_to_keep =
              num_contributing_indices - steps_remaining - 1;
          for (uint32_t i = 0; i <= num_indices_to_keep; i++) {
            ops.push_back(
                current_access_chain->GetOperand(first_index_operand + i));
          }


          auto* constant_mgr = context()->get_constant_mgr();
          std::vector<uint32_t> indices_for_type;
          for (uint32_t i = 0; i < ops.size() - 1; i++) {
            uint32_t index_for_type_calculation = 0;
            Instruction* index =
                GetDef(current_access_chain->GetSingleWordOperand(
                    first_index_operand + i));
            if (auto* index_constant =
                    constant_mgr->GetConstantFromInst(index)) {



              index_for_type_calculation =
                  uint32_t(index_constant->GetZeroExtendedValue());
            } else {

              index_for_type_calculation = 0;
            }
            indices_for_type.push_back(index_for_type_calculation);
          }
          auto* base_ptr_type = type_mgr->GetType(base->type_id())->AsPointer();
          auto* base_pointee_type = base_ptr_type->pointee_type();
          auto* new_access_chain_result_pointee_type =
              type_mgr->GetMemberType(base_pointee_type, indices_for_type);
          const uint32_t new_access_chain_type_id = type_mgr->FindPointerToType(
              type_mgr->GetId(new_access_chain_result_pointee_type),
              base_ptr_type->storage_class());


          const auto new_access_chain_id = context()->TakeNextId();
          if (new_access_chain_id == 0) {
            Fail();
            return nullptr;
          }
          auto* new_access_chain =
              InsertInst(current_access_chain, current_access_chain->opcode(),
                         new_access_chain_type_id, new_access_chain_id, ops);
          pointer_to_containing_struct = new_access_chain;
          steps_remaining = 0;
          break;
        }
      } break;
      default:
        Fail() << "Unhandled access chain in logical addressing mode passes "
                  "through "
               << current_access_chain->PrettyPrint(
                      SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET |
                      SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
        return nullptr;
    }
  }
  assert(pointer_to_containing_struct);
  auto* pointee_type =
      type_mgr->GetType(pointer_to_containing_struct->type_id())
          ->AsPointer()
          ->pointee_type();

  auto* struct_type = pointee_type->AsStruct();
  const uint32_t member_index_of_runtime_array =
      uint32_t(struct_type->element_types().size() - 1);


  const auto array_len_id = context()->TakeNextId();
  if (array_len_id == 0) {
    Fail();
    return nullptr;
  }
  analysis::Integer uint_type_for_query(32, false);
  auto* uint_type = type_mgr->GetRegisteredType(&uint_type_for_query);
  auto* array_len = InsertInst(
      access_chain, spv::Op::OpArrayLength, type_mgr->GetId(uint_type),
      array_len_id,
      {{SPV_OPERAND_TYPE_ID, {pointer_to_containing_struct->result_id()}},
       {SPV_OPERAND_TYPE_LITERAL_INTEGER, {member_index_of_runtime_array}}});
  return array_len;
}

spv_result_t GraphicsRobustAccessPass::ClampCoordinateForImageTexelPointer(
    opt::Instruction* image_texel_pointer) {


  (void)(image_texel_pointer);
  return SPV_SUCCESS;


#if 0















  auto* def_use_mgr = context()->get_def_use_mgr();
  auto* type_mgr = context()->get_type_mgr();
  auto* constant_mgr = context()->get_constant_mgr();

  auto* image_ptr = GetDef(image_texel_pointer->GetSingleWordInOperand(0));
  auto* image_ptr_type = GetDef(image_ptr->type_id());
  auto image_type_id = image_ptr_type->GetSingleWordInOperand(1);
  auto* image_type = GetDef(image_type_id);
  auto* coord = GetDef(image_texel_pointer->GetSingleWordInOperand(1));
  auto* samples = GetDef(image_texel_pointer->GetSingleWordInOperand(2));


  module_status_.modified = true;


  auto* feature_mgr = context()->get_feature_mgr();
  if (!feature_mgr->HasCapability(spv::Capability::ImageQuery)) {
    auto cap = MakeUnique<Instruction>(
        context(), spv::Op::OpCapability, 0, 0,
        std::initializer_list<Operand>{
            {SPV_OPERAND_TYPE_CAPABILITY, {spv::Capability::ImageQuery}}});
    def_use_mgr->AnalyzeInstDefUse(cap.get());
    context()->AddCapability(std::move(cap));
    feature_mgr->Analyze(context()->module());
  }



































  const auto dim = SpvDim(image_type->GetSingleWordInOperand(1));
  const bool arrayed = image_type->GetSingleWordInOperand(3) == 1;
  const bool multisampled = image_type->GetSingleWordInOperand(4) != 0;
  const auto query_num_components = [dim, arrayed, this]() -> int {
    const int arrayness_bonus = arrayed ? 1 : 0;
    int num_coords = 0;
    switch (dim) {
      case spv::Dim::Buffer:
      case SpvDim1D:
        num_coords = 1;
        break;
      case spv::Dim::Cube:

      case spv::Dim::Rect:
      case SpvDim2D:
        num_coords = 2;
        break;
      case SpvDim3D:
        num_coords = 3;
        break;
      case spv::Dim::SubpassData:
      case spv::Dim::Max:
        return Fail() << "Invalid image dimension for OpImageTexelPointer: "
                      << int(dim);
        break;
    }
    return num_coords + arrayness_bonus;
  }();
  const auto* coord_component_type = [type_mgr, coord]() {
    const analysis::Type* coord_type = type_mgr->GetType(coord->type_id());
    if (auto* vector_type = coord_type->AsVector()) {
      return vector_type->element_type()->AsInteger();
    }
    return coord_type->AsInteger();
  }();

  if (!coord_component_type) {
    return Fail() << " Coordinates for OpImageTexelPointer are not integral: "
                  << image_texel_pointer->PrettyPrint(
                         SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
  }
  if (coord_component_type->width() != 32) {
    return Fail() << " Expected OpImageTexelPointer coordinate components to "
                     "be 32-bits wide. They are "
                  << coord_component_type->width() << " bits. "
                  << image_texel_pointer->PrettyPrint(
                         SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
  }
  const auto* query_size_type =
      [type_mgr, coord_component_type,
       query_num_components]() -> const analysis::Type* {
    if (query_num_components == 1) return coord_component_type;
    analysis::Vector proposed(coord_component_type, query_num_components);
    return type_mgr->GetRegisteredType(&proposed);
  }();

  const uint32_t image_id = context()->TakeNextId();
  if (image_id == 0) {
    return Fail();
  }
  auto* image =
      InsertInst(image_texel_pointer, spv::Op::OpLoad, image_type_id, image_id,
                 {{SPV_OPERAND_TYPE_ID, {image_ptr->result_id()}}});

  const uint32_t query_size_id = context()->TakeNextId();
  if (query_size_id == 0) {
    return Fail();
  }
  auto* query_size =
      InsertInst(image_texel_pointer, spv::Op::OpImageQuerySize,
                 type_mgr->GetTypeInstruction(query_size_type), query_size_id,
                 {{SPV_OPERAND_TYPE_ID, {image->result_id()}}});

  auto* component_1 = constant_mgr->GetConstant(coord_component_type, {1});
  const uint32_t component_1_id =
      constant_mgr->GetDefiningInstruction(component_1)->result_id();
  auto* component_0 = constant_mgr->GetConstant(coord_component_type, {0});
  const uint32_t component_0_id =
      constant_mgr->GetDefiningInstruction(component_0)->result_id();





  auto* query_size_including_faces = query_size;
  if (arrayed && (dim == spv::Dim::Cube)) {

    auto* component_6 = constant_mgr->GetConstant(coord_component_type, {6});
    const uint32_t component_6_id =
        constant_mgr->GetDefiningInstruction(component_6)->result_id();
    assert(query_num_components == 3);
    auto* multiplicand = constant_mgr->GetConstant(
        query_size_type, {component_1_id, component_1_id, component_6_id});
    auto* multiplicand_inst =
        constant_mgr->GetDefiningInstruction(multiplicand);
    const auto query_size_including_faces_id = context()->TakeNextId();
    if (query_size_including_faces_id == 0) {
      return Fail();
    }
    query_size_including_faces = InsertInst(
        image_texel_pointer, spv::Op::OpIMul,
        type_mgr->GetTypeInstruction(query_size_type),
        query_size_including_faces_id,
        {{SPV_OPERAND_TYPE_ID, {query_size_including_faces->result_id()}},
         {SPV_OPERAND_TYPE_ID, {multiplicand_inst->result_id()}}});
  }


  auto* coordinate_1 =
      query_num_components == 1
          ? component_1
          : constant_mgr->GetConstant(
                query_size_type,
                std::vector<uint32_t>(query_num_components, component_1_id));

  auto* coordinate_0 =
      query_num_components == 0
          ? component_0
          : constant_mgr->GetConstant(
                query_size_type,
                std::vector<uint32_t>(query_num_components, component_0_id));

  const uint32_t query_max_including_faces_id = context()->TakeNextId();
  if (query_max_including_faces_id == 0) {
    return Fail();
  }
  auto* query_max_including_faces = InsertInst(
      image_texel_pointer, spv::Op::OpISub,
      type_mgr->GetTypeInstruction(query_size_type),
      query_max_including_faces_id,
      {{SPV_OPERAND_TYPE_ID, {query_size_including_faces->result_id()}},
       {SPV_OPERAND_TYPE_ID,
        {constant_mgr->GetDefiningInstruction(coordinate_1)->result_id()}}});


  auto* clamp_coord = MakeSClampInst(
      *type_mgr, coord, constant_mgr->GetDefiningInstruction(coordinate_0),
      query_max_including_faces, image_texel_pointer);
  if (clamp_coord == nullptr) {
    return Fail();
  }
  image_texel_pointer->SetInOperand(1, {clamp_coord->result_id()});


  if (multisampled) {

    const auto query_samples_id = context()->TakeNextId();
    if (query_samples_id == 0) {
      return Fail();
    }
    auto* query_samples = InsertInst(
        image_texel_pointer, spv::Op::OpImageQuerySamples,
        constant_mgr->GetDefiningInstruction(component_0)->type_id(),
        query_samples_id, {{SPV_OPERAND_TYPE_ID, {image->result_id()}}});

    const auto max_samples_id = context()->TakeNextId();
    if (max_samples_id == 0) {
      return Fail();
    }
    auto* max_samples = InsertInst(image_texel_pointer, spv::Op::OpImageQuerySamples,
                                   query_samples->type_id(), max_samples_id,
                                   {{SPV_OPERAND_TYPE_ID, {query_samples_id}},
                                    {SPV_OPERAND_TYPE_ID, {component_1_id}}});

    auto* clamp_samples = MakeSClampInst(
        *type_mgr, samples, constant_mgr->GetDefiningInstruction(coordinate_0),
        max_samples, image_texel_pointer);
    if (clamp_samples == nullptr) {
      return Fail();
    }
    image_texel_pointer->SetInOperand(2, {clamp_samples->result_id()});

  } else {

    image_texel_pointer->SetInOperand(2, {component_0_id});
  }

  def_use_mgr->AnalyzeInstUse(image_texel_pointer);

  return SPV_SUCCESS;
#endif
}

opt::Instruction* GraphicsRobustAccessPass::InsertInst(
    opt::Instruction* where_inst, spv::Op opcode, uint32_t type_id,
    uint32_t result_id, const Instruction::OperandList& operands) {
  if (result_id == 0) {
    return nullptr;
  }
  module_status_.modified = true;
  auto* result = where_inst->InsertBefore(
      MakeUnique<Instruction>(context(), opcode, type_id, result_id, operands));
  context()->get_def_use_mgr()->AnalyzeInstDefUse(result);
  auto* basic_block = context()->get_instr_block(where_inst);
  context()->set_instr_block(result, basic_block);
  return result;
}

}
}

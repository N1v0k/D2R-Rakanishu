// Copyright (c) 2025 Google LLC
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

#include "source/opt/split_combined_image_sampler_pass.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "source/opt/instruction.h"
#include "source/opt/ir_builder.h"
#include "source/opt/ir_context.h"
#include "source/opt/type_manager.h"
#include "source/opt/types.h"
#include "source/util/make_unique.h"
#include "source/util/string_utils.h"
#include "spirv/unified1/spirv.h"

namespace spvtools {
namespace opt {

#define CHECK(cond)                                          \
  {                                                          \
    if ((cond) != SPV_SUCCESS) return Pass::Status::Failure; \
  }

#define CHECK_STATUS(cond)                           \
  {                                                  \
    if (auto c = (cond); c != SPV_SUCCESS) return c; \
  }

IRContext::Analysis SplitCombinedImageSamplerPass::GetPreservedAnalyses() {
  return

      IRContext::kAnalysisDefUse


      | IRContext::kAnalysisDecorations


      | IRContext::kAnalysisCFG
      | IRContext::kAnalysisLoopAnalysis
      | IRContext::kAnalysisStructuredCFG


      | IRContext::kAnalysisTypes;
}

Pass::Status SplitCombinedImageSamplerPass::Process() {
  def_use_mgr_ = context()->get_def_use_mgr();
  type_mgr_ = context()->get_type_mgr();

  FindCombinedTextureSamplers();
  if (combined_types_to_remove_.empty() && !sampled_image_used_as_param_) {
    return Ok();
  }

  CHECK(RemapFunctions());
  CHECK(RemapVars());
  CHECK(RemoveDeadTypes());

  def_use_mgr_ = nullptr;
  type_mgr_ = nullptr;

  return Ok();
}

spvtools::DiagnosticStream SplitCombinedImageSamplerPass::Fail() {
  return std::move(
      spvtools::DiagnosticStream({}, consumer(), "", SPV_ERROR_INVALID_BINARY)
      << "split-combined-image-sampler: ");
}

void SplitCombinedImageSamplerPass::FindCombinedTextureSamplers() {
  for (auto& inst : context()->types_values()) {
    RegisterGlobal(inst.result_id());
    switch (inst.opcode()) {
      case spv::Op::OpTypeSampler:

        assert(!sampler_type_);
        sampler_type_ = &inst;
        break;

      case spv::Op::OpTypeSampledImage:
        if (!first_sampled_image_type_) {
          first_sampled_image_type_ = &inst;
        }
        combined_types_.insert(inst.result_id());
        def_use_mgr_->WhileEachUser(inst.result_id(), [&](Instruction* i) {
          sampled_image_used_as_param_ |=
              i->opcode() == spv::Op::OpTypeFunction;
          return !sampled_image_used_as_param_;
        });
        break;

      case spv::Op::OpTypeArray:
      case spv::Op::OpTypeRuntimeArray: {
        auto pointee_id = inst.GetSingleWordInOperand(0);
        if (combined_types_.find(pointee_id) != combined_types_.end()) {
          combined_types_.insert(inst.result_id());
          combined_types_to_remove_.push_back(inst.result_id());
        }
      } break;

      case spv::Op::OpTypePointer: {
        auto sc =
            static_cast<spv::StorageClass>(inst.GetSingleWordInOperand(0));
        if (sc == spv::StorageClass::UniformConstant) {
          auto pointee_id = inst.GetSingleWordInOperand(1);
          if (combined_types_.find(pointee_id) != combined_types_.end()) {
            combined_types_.insert(inst.result_id());
            combined_types_to_remove_.push_back(inst.result_id());
          }
        }
      } break;

      case spv::Op::OpVariable:
        if (combined_types_.find(inst.type_id()) != combined_types_.end()) {
          ordered_vars_.push_back(&inst);
        }
        break;

      default:
        break;
    }
  }
}

Instruction* SplitCombinedImageSamplerPass::GetSamplerType() {
  if (!sampler_type_) {
    analysis::Sampler s;
    uint32_t sampler_type_id = type_mgr_->GetTypeInstruction(&s);
    sampler_type_ = def_use_mgr_->GetDef(sampler_type_id);
    if (sampler_type_ == nullptr) return nullptr;
    assert(first_sampled_image_type_);
    sampler_type_->InsertBefore(first_sampled_image_type_);
    RegisterNewGlobal(sampler_type_->result_id());
  }
  return sampler_type_;
}

spv_result_t SplitCombinedImageSamplerPass::RemapVars() {
  for (Instruction* var : ordered_vars_) {
    CHECK_STATUS(RemapVar(var));
  }
  return SPV_SUCCESS;
}

std::pair<Instruction*, Instruction*> SplitCombinedImageSamplerPass::SplitType(
    Instruction& combined_kind_type) {
  if (auto where = type_remap_.find(combined_kind_type.result_id());
      where != type_remap_.end()) {
    auto& type_remap = where->second;
    return {type_remap.image_kind_type, type_remap.sampler_kind_type};
  }

  switch (combined_kind_type.opcode()) {
    case spv::Op::OpTypeSampledImage: {
      auto* image_type =
          def_use_mgr_->GetDef(combined_kind_type.GetSingleWordInOperand(0));
      auto* sampler_type = GetSamplerType();
      if (!sampler_type) return {nullptr, nullptr};
      type_remap_[combined_kind_type.result_id()] = {&combined_kind_type,
                                                     image_type, sampler_type};
      return {image_type, sampler_type};
      break;
    }
    case spv::Op::OpTypePointer: {
      auto sc = static_cast<spv::StorageClass>(
          combined_kind_type.GetSingleWordInOperand(0));
      if (sc == spv::StorageClass::UniformConstant) {
        auto* pointee =
            def_use_mgr_->GetDef(combined_kind_type.GetSingleWordInOperand(1));
        auto [image_pointee, sampler_pointee] = SplitType(*pointee);




        if (image_pointee && sampler_pointee) {
          auto* ptr_image = MakeUniformConstantPointer(image_pointee);
          if (!ptr_image) return {nullptr, nullptr};
          auto* ptr_sampler = MakeUniformConstantPointer(sampler_pointee);
          if (!ptr_sampler) return {nullptr, nullptr};
          type_remap_[combined_kind_type.result_id()] = {
              &combined_kind_type, ptr_image, ptr_sampler};
          return {ptr_image, ptr_sampler};
        }
      }
      break;
    }
    case spv::Op::OpTypeArray: {
      const auto* array_ty =
          type_mgr_->GetType(combined_kind_type.result_id())->AsArray();
      assert(array_ty);
      const auto* sampled_image_ty = array_ty->element_type()->AsSampledImage();
      assert(sampled_image_ty);

      const analysis::Type* image_ty = sampled_image_ty->image_type();
      assert(image_ty);
      analysis::Array array_image_ty(image_ty, array_ty->length_info());
      const uint32_t array_image_ty_id =
          type_mgr_->GetTypeInstruction(&array_image_ty);
      if (array_image_ty_id == 0) return {nullptr, nullptr};
      auto* array_image_ty_inst = def_use_mgr_->GetDef(array_image_ty_id);
      if (!IsKnownGlobal(array_image_ty_id)) {
        array_image_ty_inst->InsertBefore(&combined_kind_type);
        RegisterNewGlobal(array_image_ty_id);

      }

      auto* sampler_ty_inst = GetSamplerType();
      if (!sampler_ty_inst) return {nullptr, nullptr};
      analysis::Array sampler_array_ty(
          type_mgr_->GetType(sampler_ty_inst->result_id()),
          array_ty->length_info());
      const uint32_t array_sampler_ty_id =
          type_mgr_->GetTypeInstruction(&sampler_array_ty);
      if (array_sampler_ty_id == 0) return {nullptr, nullptr};
      auto* array_sampler_ty_inst = def_use_mgr_->GetDef(array_sampler_ty_id);
      if (!IsKnownGlobal(array_sampler_ty_id)) {
        array_sampler_ty_inst->InsertBefore(&combined_kind_type);
        RegisterNewGlobal(array_sampler_ty_id);

      }
      return {array_image_ty_inst, array_sampler_ty_inst};
    }
    case spv::Op::OpTypeRuntimeArray: {

      auto* array_ty =
          type_mgr_->GetType(combined_kind_type.result_id())->AsRuntimeArray();
      assert(array_ty);
      auto* sampled_image_ty = array_ty->element_type()->AsSampledImage();
      assert(sampled_image_ty);

      const analysis::Type* image_ty = sampled_image_ty->image_type();
      assert(image_ty);
      analysis::RuntimeArray array_image_ty(image_ty);
      const uint32_t array_image_ty_id =
          type_mgr_->GetTypeInstruction(&array_image_ty);
      if (array_image_ty_id == 0) return {nullptr, nullptr};
      auto* array_image_ty_inst = def_use_mgr_->GetDef(array_image_ty_id);
      if (!IsKnownGlobal(array_image_ty_id)) {
        array_image_ty_inst->InsertBefore(&combined_kind_type);
        RegisterNewGlobal(array_image_ty_id);

      }

      auto* sampler_ty_inst = GetSamplerType();
      if (!sampler_ty_inst) return {nullptr, nullptr};
      analysis::RuntimeArray sampler_array_ty(
          type_mgr_->GetType(sampler_ty_inst->result_id()));
      const uint32_t array_sampler_ty_id =
          type_mgr_->GetTypeInstruction(&sampler_array_ty);
      if (array_sampler_ty_id == 0) return {nullptr, nullptr};
      auto* array_sampler_ty_inst = def_use_mgr_->GetDef(array_sampler_ty_id);
      if (!IsKnownGlobal(array_sampler_ty_id)) {
        array_sampler_ty_inst->InsertBefore(&combined_kind_type);
        RegisterNewGlobal(array_sampler_ty_id);

      }
      return {array_image_ty_inst, array_sampler_ty_inst};
    }
    default:
      break;
  }
  return {nullptr, nullptr};
}

spv_result_t SplitCombinedImageSamplerPass::RemapVar(
    Instruction* combined_var) {
  InstructionBuilder builder(context(), combined_var,
                             IRContext::kAnalysisDefUse);


  auto* combined_var_type = def_use_mgr_->GetDef(combined_var->type_id());
  auto [ptr_image_ty, ptr_sampler_ty] = SplitType(*combined_var_type);
  if (!ptr_image_ty || !ptr_sampler_ty) return SPV_ERROR_INTERNAL;
  Instruction* sampler_var = builder.AddVariable(
      ptr_sampler_ty->result_id(), SpvStorageClassUniformConstant);
  if (sampler_var == nullptr) return SPV_ERROR_INTERNAL;
  Instruction* image_var = builder.AddVariable(ptr_image_ty->result_id(),
                                               SpvStorageClassUniformConstant);
  if (image_var == nullptr) return SPV_ERROR_INTERNAL;

  modified_ = true;
  return RemapUses(combined_var, image_var, sampler_var);
}

spv_result_t SplitCombinedImageSamplerPass::RemapUses(
    Instruction* combined, Instruction* image_part, Instruction* sampler_part) {

  std::unordered_set<Instruction*> dead_insts;


  InstructionBuilder builder(context(), combined, IRContext::kAnalysisDefUse);

















  struct RemapUse {
    uint32_t used_id;
    Instruction* user;
    uint32_t index;
    Instruction* image_part;
    Instruction* sampler_part;
  };

  std::vector<RemapUse> uses;



  auto add_remap = [this, &dead_insts, &uses](Instruction* combined_arg,
                                              Instruction* image_part_arg,
                                              Instruction* sampler_part_arg) {
    const uint32_t used_combined_id = combined_arg->result_id();

    def_use_mgr_->ForEachUse(
        combined_arg, [&](Instruction* user, uint32_t use_index) {
          uses.push_back({used_combined_id, user, use_index, image_part_arg,
                          sampler_part_arg});
        });
    dead_insts.insert(combined_arg);
  };

  add_remap(combined, image_part, sampler_part);



  for (size_t use_index = 0; use_index < uses.size(); ++use_index) {
    auto& use = uses[use_index];
    switch (use.user->opcode()) {
      case spv::Op::OpCopyObject: {

        add_remap(use.user, image_part, sampler_part);
        break;
      }
      case spv::Op::OpLoad: {
        assert(use.index == 2 && "variable used as non-pointer index on load");
        Instruction* load = use.user;


        assert(def_use_mgr_->GetDef(load->type_id())->opcode() ==
               spv::Op::OpTypeSampledImage);


        builder.SetInsertPoint(load);
        auto* image = builder.AddLoad(PointeeTypeId(use.image_part),
                                      use.image_part->result_id());
        if (!image) return SPV_ERROR_INTERNAL;
        auto* sampler = builder.AddLoad(PointeeTypeId(use.sampler_part),
                                        use.sampler_part->result_id());
        if (!sampler) return SPV_ERROR_INTERNAL;


        auto* deco_mgr = context()->get_decoration_mgr();
        deco_mgr->CloneDecorations(load->result_id(), image->result_id());
        deco_mgr->CloneDecorations(load->result_id(), sampler->result_id());
        deco_mgr->RemoveDecorationsFrom(load->result_id());


        auto* sampled_image = builder.AddSampledImage(
            load->type_id(), image->result_id(), sampler->result_id());
        if (!sampled_image) return SPV_ERROR_INTERNAL;

        std::unordered_set<Instruction*> users;
        def_use_mgr_->ForEachUse(
            load, [&users, sampled_image](Instruction* user, uint32_t index) {
              user->SetOperand(index, {sampled_image->result_id()});
              users.insert(user);
            });
        for (auto* user : users) {
          def_use_mgr_->AnalyzeInstUse(user);
        }
        dead_insts.insert(load);
        break;
      }
      case spv::Op::OpDecorate: {
        assert(use.index == 0 && "variable used as non-target index");
        builder.SetInsertPoint(use.user);
        spv::Decoration deco{use.user->GetSingleWordInOperand(1)};
        std::vector<uint32_t> literals;
        for (uint32_t i = 2; i < use.user->NumInOperands(); i++) {
          literals.push_back(use.user->GetSingleWordInOperand(i));
        }
        builder.AddDecoration(use.image_part->result_id(), deco, literals);
        builder.AddDecoration(use.sampler_part->result_id(), deco, literals);


        break;
      }
      case spv::Op::OpEntryPoint: {







        assert(use.index >= 3 &&
               "variable used in OpEntryPoint but not as an interface ID");
        use.user->SetOperand(use.index, {use.image_part->result_id()});
        use.user->InsertOperand(
            use.user->NumOperands(),
            {SPV_OPERAND_TYPE_ID, {use.sampler_part->result_id()}});
        def_use_mgr_->AnalyzeInstUse(use.user);
        break;
      }
      case spv::Op::OpName: {

        const auto name = use.user->GetOperand(1).AsString();
        AddOpName(use.image_part->result_id(), name + "_image");
        AddOpName(use.sampler_part->result_id(), name + "_sampler");



        break;
      }
      case spv::Op::OpFunctionCall: {





        auto& call = *use.user;

        const auto first_arg_operand_index = 3;
        for (uint32_t i = first_arg_operand_index; i < call.NumOperands();
             ++i) {
          if (use.used_id == call.GetSingleWordOperand(i)) {
            call.SetOperand(i, {use.sampler_part->result_id()});
            call.InsertOperand(
                i, {SPV_OPERAND_TYPE_ID, {use.image_part->result_id()}});
            ++i;
          }
        }
        def_use_mgr_->AnalyzeInstUse(&call);
        break;
      }
      case spv::Op::OpAccessChain:
      case spv::Op::OpInBoundsAccessChain: {
        auto* original_access_chain = use.user;
        builder.SetInsertPoint(original_access_chain);

        assert(use.index == 2);



        std::vector<uint32_t> indices;
        for (uint32_t i = 3; i < original_access_chain->NumOperands(); i++) {
          indices.push_back(original_access_chain->GetSingleWordOperand(i));
        }

        auto [result_image_part_ty, result_sampler_part_ty] =
            SplitType(*def_use_mgr_->GetDef(original_access_chain->type_id()));
        if (!result_image_part_ty || !result_sampler_part_ty)
          return Fail() << "failed to split type for access chain";
        auto* result_image_part = builder.AddOpcodeAccessChain(
            use.user->opcode(), result_image_part_ty->result_id(),
            use.image_part->result_id(), indices);
        if (!result_image_part)
          return Fail() << "failed to create access chain for image part";
        auto* result_sampler_part = builder.AddOpcodeAccessChain(
            use.user->opcode(), result_sampler_part_ty->result_id(),
            use.sampler_part->result_id(), indices);
        if (!result_sampler_part)
          return Fail() << "failed to create access chain for sampler part";


        add_remap(original_access_chain, result_image_part,
                  result_sampler_part);
        break;
      }
      default: {
        uint32_t used_type_id = def_use_mgr_->GetDef(use.used_id)->type_id();
        auto* used_type = def_use_mgr_->GetDef(used_type_id);
        if (used_type->opcode() == spv::Op::OpTypeSampledImage) {



          builder.SetInsertPoint(use.user);
          auto* sampled_image =
              builder.AddSampledImage(used_type_id, use.image_part->result_id(),
                                      use.sampler_part->result_id());
          use.user->SetOperand(use.index, {sampled_image->result_id()});
          def_use_mgr_->AnalyzeInstUse(use.user);
          break;
        }
        return Fail() << "unhandled user: " << *use.user;
      }
    }
  }

  for (auto* inst : dead_insts) {
    KillInst(inst);
  }

  return SPV_SUCCESS;
}

spv_result_t SplitCombinedImageSamplerPass::RemapFunctions() {


  {
    std::unordered_set<Instruction*> dead_insts;
    for (auto& inst : context()->types_values()) {
      if (inst.opcode() != spv::Op::OpTypeFunction) {
        continue;
      }
      analysis::Function* f_ty =
          type_mgr_->GetType(inst.result_id())->AsFunction();
      std::vector<const analysis::Type*> new_params;
      for (const auto* param_ty : f_ty->param_types()) {
        const auto param_ty_id = type_mgr_->GetId(param_ty);
        if (combined_types_.find(param_ty_id) != combined_types_.end()) {
          auto* param_type = def_use_mgr_->GetDef(param_ty_id);
          auto [image_type, sampler_type] = SplitType(*param_type);
          if (!image_type || !sampler_type) return SPV_ERROR_INTERNAL;


          new_params.push_back(type_mgr_->GetType(image_type->result_id()));
          new_params.push_back(type_mgr_->GetType(sampler_type->result_id()));
        } else {
          new_params.push_back(param_ty);
        }
      }
      if (new_params.size() != f_ty->param_types().size()) {

        analysis::Function new_f_ty(f_ty->return_type(), new_params);
        const uint32_t new_f_ty_id = type_mgr_->GetTypeInstruction(&new_f_ty);
        std::unordered_set<Instruction*> users;
        def_use_mgr_->ForEachUse(
            &inst,
            [&users, new_f_ty_id](Instruction* user, uint32_t use_index) {
              user->SetOperand(use_index, {new_f_ty_id});
              users.insert(user);
            });
        for (auto* user : users) {
          def_use_mgr_->AnalyzeInstUse(user);
        }
        dead_insts.insert(&inst);
      }
    }
    for (auto* inst : dead_insts) {
      KillInst(inst);
    }
  }


  for (Function& fn : *context()->module()) {

    struct Replacement {
      Instruction* combined;
      Instruction* image;
      Instruction* sampler;
    };
    std::vector<Replacement> replacements;
    bool error = false;

    Function::RewriteParamFn rewriter =
        [&](std::unique_ptr<Instruction>&& param,
            std::back_insert_iterator<Function::ParamList>& appender) {
          if (error) {
            return;
          }
          if (combined_types_.count(param->type_id()) == 0) {
            appender = std::move(param);
            return;
          }


          auto* combined_inst = param.release();
          auto* combined_type = def_use_mgr_->GetDef(combined_inst->type_id());
          auto [image_type, sampler_type] = SplitType(*combined_type);
          if (!image_type || !sampler_type) {
            error = true;
            return;
          }

          uint32_t image_param_id = context()->TakeNextId();
          if (image_param_id == 0) {
            error = true;
            return;
          }
          auto image_param = MakeUnique<Instruction>(
              context(), spv::Op::OpFunctionParameter, image_type->result_id(),
              image_param_id, Instruction::OperandList{});
          uint32_t sampler_param_id = context()->TakeNextId();
          if (sampler_param_id == 0) {
            error = true;
            return;
          }
          auto sampler_param = MakeUnique<Instruction>(
              context(), spv::Op::OpFunctionParameter,
              sampler_type->result_id(), sampler_param_id,
              Instruction::OperandList{});
          replacements.push_back(
              {combined_inst, image_param.get(), sampler_param.get()});
          appender = std::move(image_param);
          appender = std::move(sampler_param);
        };
    fn.RewriteParams(rewriter);

    if (error) {
      return SPV_ERROR_INTERNAL;
    }

    for (auto& r : replacements) {
      modified_ = true;
      def_use_mgr_->AnalyzeInstDefUse(r.image);
      def_use_mgr_->AnalyzeInstDefUse(r.sampler);
      CHECK_STATUS(RemapUses(r.combined, r.image, r.sampler));
    }
  }
  return SPV_SUCCESS;
}

Instruction* SplitCombinedImageSamplerPass::MakeUniformConstantPointer(
    Instruction* pointee) {
  uint32_t ptr_id = type_mgr_->FindPointerToType(
      pointee->result_id(), spv::StorageClass::UniformConstant);
  if (ptr_id == 0) return nullptr;
  auto* ptr = def_use_mgr_->GetDef(ptr_id);
  if (!IsKnownGlobal(ptr_id)) {


    ptr->InsertBefore(pointee);
    pointee->InsertBefore(ptr);
    RegisterNewGlobal(ptr_id);

  }
  return ptr;
}

void SplitCombinedImageSamplerPass::AddOpName(uint32_t id,
                                              const std::string& name) {
  std::unique_ptr<Instruction> opname{new Instruction{
      context(),
      spv::Op::OpName,
      0u,
      0u,
      {{SPV_OPERAND_TYPE_ID, {id}},
       {SPV_OPERAND_TYPE_LITERAL_STRING,
        utils::MakeVector<spvtools::opt::Operand::OperandData>(name)}}}};

  context()->AddDebug2Inst(std::move(opname));
}

spv_result_t SplitCombinedImageSamplerPass::RemoveDeadTypes() {
  for (auto dead_type_id : combined_types_to_remove_) {
    if (auto* ty = def_use_mgr_->GetDef(dead_type_id)) {
      KillInst(ty);
    }
  }
  return SPV_SUCCESS;
}

void SplitCombinedImageSamplerPass::KillInst(Instruction* inst) {


  const bool was_in_list = inst->IsInAList();
  context()->KillInst(inst);
  if (!was_in_list) {

    delete inst;
  }
  modified_ = true;
}

}
}

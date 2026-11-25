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

#include "source/opt/copy_prop_arrays.h"

#include <utility>

#include "source/opt/ir_builder.h"

namespace spvtools {
namespace opt {
namespace {

constexpr uint32_t kLoadPointerInOperand = 0;
constexpr uint32_t kStorePointerInOperand = 0;
constexpr uint32_t kStoreObjectInOperand = 1;
constexpr uint32_t kCompositeExtractObjectInOperand = 0;
constexpr uint32_t kTypePointerStorageClassInIdx = 0;
constexpr uint32_t kTypePointerPointeeInIdx = 1;
constexpr uint32_t kExtInstSetInIdx = 0;
constexpr uint32_t kExtInstOpInIdx = 1;
constexpr uint32_t kInterpolantInIdx = 2;

bool IsDebugDeclareOrValue(Instruction* di) {
  auto dbg_opcode = di->GetCommonDebugOpcode();
  return dbg_opcode == CommonDebugInfoDebugDeclare ||
         dbg_opcode == CommonDebugInfoDebugValue;
}




uint32_t GetNumberOfMembers(const analysis::Type* type, IRContext* context) {
  if (const analysis::Struct* struct_type = type->AsStruct()) {
    return static_cast<uint32_t>(struct_type->element_types().size());
  } else if (const analysis::Array* array_type = type->AsArray()) {
    const analysis::Constant* length_const =
        context->get_constant_mgr()->FindDeclaredConstant(
            array_type->LengthId());

    if (length_const == nullptr) {

      return 0;
    }
    assert(length_const->type()->AsInteger());
    return length_const->GetU32();
  } else if (const analysis::Vector* vector_type = type->AsVector()) {
    return vector_type->element_count();
  } else if (const analysis::Matrix* matrix_type = type->AsMatrix()) {
    return matrix_type->element_count();
  } else {
    return 0;
  }
}

}

Pass::Status CopyPropagateArrays::Process() {
  bool modified = false;
  for (Function& function : *get_module()) {
    if (function.IsDeclaration()) {
      continue;
    }

    BasicBlock* entry_bb = &*function.begin();

    for (auto var_inst = entry_bb->begin();
         var_inst->opcode() == spv::Op::OpVariable; ++var_inst) {
      worklist_.push(&*var_inst);
    }
  }

  while (!worklist_.empty()) {
    Instruction* var_inst = worklist_.front();
    worklist_.pop();


    Instruction* store_inst = FindStoreInstruction(&*var_inst);

    if (!store_inst) {
      continue;
    }

    std::unique_ptr<MemoryObject> source_object =
        FindSourceObjectIfPossible(&*var_inst, store_inst);

    if (source_object == nullptr) {
      continue;
    }

    if (!IsPointerToArrayType(var_inst->type_id()) &&
        source_object->GetStorageClass() != spv::StorageClass::Input) {
      continue;
    }

    uint32_t pointer_type_id = source_object->GetPointerTypeId(this);
    if (pointer_type_id == 0) {
      return Status::Failure;
    }

    if (CanUpdateUses(&*var_inst, pointer_type_id)) {
      modified = true;

      if (!PropagateObject(&*var_inst, source_object.get(), store_inst)) {
        return Status::Failure;
      }
    }
  }

  return (modified ? Status::SuccessWithChange : Status::SuccessWithoutChange);
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::FindSourceObjectIfPossible(Instruction* var_inst,
                                                Instruction* store_inst) {
  assert(var_inst->opcode() == spv::Op::OpVariable && "Expecting a variable.");



  if (!store_inst) {
    return nullptr;
  }


  if (!HasValidReferencesOnly(var_inst, store_inst)) {
    return nullptr;
  }


  std::unique_ptr<MemoryObject> source = GetSourceObjectIfAny(
      store_inst->GetSingleWordInOperand(kStoreObjectInOperand));

  if (!source) {
    return nullptr;
  }







  if (!HasNoStores(source->GetVariable())) {
    return nullptr;
  }
  return source;
}

Instruction* CopyPropagateArrays::FindStoreInstruction(
    const Instruction* var_inst) const {
  Instruction* store_inst = nullptr;
  get_def_use_mgr()->WhileEachUser(
      var_inst, [&store_inst, var_inst](Instruction* use) {
        if (use->opcode() == spv::Op::OpStore &&
            use->GetSingleWordInOperand(kStorePointerInOperand) ==
                var_inst->result_id()) {
          if (store_inst == nullptr) {
            store_inst = use;
          } else {
            store_inst = nullptr;
            return false;
          }
        }
        return true;
      });
  return store_inst;
}

bool CopyPropagateArrays::PropagateObject(Instruction* var_inst,
                                          MemoryObject* source,
                                          Instruction* insertion_point) {
  assert(var_inst->opcode() == spv::Op::OpVariable &&
         "This function propagates variables.");

  Instruction* new_access_chain = BuildNewAccessChain(insertion_point, source);
  if (!new_access_chain) return false;
  context()->KillNamesAndDecorates(var_inst);
  return UpdateUses(var_inst, new_access_chain);
}

Instruction* CopyPropagateArrays::BuildNewAccessChain(
    Instruction* insertion_point,
    CopyPropagateArrays::MemoryObject* source) const {
  InstructionBuilder builder(
      context(), insertion_point,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

  if (source->AccessChain().size() == 0) {
    return source->GetVariable();
  }

  if (!source->BuildConstants()) return nullptr;
  std::vector<uint32_t> access_ids(source->AccessChain().size());
  std::transform(
      source->AccessChain().cbegin(), source->AccessChain().cend(),
      access_ids.begin(), [](const AccessChainEntry& entry) {
        assert(entry.is_result_id && "Constants needs to be built first.");
        return entry.result_id;
      });

  return builder.AddAccessChain(source->GetPointerTypeId(this),
                                source->GetVariable()->result_id(), access_ids);
}

bool CopyPropagateArrays::HasNoStores(Instruction* ptr_inst) {
  return get_def_use_mgr()->WhileEachUser(ptr_inst, [this](Instruction* use) {
    if (use->opcode() == spv::Op::OpLoad) {
      return true;
    } else if (use->opcode() == spv::Op::OpAccessChain) {
      return HasNoStores(use);
    } else if (use->IsDecoration() || use->opcode() == spv::Op::OpName) {
      return true;
    } else if (use->opcode() == spv::Op::OpStore) {
      return false;
    } else if (use->opcode() == spv::Op::OpImageTexelPointer) {
      return true;
    } else if (use->opcode() == spv::Op::OpEntryPoint) {
      return true;
    } else if (IsInterpolationInstruction(use)) {
      return true;
    } else if (use->IsCommonDebugInstr()) {
      return true;
    }

    return false;
  });
}

bool CopyPropagateArrays::HasValidReferencesOnly(Instruction* ptr_inst,
                                                 Instruction* store_inst) {
  BasicBlock* store_block = context()->get_instr_block(store_inst);
  DominatorAnalysis* dominator_analysis =
      context()->GetDominatorAnalysis(store_block->GetParent());

  return get_def_use_mgr()->WhileEachUser(
      ptr_inst,
      [this, store_inst, dominator_analysis, ptr_inst](Instruction* use) {
        if (use->opcode() == spv::Op::OpLoad ||
            use->opcode() == spv::Op::OpImageTexelPointer) {



          return dominator_analysis->Dominates(store_inst, use);
        } else if (IsInterpolationInstruction(use)) {

          uint32_t interpolant = use->GetSingleWordInOperand(kInterpolantInIdx);
          if (interpolant !=
              store_inst->GetSingleWordInOperand(kStorePointerInOperand))
            return false;
          return dominator_analysis->Dominates(store_inst, use);
        } else if (use->opcode() == spv::Op::OpAccessChain) {
          return HasValidReferencesOnly(use, store_inst);
        } else if (use->IsDecoration() || use->opcode() == spv::Op::OpName) {
          return true;
        } else if (use->opcode() == spv::Op::OpStore) {

          return ptr_inst->opcode() == spv::Op::OpVariable &&
                 store_inst->GetSingleWordInOperand(kStorePointerInOperand) ==
                     ptr_inst->result_id();
        } else if (IsDebugDeclareOrValue(use)) {



          return true;
        }

        return false;
      });
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::GetSourceObjectIfAny(uint32_t result) {
  Instruction* result_inst = context()->get_def_use_mgr()->GetDef(result);

  switch (result_inst->opcode()) {
    case spv::Op::OpLoad:
      return BuildMemoryObjectFromLoad(result_inst);
    case spv::Op::OpCompositeExtract:
      return BuildMemoryObjectFromExtract(result_inst);
    case spv::Op::OpCompositeConstruct:
      return BuildMemoryObjectFromCompositeConstruct(result_inst);
    case spv::Op::OpCopyObject:
    case spv::Op::OpCopyLogical:
      return GetSourceObjectIfAny(result_inst->GetSingleWordInOperand(0));
    case spv::Op::OpCompositeInsert:
      return BuildMemoryObjectFromInsert(result_inst);
    default:
      return nullptr;
  }
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::BuildMemoryObjectFromLoad(Instruction* load_inst) {
  std::vector<uint32_t> components_in_reverse;
  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();

  Instruction* current_inst = def_use_mgr->GetDef(
      load_inst->GetSingleWordInOperand(kLoadPointerInOperand));








  while (current_inst->opcode() == spv::Op::OpAccessChain) {
    for (uint32_t i = current_inst->NumInOperands() - 1; i >= 1; --i) {
      uint32_t element_index_id = current_inst->GetSingleWordInOperand(i);
      components_in_reverse.push_back(element_index_id);
    }
    current_inst = def_use_mgr->GetDef(current_inst->GetSingleWordInOperand(0));
  }





  if (current_inst->opcode() != spv::Op::OpVariable) {
    return nullptr;
  }



  return std::unique_ptr<CopyPropagateArrays::MemoryObject>(
      new MemoryObject(current_inst, components_in_reverse.rbegin(),
                       components_in_reverse.rend()));
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::BuildMemoryObjectFromExtract(Instruction* extract_inst) {
  assert(extract_inst->opcode() == spv::Op::OpCompositeExtract &&
         "Expecting an OpCompositeExtract instruction.");
  std::unique_ptr<MemoryObject> result = GetSourceObjectIfAny(
      extract_inst->GetSingleWordInOperand(kCompositeExtractObjectInOperand));

  if (!result) {
    return nullptr;
  }


  std::vector<AccessChainEntry> components;
  for (uint32_t i = 1; i < extract_inst->NumInOperands(); ++i) {
    components.push_back({false, {extract_inst->GetSingleWordInOperand(i)}});
  }
  result->PushIndirection(components);
  return result;
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::BuildMemoryObjectFromCompositeConstruct(
    Instruction* conststruct_inst) {
  assert(conststruct_inst->opcode() == spv::Op::OpCompositeConstruct &&
         "Expecting an OpCompositeConstruct instruction.");





  std::unique_ptr<MemoryObject> memory_object =
      GetSourceObjectIfAny(conststruct_inst->GetSingleWordInOperand(0));

  if (!memory_object) {
    return nullptr;
  }

  if (!memory_object->IsMember()) {
    return nullptr;
  }

  AccessChainEntry last_access = memory_object->AccessChain().back();
  if (!IsAccessChainIndexValidAndEqualTo(last_access, 0)) {
    return nullptr;
  }

  memory_object->PopIndirection();
  if (memory_object->GetNumberOfMembers() !=
      conststruct_inst->NumInOperands()) {
    return nullptr;
  }

  for (uint32_t i = 1; i < conststruct_inst->NumInOperands(); ++i) {
    std::unique_ptr<MemoryObject> member_object =
        GetSourceObjectIfAny(conststruct_inst->GetSingleWordInOperand(i));

    if (!member_object) {
      return nullptr;
    }

    if (!member_object->IsMember()) {
      return nullptr;
    }

    if (!memory_object->Contains(member_object.get())) {
      return nullptr;
    }

    last_access = member_object->AccessChain().back();
    if (!IsAccessChainIndexValidAndEqualTo(last_access, i)) {
      return nullptr;
    }
  }
  return memory_object;
}

std::unique_ptr<CopyPropagateArrays::MemoryObject>
CopyPropagateArrays::BuildMemoryObjectFromInsert(Instruction* insert_inst) {
  assert(insert_inst->opcode() == spv::Op::OpCompositeInsert &&
         "Expecting an OpCompositeInsert instruction.");

  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  const analysis::Type* result_type = type_mgr->GetType(insert_inst->type_id());

  uint32_t number_of_elements = GetNumberOfMembers(result_type, context());

  if (number_of_elements == 0) {
    return nullptr;
  }

  if (insert_inst->NumInOperands() != 3) {
    return nullptr;
  }

  if (insert_inst->GetSingleWordInOperand(2) != number_of_elements - 1) {
    return nullptr;
  }

  std::unique_ptr<MemoryObject> memory_object =
      GetSourceObjectIfAny(insert_inst->GetSingleWordInOperand(0));

  if (!memory_object) {
    return nullptr;
  }

  if (!memory_object->IsMember()) {
    return nullptr;
  }

  AccessChainEntry last_access = memory_object->AccessChain().back();
  if (!IsAccessChainIndexValidAndEqualTo(last_access, number_of_elements - 1)) {
    return nullptr;
  }

  memory_object->PopIndirection();

  Instruction* current_insert =
      def_use_mgr->GetDef(insert_inst->GetSingleWordInOperand(1));
  for (uint32_t i = number_of_elements - 1; i > 0; --i) {
    if (current_insert->opcode() != spv::Op::OpCompositeInsert) {
      return nullptr;
    }

    if (current_insert->NumInOperands() != 3) {
      return nullptr;
    }

    if (current_insert->GetSingleWordInOperand(2) != i - 1) {
      return nullptr;
    }

    std::unique_ptr<MemoryObject> current_memory_object =
        GetSourceObjectIfAny(current_insert->GetSingleWordInOperand(0));

    if (!current_memory_object) {
      return nullptr;
    }

    if (!current_memory_object->IsMember()) {
      return nullptr;
    }

    if (memory_object->AccessChain().size() + 1 !=
        current_memory_object->AccessChain().size()) {
      return nullptr;
    }

    if (!memory_object->Contains(current_memory_object.get())) {
      return nullptr;
    }

    AccessChainEntry current_last_access =
        current_memory_object->AccessChain().back();
    if (!IsAccessChainIndexValidAndEqualTo(current_last_access, i - 1)) {
      return nullptr;
    }
    current_insert =
        def_use_mgr->GetDef(current_insert->GetSingleWordInOperand(1));
  }

  return memory_object;
}

bool CopyPropagateArrays::IsAccessChainIndexValidAndEqualTo(
    const AccessChainEntry& entry, uint32_t value) const {
  if (!entry.is_result_id) {
    return entry.immediate == value;
  }

  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();
  const analysis::Constant* constant =
      const_mgr->FindDeclaredConstant(entry.result_id);
  if (!constant || !constant->type()->AsInteger()) {
    return false;
  }
  return constant->GetU32() == value;
}

bool CopyPropagateArrays::IsPointerToArrayType(uint32_t type_id) {
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  analysis::Pointer* pointer_type = type_mgr->GetType(type_id)->AsPointer();
  if (pointer_type) {
    return pointer_type->pointee_type()->kind() == analysis::Type::kArray ||
           pointer_type->pointee_type()->kind() == analysis::Type::kImage;
  }
  return false;
}

bool CopyPropagateArrays::IsInterpolationInstruction(Instruction* inst) {
  if (inst->opcode() == spv::Op::OpExtInst &&
      inst->GetSingleWordInOperand(kExtInstSetInIdx) ==
          context()->get_feature_mgr()->GetExtInstImportId_GLSLstd450()) {
    uint32_t ext_inst = inst->GetSingleWordInOperand(kExtInstOpInIdx);
    switch (ext_inst) {
      case GLSLstd450InterpolateAtCentroid:
      case GLSLstd450InterpolateAtOffset:
      case GLSLstd450InterpolateAtSample:
        return true;
    }
  }
  return false;
}

bool CopyPropagateArrays::CanUpdateUses(Instruction* original_ptr_inst,
                                        uint32_t type_id) {
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();
  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();

  analysis::Type* type = type_mgr->GetType(type_id);
  if (type->AsRuntimeArray()) {
    return false;
  }

  if (!type->AsStruct() && !type->AsArray() && !type->AsPointer()) {


    return true;
  }

  return def_use_mgr->WhileEachUse(original_ptr_inst, [this, type_mgr,
                                                       const_mgr,
                                                       type](Instruction* use,
                                                             uint32_t) {
    if (IsDebugDeclareOrValue(use)) return true;

    switch (use->opcode()) {
      case spv::Op::OpLoad: {
        analysis::Pointer* pointer_type = type->AsPointer();
        uint32_t new_type_id = type_mgr->GetId(pointer_type->pointee_type());

        if (new_type_id != use->type_id()) {
          return CanUpdateUses(use, new_type_id);
        }
        return true;
      }
      case spv::Op::OpExtInst:
        if (IsInterpolationInstruction(use)) {
          return true;
        }
        return false;
      case spv::Op::OpAccessChain: {
        analysis::Pointer* pointer_type = type->AsPointer();
        const analysis::Type* pointee_type = pointer_type->pointee_type();

        std::vector<uint32_t> access_chain;
        for (uint32_t i = 1; i < use->NumInOperands(); ++i) {
          const analysis::Constant* index_const =
              const_mgr->FindDeclaredConstant(use->GetSingleWordInOperand(i));
          if (index_const) {
            access_chain.push_back(index_const->GetU32());
          } else {


            access_chain.push_back(0);



            if (pointee_type->kind() == analysis::Type::kStruct) {
              return false;
            }
          }
        }

        const analysis::Type* new_pointee_type =
            type_mgr->GetMemberType(pointee_type, access_chain);
        analysis::Pointer pointerTy(new_pointee_type,
                                    pointer_type->storage_class());
        uint32_t new_pointer_type_id =
            context()->get_type_mgr()->GetTypeInstruction(&pointerTy);
        if (new_pointer_type_id == 0) {
          return false;
        }

        if (new_pointer_type_id != use->type_id()) {
          return CanUpdateUses(use, new_pointer_type_id);
        }
        return true;
      }
      case spv::Op::OpCompositeExtract: {
        std::vector<uint32_t> access_chain;
        for (uint32_t i = 1; i < use->NumInOperands(); ++i) {
          access_chain.push_back(use->GetSingleWordInOperand(i));
        }

        const analysis::Type* new_type =
            type_mgr->GetMemberType(type, access_chain);
        uint32_t new_type_id = type_mgr->GetTypeInstruction(new_type);
        if (new_type_id == 0) {
          return false;
        }

        if (new_type_id != use->type_id()) {
          return CanUpdateUses(use, new_type_id);
        }
        return true;
      }
      case spv::Op::OpStore:



        return true;
      case spv::Op::OpImageTexelPointer:
      case spv::Op::OpName:
        return true;
      default:
        return use->IsDecoration();
    }
  });
}

bool CopyPropagateArrays::UpdateUses(Instruction* original_ptr_inst,
                                     Instruction* new_ptr_inst) {
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  analysis::ConstantManager* const_mgr = context()->get_constant_mgr();
  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();

  std::vector<std::pair<Instruction*, uint32_t> > uses;
  def_use_mgr->ForEachUse(original_ptr_inst,
                          [&uses](Instruction* use, uint32_t index) {
                            uses.push_back({use, index});
                          });

  for (auto pair : uses) {
    Instruction* use = pair.first;
    uint32_t index = pair.second;

    if (use->IsCommonDebugInstr()) {



      BasicBlock* store_block = context()->get_instr_block(new_ptr_inst);
      if (store_block) {
        Function* function = store_block->GetParent();
        DominatorAnalysis* dominator_analysis =
            context()->GetDominatorAnalysis(function);
        if (!dominator_analysis->Dominates(new_ptr_inst, use)) {
          assert(dominator_analysis->Dominates(use, new_ptr_inst));
          use->InsertAfter(new_ptr_inst);
          context()->set_instr_block(use,
                                     context()->get_instr_block(new_ptr_inst));
        }
      }

      switch (use->GetCommonDebugOpcode()) {
        case CommonDebugInfoDebugDeclare: {
          if (new_ptr_inst->opcode() == spv::Op::OpVariable ||
              new_ptr_inst->opcode() == spv::Op::OpFunctionParameter) {
            context()->ForgetUses(use);
            use->SetOperand(index, {new_ptr_inst->result_id()});
            context()->AnalyzeUses(use);
          } else {




            context()->ForgetUses(use);


            use->SetOperand(index - 2,
                            {static_cast<uint32_t>(CommonDebugInfoDebugValue)});
            use->SetOperand(index, {new_ptr_inst->result_id()});


            Instruction* dbg_expr =
                def_use_mgr->GetDef(use->GetSingleWordOperand(index + 1));
            auto* deref_expr_instr =
                context()->get_debug_info_mgr()->DerefDebugExpression(dbg_expr);
            if (!deref_expr_instr) return false;
            use->SetOperand(index + 1, {deref_expr_instr->result_id()});

            context()->AnalyzeUses(deref_expr_instr);
            context()->AnalyzeUses(use);
          }
          break;
        }
        case CommonDebugInfoDebugValue:
          context()->ForgetUses(use);
          use->SetOperand(index, {new_ptr_inst->result_id()});
          context()->AnalyzeUses(use);
          break;
        default:
          assert(false && "Don't know how to rewrite instruction");
          break;
      }
      continue;
    }

    switch (use->opcode()) {
      case spv::Op::OpLoad: {

        context()->ForgetUses(use);
        use->SetOperand(index, {new_ptr_inst->result_id()});


        Instruction* pointer_type_inst =
            def_use_mgr->GetDef(new_ptr_inst->type_id());
        uint32_t new_type_id =
            pointer_type_inst->GetSingleWordInOperand(kTypePointerPointeeInIdx);
        if (new_type_id != use->type_id()) {
          use->SetResultType(new_type_id);
          context()->AnalyzeUses(use);
          UpdateUses(use, use);
        } else {
          context()->AnalyzeUses(use);
        }

        AddUsesToWorklist(use);
      } break;
      case spv::Op::OpExtInst: {
        if (IsInterpolationInstruction(use)) {

          context()->ForgetUses(use);
          use->SetOperand(index, {new_ptr_inst->result_id()});
          context()->AnalyzeUses(use);
        } else {
          assert(false && "Don't know how to rewrite instruction");
        }
      } break;
      case spv::Op::OpAccessChain: {

        context()->ForgetUses(use);
        use->SetOperand(index, {new_ptr_inst->result_id()});



        std::vector<uint32_t> access_chain;
        for (uint32_t i = 1; i < use->NumInOperands(); ++i) {
          const analysis::Constant* index_const =
              const_mgr->FindDeclaredConstant(use->GetSingleWordInOperand(i));
          if (index_const) {
            access_chain.push_back(index_const->GetU32());
          } else {


            access_chain.push_back(0);
          }
        }

        Instruction* pointer_type_inst =
            get_def_use_mgr()->GetDef(new_ptr_inst->type_id());

        uint32_t new_pointee_type_id = GetMemberTypeId(
            pointer_type_inst->GetSingleWordInOperand(kTypePointerPointeeInIdx),
            access_chain);

        spv::StorageClass storage_class = static_cast<spv::StorageClass>(
            pointer_type_inst->GetSingleWordInOperand(
                kTypePointerStorageClassInIdx));

        uint32_t new_pointer_type_id =
            type_mgr->FindPointerToType(new_pointee_type_id, storage_class);

        if (new_pointer_type_id == 0) return false;

        if (new_pointer_type_id != use->type_id()) {
          use->SetResultType(new_pointer_type_id);
          context()->AnalyzeUses(use);
          UpdateUses(use, use);
        } else {
          context()->AnalyzeUses(use);
        }
      } break;
      case spv::Op::OpCompositeExtract: {

        context()->ForgetUses(use);
        use->SetOperand(index, {new_ptr_inst->result_id()});

        uint32_t new_type_id = new_ptr_inst->type_id();
        std::vector<uint32_t> access_chain;
        for (uint32_t i = 1; i < use->NumInOperands(); ++i) {
          access_chain.push_back(use->GetSingleWordInOperand(i));
        }

        new_type_id = GetMemberTypeId(new_type_id, access_chain);

        if (new_type_id != use->type_id()) {
          use->SetResultType(new_type_id);
          context()->AnalyzeUses(use);
          UpdateUses(use, use);
        } else {
          context()->AnalyzeUses(use);
        }
      } break;
      case spv::Op::OpStore:








        if (index == 1) {
          Instruction* target_pointer = def_use_mgr->GetDef(
              use->GetSingleWordInOperand(kStorePointerInOperand));
          Instruction* pointer_type =
              def_use_mgr->GetDef(target_pointer->type_id());
          uint32_t pointee_type_id =
              pointer_type->GetSingleWordInOperand(kTypePointerPointeeInIdx);
          uint32_t copy = GenerateCopy(original_ptr_inst, pointee_type_id, use);
          if (copy == 0) return false;

          context()->ForgetUses(use);
          use->SetInOperand(index, {copy});
          context()->AnalyzeUses(use);
        }
        break;
      case spv::Op::OpDecorate:



      case spv::Op::OpImageTexelPointer:

        context()->ForgetUses(use);
        use->SetOperand(index, {new_ptr_inst->result_id()});
        context()->AnalyzeUses(use);
        break;
      default:
        assert(false && "Don't know how to rewrite instruction");
        break;
    }
  }
  return true;
}

uint32_t CopyPropagateArrays::GetMemberTypeId(
    uint32_t id, const std::vector<uint32_t>& access_chain) const {
  for (uint32_t element_index : access_chain) {
    Instruction* type_inst = get_def_use_mgr()->GetDef(id);
    switch (type_inst->opcode()) {
      case spv::Op::OpTypeArray:
      case spv::Op::OpTypeRuntimeArray:
      case spv::Op::OpTypeMatrix:
      case spv::Op::OpTypeVector:
        id = type_inst->GetSingleWordInOperand(0);
        break;
      case spv::Op::OpTypeStruct:
        id = type_inst->GetSingleWordInOperand(element_index);
        break;
      default:
        break;
    }
    assert(id != 0 &&
           "Tried to extract from an object where it cannot be done.");
  }
  return id;
}

void CopyPropagateArrays::AddUsesToWorklist(Instruction* inst) {
  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();

  def_use_mgr->ForEachUse(inst, [this](Instruction* use, uint32_t) {
    if (use->opcode() == spv::Op::OpStore) {
      uint32_t var_id;
      Instruction* target_pointer = GetPtr(use, &var_id);
      if (target_pointer->opcode() != spv::Op::OpVariable) {
        return;
      }

      worklist_.push(target_pointer);
    }
  });
}

void CopyPropagateArrays::MemoryObject::PushIndirection(
    const std::vector<AccessChainEntry>& access_chain) {
  access_chain_.insert(access_chain_.end(), access_chain.begin(),
                       access_chain.end());
}

uint32_t CopyPropagateArrays::MemoryObject::GetNumberOfMembers() {
  IRContext* context = variable_inst_->context();
  analysis::TypeManager* type_mgr = context->get_type_mgr();

  const analysis::Type* type = type_mgr->GetType(variable_inst_->type_id());
  type = type->AsPointer()->pointee_type();

  std::vector<uint32_t> access_indices = GetAccessIds();
  type = type_mgr->GetMemberType(type, access_indices);

  return opt::GetNumberOfMembers(type, context);
}
template <class iterator>
CopyPropagateArrays::MemoryObject::MemoryObject(Instruction* var_inst,
                                                iterator begin, iterator end)
    : variable_inst_(var_inst) {
  std::transform(begin, end, std::back_inserter(access_chain_),
                 [](uint32_t id) { return AccessChainEntry{true, {id}}; });
}

std::vector<uint32_t> CopyPropagateArrays::MemoryObject::GetAccessIds() const {
  analysis::ConstantManager* const_mgr =
      variable_inst_->context()->get_constant_mgr();

  std::vector<uint32_t> indices(AccessChain().size());
  std::transform(AccessChain().cbegin(), AccessChain().cend(), indices.begin(),
                 [&const_mgr](const AccessChainEntry& entry) {
                   if (entry.is_result_id) {
                     const analysis::Constant* constant =
                         const_mgr->FindDeclaredConstant(entry.result_id);
                     return constant == nullptr ? 0 : constant->GetU32();
                   }

                   return entry.immediate;
                 });
  return indices;
}

bool CopyPropagateArrays::MemoryObject::Contains(
    CopyPropagateArrays::MemoryObject* other) {
  if (this->GetVariable() != other->GetVariable()) {
    return false;
  }

  if (AccessChain().size() > other->AccessChain().size()) {
    return false;
  }

  for (uint32_t i = 0; i < AccessChain().size(); i++) {
    if (AccessChain()[i] != other->AccessChain()[i]) {
      return false;
    }
  }
  return true;
}

bool CopyPropagateArrays::MemoryObject::BuildConstants() {
  for (auto& entry : access_chain_) {
    if (entry.is_result_id) {
      continue;
    }

    auto context = variable_inst_->context();
    analysis::Integer int_type(32, false);
    const analysis::Type* uint32_type =
        context->get_type_mgr()->GetRegisteredType(&int_type);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Constant* index_const =
        const_mgr->GetConstant(uint32_type, {entry.immediate});
    if (!index_const) return false;
    Instruction* constant_inst = const_mgr->GetDefiningInstruction(index_const);
    if (!constant_inst) return false;
    entry.result_id = constant_inst->result_id();
    entry.is_result_id = true;
  }
  return true;
}

}
}

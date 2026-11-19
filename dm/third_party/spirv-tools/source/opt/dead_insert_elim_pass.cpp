// Copyright (c) 2018 The Khronos Group Inc.
// Copyright (c) 2018 Valve Corporation
// Copyright (c) 2018 LunarG Inc.
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

#include "source/opt/dead_insert_elim_pass.h"

#include "source/opt/composite.h"
#include "source/opt/ir_context.h"
#include "source/opt/iterator.h"
#include "spirv/1.2/GLSL.std.450.h"

namespace spvtools {
namespace opt {
namespace {
constexpr uint32_t kTypeVectorCountInIdx = 1;
constexpr uint32_t kTypeMatrixCountInIdx = 1;
constexpr uint32_t kTypeArrayLengthIdInIdx = 1;
constexpr uint32_t kTypeIntWidthInIdx = 0;
constexpr uint32_t kConstantValueInIdx = 0;
constexpr uint32_t kInsertObjectIdInIdx = 0;
constexpr uint32_t kInsertCompositeIdInIdx = 1;
}

uint32_t DeadInsertElimPass::NumComponents(Instruction* typeInst) {
  switch (typeInst->opcode()) {
    case spv::Op::OpTypeVector: {
      return typeInst->GetSingleWordInOperand(kTypeVectorCountInIdx);
    } break;
    case spv::Op::OpTypeMatrix: {
      return typeInst->GetSingleWordInOperand(kTypeMatrixCountInIdx);
    } break;
    case spv::Op::OpTypeArray: {
      uint32_t lenId =
          typeInst->GetSingleWordInOperand(kTypeArrayLengthIdInIdx);
      Instruction* lenInst = get_def_use_mgr()->GetDef(lenId);
      if (lenInst->opcode() != spv::Op::OpConstant) return 0;
      uint32_t lenTypeId = lenInst->type_id();
      Instruction* lenTypeInst = get_def_use_mgr()->GetDef(lenTypeId);

      if (lenTypeInst->GetSingleWordInOperand(kTypeIntWidthInIdx) != 32)
        return 0;
      return lenInst->GetSingleWordInOperand(kConstantValueInIdx);
    } break;
    case spv::Op::OpTypeStruct: {
      return typeInst->NumInOperands();
    } break;
    default: { return 0; } break;
  }
}

void DeadInsertElimPass::MarkInsertChain(
    Instruction* insertChain, std::vector<uint32_t>* pExtIndices,
    uint32_t extOffset, std::unordered_set<uint32_t>* visited_phis) {

  Instruction* typeInst = get_def_use_mgr()->GetDef(insertChain->type_id());
  if (typeInst->opcode() == spv::Op::OpTypeArray) return;

  if (insertChain->opcode() != spv::Op::OpCompositeInsert &&
      insertChain->opcode() != spv::Op::OpPhi)
    return;


  if (pExtIndices == nullptr) {
    uint32_t cnum = NumComponents(typeInst);
    if (cnum > 0) {
      std::vector<uint32_t> extIndices;
      for (uint32_t i = 0; i < cnum; i++) {
        extIndices.clear();
        extIndices.push_back(i);
        std::unordered_set<uint32_t> sub_visited_phis;
        MarkInsertChain(insertChain, &extIndices, 0, &sub_visited_phis);
      }
      return;
    }
  }
  Instruction* insInst = insertChain;
  while (insInst->opcode() == spv::Op::OpCompositeInsert) {







    if (pExtIndices == nullptr) {
      liveInserts_.insert(insInst->result_id());
      uint32_t objId = insInst->GetSingleWordInOperand(kInsertObjectIdInIdx);
      std::unordered_set<uint32_t> obj_visited_phis;
      MarkInsertChain(get_def_use_mgr()->GetDef(objId), nullptr, 0,
                      &obj_visited_phis);


    } else if (ExtInsMatch(*pExtIndices, insInst, extOffset)) {
      liveInserts_.insert(insInst->result_id());
      uint32_t objId = insInst->GetSingleWordInOperand(kInsertObjectIdInIdx);
      std::unordered_set<uint32_t> obj_visited_phis;
      MarkInsertChain(get_def_use_mgr()->GetDef(objId), nullptr, 0,
                      &obj_visited_phis);
      break;

    } else if (ExtInsConflict(*pExtIndices, insInst, extOffset)) {
      liveInserts_.insert(insInst->result_id());


      uint32_t numInsertIndices = insInst->NumInOperands() - 2;
      if (pExtIndices->size() - extOffset > numInsertIndices) {
        uint32_t objId = insInst->GetSingleWordInOperand(kInsertObjectIdInIdx);
        std::unordered_set<uint32_t> obj_visited_phis;
        MarkInsertChain(get_def_use_mgr()->GetDef(objId), pExtIndices,
                        extOffset + numInsertIndices, &obj_visited_phis);
        break;


      } else {
        uint32_t objId = insInst->GetSingleWordInOperand(kInsertObjectIdInIdx);
        std::unordered_set<uint32_t> obj_visited_phis;
        MarkInsertChain(get_def_use_mgr()->GetDef(objId), nullptr, 0,
                        &obj_visited_phis);
      }
    }

    const uint32_t compId =
        insInst->GetSingleWordInOperand(kInsertCompositeIdInIdx);
    insInst = get_def_use_mgr()->GetDef(compId);
  }

  if (insInst->opcode() != spv::Op::OpPhi) return;


  if (visited_phis->count(insInst->result_id()) != 0) return;
  visited_phis->insert(insInst->result_id());



  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < insInst->NumInOperands(); i += 2) {
    ids.push_back(insInst->GetSingleWordInOperand(i));
  }
  std::sort(ids.begin(), ids.end());
  auto new_end = std::unique(ids.begin(), ids.end());
  for (auto id_iter = ids.begin(); id_iter != new_end; ++id_iter) {
    Instruction* pi = get_def_use_mgr()->GetDef(*id_iter);
    MarkInsertChain(pi, pExtIndices, extOffset, visited_phis);
  }
}

bool DeadInsertElimPass::EliminateDeadInserts(Function* func) {
  bool modified = false;
  bool lastmodified = true;


  while (lastmodified) {
    lastmodified = EliminateDeadInsertsOnePass(func);
    modified |= lastmodified;
  }
  return modified;
}

bool DeadInsertElimPass::EliminateDeadInsertsOnePass(Function* func) {
  bool modified = false;
  liveInserts_.clear();
  visitedPhis_.clear();

  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {

      spv::Op op = ii->opcode();
      Instruction* typeInst = get_def_use_mgr()->GetDef(ii->type_id());
      if (op != spv::Op::OpCompositeInsert &&
          (op != spv::Op::OpPhi || !spvOpcodeIsComposite(typeInst->opcode())))
        continue;




      if (op == spv::Op::OpCompositeInsert) {
        if (typeInst->opcode() == spv::Op::OpTypeArray) {
          liveInserts_.insert(ii->result_id());
          continue;
        }
      }
      const uint32_t id = ii->result_id();
      get_def_use_mgr()->ForEachUser(id, [&ii, this](Instruction* user) {
        if (user->IsCommonDebugInstr()) return;
        switch (user->opcode()) {
          case spv::Op::OpCompositeInsert:
          case spv::Op::OpPhi:

            break;
          case spv::Op::OpCompositeExtract: {

            std::vector<uint32_t> extIndices;
            uint32_t icnt = 0;
            user->ForEachInOperand([&icnt, &extIndices](const uint32_t* idp) {
              if (icnt > 0) extIndices.push_back(*idp);
              ++icnt;
            });

            std::unordered_set<uint32_t> visited_phis;
            MarkInsertChain(&*ii, &extIndices, 0, &visited_phis);
          } break;
          default: {

            std::unordered_set<uint32_t> visited_phis;
            MarkInsertChain(&*ii, nullptr, 0, &visited_phis);
          } break;
        }
      });
    }
  }

  std::vector<Instruction*> dead_instructions;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      if (ii->opcode() != spv::Op::OpCompositeInsert) continue;
      const uint32_t id = ii->result_id();
      if (liveInserts_.find(id) != liveInserts_.end()) continue;
      const uint32_t replId =
          ii->GetSingleWordInOperand(kInsertCompositeIdInIdx);
      (void)context()->ReplaceAllUsesWith(id, replId);
      dead_instructions.push_back(&*ii);
      modified = true;
    }
  }

  while (!dead_instructions.empty()) {
    Instruction* inst = dead_instructions.back();
    dead_instructions.pop_back();
    DCEInst(inst, [&dead_instructions](Instruction* other_inst) {
      auto i = std::find(dead_instructions.begin(), dead_instructions.end(),
                         other_inst);
      if (i != dead_instructions.end()) {
        dead_instructions.erase(i);
      }
    });
  }
  return modified;
}

Pass::Status DeadInsertElimPass::Process() {

  ProcessFunction pfn = [this](Function* fp) {
    return EliminateDeadInserts(fp);
  };
  bool modified = context()->ProcessReachableCallTree(pfn);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}

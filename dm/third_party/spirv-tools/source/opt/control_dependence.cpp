// Copyright (c) 2021 Google LLC.
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

#include "source/opt/control_dependence.h"

#include <cassert>
#include <tuple>

#include "source/opt/basic_block.h"
#include "source/opt/cfg.h"
#include "source/opt/dominator_analysis.h"
#include "source/opt/function.h"
#include "source/opt/instruction.h"













namespace spvtools {
namespace opt {
constexpr uint32_t ControlDependenceAnalysis::kPseudoEntryBlock;

uint32_t ControlDependence::GetConditionID(const CFG& cfg) const {
  if (source_bb_id() == 0) {

    return 0;
  }
  const BasicBlock* source_bb = cfg.block(source_bb_id());
  const Instruction* branch = source_bb->terminator();
  assert((branch->opcode() == spv::Op::OpBranchConditional ||
          branch->opcode() == spv::Op::OpSwitch) &&
         "invalid control dependence; last instruction must be conditional "
         "branch or switch");
  return branch->GetSingleWordInOperand(0);
}

bool ControlDependence::operator<(const ControlDependence& other) const {
  return std::tie(source_bb_id_, target_bb_id_, branch_target_bb_id_) <
         std::tie(other.source_bb_id_, other.target_bb_id_,
                  other.branch_target_bb_id_);
}

bool ControlDependence::operator==(const ControlDependence& other) const {
  return std::tie(source_bb_id_, target_bb_id_, branch_target_bb_id_) ==
         std::tie(other.source_bb_id_, other.target_bb_id_,
                  other.branch_target_bb_id_);
}

std::ostream& operator<<(std::ostream& os, const ControlDependence& dep) {
  os << dep.source_bb_id() << "->" << dep.target_bb_id();
  if (dep.branch_target_bb_id() != dep.target_bb_id()) {
    os << " through " << dep.branch_target_bb_id();
  }
  return os;
}

void ControlDependenceAnalysis::ComputePostDominanceFrontiers(
    const CFG& cfg, const PostDominatorAnalysis& pdom) {

















  assert(!cfg.IsPseudoExitBlock(pdom.GetDomTree().post_begin()->bb_));
  Function* function = pdom.GetDomTree().post_begin()->bb_->GetParent();
  uint32_t function_entry = function->entry()->id();


  reverse_nodes_[kPseudoEntryBlock] = {};
  for (auto it = pdom.GetDomTree().post_cbegin();
       it != pdom.GetDomTree().post_cend(); ++it) {
    ComputePostDominanceFrontierForNode(cfg, pdom, function_entry, *it);
  }
}

void ControlDependenceAnalysis::ComputePostDominanceFrontierForNode(
    const CFG& cfg, const PostDominatorAnalysis& pdom, uint32_t function_entry,
    const DominatorTreeNode& pdom_node) {
  const uint32_t label = pdom_node.id();
  ControlDependenceList& edges = reverse_nodes_[label];
  for (uint32_t pred : cfg.preds(label)) {
    if (!pdom.StrictlyDominates(label, pred)) {
      edges.push_back(ControlDependence(pred, label));
    }
  }
  if (label == function_entry) {



    edges.push_back(ControlDependence(kPseudoEntryBlock, label));
  }
  for (DominatorTreeNode* child : pdom_node) {

    for (const ControlDependence& dep : reverse_nodes_[child->id()]) {

      if (dep.source_bb_id() == kPseudoEntryBlock ||
          !pdom.StrictlyDominates(label, dep.source_bb_id())) {
        edges.push_back(ControlDependence(dep.source_bb_id(), label,
                                          dep.branch_target_bb_id()));
      }
    }
  }
}

void ControlDependenceAnalysis::ComputeControlDependenceGraph(
    const CFG& cfg, const PostDominatorAnalysis& pdom) {
  ComputePostDominanceFrontiers(cfg, pdom);
  ComputeForwardGraphFromReverse();
}

void ControlDependenceAnalysis::ComputeForwardGraphFromReverse() {
  for (const auto& entry : reverse_nodes_) {

    forward_nodes_[entry.first];
    for (const ControlDependence& dep : entry.second) {
      forward_nodes_[dep.source_bb_id()].push_back(dep);
    }
  }
}

}
}

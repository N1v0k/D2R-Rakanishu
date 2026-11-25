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

#ifndef SOURCE_OPT_CONTROL_DEPENDENCE_H_
#define SOURCE_OPT_CONTROL_DEPENDENCE_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "source/opt/cfg.h"
#include "source/opt/dominator_analysis.h"

namespace spvtools {
namespace opt {

class ControlDependence {
 public:




  uint32_t source_bb_id() const { return source_bb_id_; }


  uint32_t target_bb_id() const { return target_bb_id_; }









  uint32_t branch_target_bb_id() const { return branch_target_bb_id_; }


  ControlDependence(uint32_t source, uint32_t target)
      : source_bb_id_(source),
        target_bb_id_(target),
        branch_target_bb_id_(target) {}


  ControlDependence(uint32_t source, uint32_t target, uint32_t branch_target)
      : source_bb_id_(source),
        target_bb_id_(target),
        branch_target_bb_id_(branch_target) {}





  uint32_t GetConditionID(const CFG& cfg) const;

  bool operator==(const ControlDependence& other) const;
  bool operator!=(const ControlDependence& other) const {
    return !(*this == other);
  }


  bool operator<(const ControlDependence& other) const;
  bool operator>(const ControlDependence& other) const { return other < *this; }
  bool operator<=(const ControlDependence& other) const {
    return !(*this > other);
  }
  bool operator>=(const ControlDependence& other) const {
    return !(*this < other);
  }

 private:
  uint32_t source_bb_id_;
  uint32_t target_bb_id_;
  uint32_t branch_target_bb_id_;
};




std::ostream& operator<<(std::ostream& os, const ControlDependence& dep);









class ControlDependenceAnalysis {
 public:


  using ControlDependenceList = std::vector<ControlDependence>;
  using ControlDependenceListMap =
      std::unordered_map<uint32_t, ControlDependenceList>;




  static constexpr uint32_t kPseudoEntryBlock = 0;



  void ComputeControlDependenceGraph(const CFG& cfg,
                                     const PostDominatorAnalysis& pdom);



  const ControlDependenceList& GetDependenceTargets(uint32_t block) const {
    return forward_nodes_.at(block);
  }



  const ControlDependenceList& GetDependenceSources(uint32_t block) const {
    return reverse_nodes_.at(block);
  }





  bool WhileEachBlockLabel(std::function<bool(uint32_t)> f) const {
    for (const auto& entry : forward_nodes_) {
      if (!f(entry.first)) {
        return false;
      }
    }
    return true;
  }



  void ForEachBlockLabel(std::function<void(uint32_t)> f) const {
    WhileEachBlockLabel([&f](uint32_t label) {
      f(label);
      return true;
    });
  }




  bool HasBlock(uint32_t id) const { return forward_nodes_.count(id) > 0; }


  bool IsDependent(uint32_t a, uint32_t b) const {
    if (!HasBlock(a)) return false;


    const ControlDependenceList& a_sources = GetDependenceSources(a);
    return std::find_if(a_sources.begin(), a_sources.end(),
                        [b](const ControlDependence& dep) {
                          return dep.source_bb_id() == b;
                        }) != a_sources.end();
  }

 private:



  void ComputePostDominanceFrontiers(const CFG& cfg,
                                     const PostDominatorAnalysis& pdom);


  void ComputePostDominanceFrontierForNode(const CFG& cfg,
                                           const PostDominatorAnalysis& pdom,
                                           uint32_t function_entry,
                                           const DominatorTreeNode& pdom_node);



  void ComputeForwardGraphFromReverse();

  ControlDependenceListMap forward_nodes_;
  ControlDependenceListMap reverse_nodes_;
};

}
}

#endif

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

#ifndef SOURCE_OPT_PROPAGATOR_H_
#define SOURCE_OPT_PROPAGATOR_H_

#include <functional>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


struct Edge {
  Edge(BasicBlock* b1, BasicBlock* b2) : source(b1), dest(b2) {
    assert(source && "CFG edges cannot have a null source block.");
    assert(dest && "CFG edges cannot have a null destination block.");
  }
  BasicBlock* source;
  BasicBlock* dest;
  bool operator<(const Edge& o) const {
    return std::make_pair(source->id(), dest->id()) <
           std::make_pair(o.source->id(), o.dest->id());
  }
};








































































































































class SSAPropagator {
 public:


  enum PropStatus { kNotInteresting, kInteresting, kVarying, kFailed };

  using VisitFunction = std::function<PropStatus(Instruction*, BasicBlock**)>;

  SSAPropagator(IRContext* context, const VisitFunction& visit_fn)
      : ctx_(context), visit_fn_(visit_fn) {}





  bool Run(Function* fn);




  bool IsPhiArgExecutable(Instruction* phi, uint32_t i) const;



  bool HasStatus(Instruction* inst) const { return statuses_.count(inst); }



  PropStatus Status(Instruction* inst) const {
    return statuses_.find(inst)->second;
  }



  bool SetStatus(Instruction* inst, PropStatus status);

 private:

  void Initialize(Function* fn);



  Pass::Status Simulate(BasicBlock* block);





  Pass::Status Simulate(Instruction* instr);


  bool ShouldSimulateAgain(Instruction* instr) const {
    return do_not_simulate_.find(instr) == do_not_simulate_.end();
  }


  void DontSimulateAgain(Instruction* instr) { do_not_simulate_.insert(instr); }


  bool BlockHasBeenSimulated(BasicBlock* block) const {
    return simulated_blocks_.find(block) != simulated_blocks_.end();
  }


  void MarkBlockSimulated(BasicBlock* block) {
    simulated_blocks_.insert(block);
  }



  bool MarkEdgeExecutable(const Edge& edge) {
    return executable_edges_.insert(edge).second;
  }


  bool IsEdgeExecutable(const Edge& edge) const {
    return executable_edges_.find(edge) != executable_edges_.end();
  }


  analysis::DefUseManager* get_def_use_mgr() const {
    return ctx_->get_def_use_mgr();
  }



  void AddControlEdge(const Edge& e);



  void AddSSAEdges(Instruction* instr);


  IRContext* ctx_;





  VisitFunction visit_fn_;



  std::queue<Instruction*> ssa_edge_uses_;


  std::queue<BasicBlock*> blocks_;


  std::unordered_set<BasicBlock*> simulated_blocks_;



  std::unordered_set<Instruction*> do_not_simulate_;




  std::unordered_map<BasicBlock*, std::vector<Edge>> bb_preds_;




  std::unordered_map<BasicBlock*, std::vector<Edge>> bb_succs_;


  std::set<Edge> executable_edges_;


  std::unordered_map<Instruction*, SSAPropagator::PropStatus> statuses_;
};

std::ostream& operator<<(std::ostream& str,
                         const SSAPropagator::PropStatus& status);

}
}

#endif

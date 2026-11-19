// Copyright (c) 2022-2025 Arm Ltd.
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

#ifndef SOURCE_OPT_GRAPH_H_
#define SOURCE_OPT_GRAPH_H_

#include "source/opt/instruction.h"

namespace spvtools {
namespace opt {

struct Graph {


  inline explicit Graph(std::unique_ptr<Instruction> def_inst);
  explicit Graph(const Graph& f) = delete;





  Graph* Clone(IRContext*) const;


  Instruction& DefInst() { return *def_inst_; }
  const Instruction& DefInst() const { return *def_inst_; }


  inline void AddInput(std::unique_ptr<Instruction> inst);


  inline void AddInstruction(std::unique_ptr<Instruction> inst);


  inline void AddOutput(std::unique_ptr<Instruction> inst);



  inline void AddNonSemanticInstruction(std::unique_ptr<Instruction> inst);


  void SetGraphEnd(std::unique_ptr<Instruction> end_inst);


  inline Instruction* EndInst() { return end_inst_.get(); }
  inline const Instruction* EndInst() const { return end_inst_.get(); }


  inline uint32_t result_id() const { return def_inst_->result_id(); }


  inline uint32_t type_id() const { return def_inst_->type_id(); }



  const std::vector<std::unique_ptr<Instruction>>& instructions() const {
    return insts_;
  }



  const std::vector<std::unique_ptr<Instruction>>& inputs() const {
    return inputs_;
  }



  const std::vector<std::unique_ptr<Instruction>>& outputs() const {
    return outputs_;
  }




  void ForEachInst(const std::function<void(Instruction*)>& f,
                   bool run_on_debug_line_insts = false,
                   bool run_on_non_semantic_insts = false);
  void ForEachInst(const std::function<void(const Instruction*)>& f,
                   bool run_on_debug_line_insts = false,
                   bool run_on_non_semantic_insts = false) const;

 private:

  std::unique_ptr<Instruction> def_inst_;

  std::vector<std::unique_ptr<Instruction>> inputs_;

  std::vector<std::unique_ptr<Instruction>> insts_;

  std::vector<std::unique_ptr<Instruction>> outputs_;

  std::unique_ptr<Instruction> end_inst_;

  std::vector<std::unique_ptr<Instruction>> non_semantic_;
};

inline Graph::Graph(std::unique_ptr<Instruction> def_inst)
    : def_inst_(std::move(def_inst)) {}

inline void Graph::AddInput(std::unique_ptr<Instruction> inst) {
  inputs_.emplace_back(std::move(inst));
}

inline void Graph::AddInstruction(std::unique_ptr<Instruction> inst) {
  insts_.emplace_back(std::move(inst));
}

inline void Graph::AddOutput(std::unique_ptr<Instruction> inst) {
  outputs_.emplace_back(std::move(inst));
}

inline void Graph::AddNonSemanticInstruction(
    std::unique_ptr<Instruction> inst) {
  non_semantic_.emplace_back(std::move(inst));
}

inline void Graph::SetGraphEnd(std::unique_ptr<Instruction> end_inst) {
  end_inst_ = std::move(end_inst);
}

}
}

#endif

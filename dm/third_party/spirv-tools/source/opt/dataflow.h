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

#ifndef SOURCE_OPT_DATAFLOW_H_
#define SOURCE_OPT_DATAFLOW_H_

#include <queue>
#include <unordered_map>
#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {





class DataFlowAnalysis {
 public:


  enum class VisitResult {



    kResultChanged,



    kResultFixed,
  };

  virtual ~DataFlowAnalysis() {}



  void Run(Function* function);

 protected:
  DataFlowAnalysis(IRContext& context) : context_(context) {}






  virtual void InitializeWorklist(Function* function,
                                  bool is_first_iteration) = 0;




  virtual void EnqueueSuccessors(Instruction* inst) = 0;




  virtual VisitResult Visit(Instruction* inst) = 0;



  bool Enqueue(Instruction* inst);

  IRContext& context() { return context_; }

 private:


  VisitResult RunOnce(Function* function, bool is_first_iteration);

  IRContext& context_;
  std::unordered_map<Instruction*, bool> on_worklist_;













  std::queue<Instruction*> worklist_;
};


class ForwardDataFlowAnalysis : public DataFlowAnalysis {
 public:

  enum class LabelPosition {

    kLabelsAtBeginning,

    kLabelsAtEnd,

    kNoLabels,

    kLabelsOnly,
  };

  ForwardDataFlowAnalysis(IRContext& context, LabelPosition label_position)
      : DataFlowAnalysis(context), label_position_(label_position) {}

 protected:



  void InitializeWorklist(Function* function, bool is_first_iteration) override;



  void EnqueueSuccessors(Instruction* inst) override {
    EnqueueUsers(inst);
    EnqueueBlockSuccessors(inst);
  }


  void EnqueueUsers(Instruction* inst);



  void EnqueueBlockSuccessors(Instruction* inst);

 private:
  LabelPosition label_position_;
};

}
}

#endif

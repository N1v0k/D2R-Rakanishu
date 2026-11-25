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

#ifndef SOURCE_OPT_REGISTER_PRESSURE_H_
#define SOURCE_OPT_REGISTER_PRESSURE_H_

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/function.h"
#include "source/opt/types.h"

namespace spvtools {
namespace opt {

class IRContext;
class Loop;
class LoopDescriptor;




class RegisterLiveness {
 public:

  struct RegisterClass {
    analysis::Type* type_;
    bool is_uniform_;

    bool operator==(const RegisterClass& rhs) const {
      return std::tie(type_, is_uniform_) ==
             std::tie(rhs.type_, rhs.is_uniform_);
    }
  };

  struct RegionRegisterLiveness {
    using LiveSet = std::unordered_set<Instruction*>;
    using RegClassSetTy = std::vector<std::pair<RegisterClass, size_t>>;


    LiveSet live_in_;

    LiveSet live_out_;


    size_t used_registers_;

    RegClassSetTy registers_classes_;

    void Clear() {
      live_out_.clear();
      live_in_.clear();
      used_registers_ = 0;
      registers_classes_.clear();
    }

    void AddRegisterClass(const RegisterClass& reg_class) {
      auto it = std::find_if(
          registers_classes_.begin(), registers_classes_.end(),
          [&reg_class](const std::pair<RegisterClass, size_t>& class_count) {
            return class_count.first == reg_class;
          });
      if (it != registers_classes_.end()) {
        it->second++;
      } else {
        registers_classes_.emplace_back(std::move(reg_class),
                                        static_cast<size_t>(1));
      }
    }

    void AddRegisterClass(Instruction* insn);
  };

  RegisterLiveness(IRContext* context, Function* f) : context_(context) {
    Analyze(f);
  }



  const RegionRegisterLiveness* Get(const BasicBlock* bb) const {
    return Get(bb->id());
  }



  const RegionRegisterLiveness* Get(uint32_t bb_id) const {
    RegionRegisterLivenessMap::const_iterator it = block_pressure_.find(bb_id);
    if (it != block_pressure_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  IRContext* GetContext() const { return context_; }



  RegionRegisterLiveness* Get(const BasicBlock* bb) { return Get(bb->id()); }



  RegionRegisterLiveness* Get(uint32_t bb_id) {
    RegionRegisterLivenessMap::iterator it = block_pressure_.find(bb_id);
    if (it != block_pressure_.end()) {
      return &it->second;
    }
    return nullptr;
  }



  RegionRegisterLiveness* GetOrInsert(uint32_t bb_id) {
    return &block_pressure_[bb_id];
  }





  void ComputeLoopRegisterPressure(const Loop& loop,
                                   RegionRegisterLiveness* reg_pressure) const;



  void SimulateFusion(const Loop& l1, const Loop& l2,
                      RegionRegisterLiveness* simulation_result) const;









  void SimulateFission(
      const Loop& loop,
      const std::unordered_set<Instruction*>& moved_instructions,
      const std::unordered_set<Instruction*>& copied_instructions,
      RegionRegisterLiveness* loop1_sim_result,
      RegionRegisterLiveness* loop2_sim_result) const;

 private:
  using RegionRegisterLivenessMap =
      std::unordered_map<uint32_t, RegionRegisterLiveness>;

  IRContext* context_;
  RegionRegisterLivenessMap block_pressure_;

  void Analyze(Function* f);
};




class LivenessAnalysis {
  using LivenessAnalysisMap =
      std::unordered_map<const Function*, RegisterLiveness>;

 public:
  LivenessAnalysis(IRContext* context) : context_(context) {}




  const RegisterLiveness* Get(Function* f) {
    LivenessAnalysisMap::iterator it = analysis_cache_.find(f);
    if (it != analysis_cache_.end()) {
      return &it->second;
    }
    return &analysis_cache_.emplace(f, RegisterLiveness{context_, f})
                .first->second;
  }

 private:
  IRContext* context_;
  LivenessAnalysisMap analysis_cache_;
};

}
}

#endif

// Copyright (c) 2022 The Khronos Group Inc.
// Copyright (c) 2022 LunarG Inc.
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

#ifndef SOURCE_OPT_LIVENESS_H_
#define SOURCE_OPT_LIVENESS_H_

#include <cstdint>
#include <unordered_set>

namespace spvtools {
namespace opt {

class IRContext;
class Instruction;

namespace analysis {

class Type;


class LivenessManager {
 public:
  LivenessManager(IRContext* ctx);


  void GetLiveness(std::unordered_set<uint32_t>* live_locs,
                   std::unordered_set<uint32_t>* live_builtins);


  bool IsAnalyzedBuiltin(uint32_t bi);





  uint32_t AnalyzeAccessChainLoc(const Instruction* ac, uint32_t curr_type_id,
                                 uint32_t* offset, bool* no_loc, bool is_patch,
                                 bool input = true);


  uint32_t GetLocSize(const analysis::Type* type) const;

 private:
  IRContext* context() const { return ctx_; }


  void InitializeAnalysis();



  bool AnalyzeBuiltIn(uint32_t id);


  void MarkRefLive(const Instruction* user, Instruction* var);


  void MarkLocsLive(uint32_t start, uint32_t count);


  uint32_t GetComponentType(uint32_t index, uint32_t agg_type_id) const;



  uint32_t GetLocOffset(uint32_t index, uint32_t agg_type_id) const;


  void ComputeLiveness();


  IRContext* ctx_;


  bool computed_;


  std::unordered_set<uint32_t> live_locs_;


  std::unordered_set<uint32_t> live_builtins_;
};

}
}
}

#endif

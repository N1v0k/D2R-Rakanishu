// Copyright (c) 2016 Google Inc.
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

#ifndef SOURCE_OPT_PASS_MANAGER_H_
#define SOURCE_OPT_PASS_MANAGER_H_

#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "source/opt/log.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

#include "source/opt/ir_context.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {




class PassManager {
 public:





  PassManager()
      : consumer_(nullptr),
        print_all_stream_(nullptr),
        time_report_stream_(nullptr),
        target_env_(SPV_ENV_UNIVERSAL_1_2),
        val_options_(nullptr),
        validate_after_all_(false) {}


  void SetMessageConsumer(MessageConsumer c) { consumer_ = std::move(c); }


  void AddPass(std::unique_ptr<Pass> pass);



  template <typename T, typename... Args>
  void AddPass(Args&&... args);


  uint32_t NumPasses() const;

  inline Pass* GetPass(uint32_t index) const;


  inline const MessageConsumer& consumer() const;








  Pass::Status Run(IRContext* context);




  PassManager& SetPrintAll(std::ostream* out) {
    print_all_stream_ = out;
    return *this;
  }




  PassManager& SetTimeReport(std::ostream* out) {
    time_report_stream_ = out;
    return *this;
  }


  PassManager& SetTargetEnv(spv_target_env env) {
    target_env_ = env;
    return *this;
  }


  PassManager& SetValidatorOptions(spv_validator_options options) {
    val_options_ = options;
    return *this;
  }


  PassManager& SetValidateAfterAll(bool validate) {
    validate_after_all_ = validate;
    return *this;
  }

 private:

  MessageConsumer consumer_;

  std::vector<std::unique_ptr<Pass>> passes_;


  std::ostream* print_all_stream_;


  std::ostream* time_report_stream_;

  spv_target_env target_env_;

  spv_validator_options val_options_;

  bool validate_after_all_;
};

inline void PassManager::AddPass(std::unique_ptr<Pass> pass) {
  passes_.push_back(std::move(pass));
}

template <typename T, typename... Args>
inline void PassManager::AddPass(Args&&... args) {
  passes_.emplace_back(new T(std::forward<Args>(args)...));
  passes_.back()->SetMessageConsumer(consumer_);
}

inline uint32_t PassManager::NumPasses() const {
  return static_cast<uint32_t>(passes_.size());
}

inline Pass* PassManager::GetPass(uint32_t index) const {
  SPIRV_ASSERT(consumer_, index < passes_.size(), "index out of bound");
  return passes_[index].get();
}

inline const MessageConsumer& PassManager::consumer() const {
  return consumer_;
}

}
}

#endif

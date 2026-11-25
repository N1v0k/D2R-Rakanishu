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

#ifndef INCLUDE_SPIRV_TOOLS_LINTER_HPP_
#define INCLUDE_SPIRV_TOOLS_LINTER_HPP_

#include "libspirv.hpp"

namespace spvtools {






class SPIRV_TOOLS_EXPORT Linter {
 public:
  explicit Linter(spv_target_env env);

  ~Linter();



  void SetMessageConsumer(MessageConsumer consumer);


  const MessageConsumer& Consumer() const;

  bool Run(const uint32_t* binary, size_t binary_size);

 private:
  struct SPIRV_TOOLS_LOCAL Impl;
  std::unique_ptr<Impl> impl_;
};
}

#endif

// Copyright (c) 2015-2016 The Khronos Group Inc.
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

#ifndef SOURCE_DIAGNOSTIC_H_
#define SOURCE_DIAGNOSTIC_H_

#include <sstream>
#include <string>

#include "spirv-tools/libspirv.hpp"

namespace spvtools {





class DiagnosticStream {
 public:
  DiagnosticStream(spv_position_t position, const MessageConsumer& consumer,
                   const std::string& disassembled_instruction,
                   spv_result_t error, std::string shader_debug_info = "")
      : position_(position),
        consumer_(consumer),
        disassembled_instruction_(disassembled_instruction),
        error_(error),
        shader_debug_info_(shader_debug_info) {}




  DiagnosticStream(DiagnosticStream&& other);




  ~DiagnosticStream();


  template <typename T>
  DiagnosticStream& operator<<(const T& val) {
    stream_ << val;
    return *this;
  }


  operator spv_result_t() { return error_; }

 private:
  std::ostringstream stream_;
  spv_position_t position_;
  MessageConsumer consumer_;
  std::string disassembled_instruction_;
  spv_result_t error_;

  std::string shader_debug_info_;
};






void UseDiagnosticAsMessageConsumer(spv_context context,
                                    spv_diagnostic* diagnostic);

std::string spvResultToString(spv_result_t res);

}

#endif

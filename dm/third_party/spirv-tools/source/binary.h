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

#ifndef SOURCE_BINARY_H_
#define SOURCE_BINARY_H_

#include <string>

#include "source/spirv_definition.h"
#include "spirv-tools/libspirv.h"






spv_result_t spvBinaryHeaderGet(const spv_const_binary binary,
                                const spv_endianness_t endian,
                                spv_header_t* header);





size_t spv_strnlen_s(const char* str, size_t strsz);



spv_result_t spvBinaryParseWithOptions(
    const spv_const_context context, void* user_data, const uint32_t* code,
    const size_t num_words, spv_parsed_header_fn_t parsed_header,
    spv_parsed_instruction_fn_t parsed_instruction, spv_diagnostic* diagnostic,
    uint32_t options);



std::string spvDecodeLiteralStringOperand(const spv_parsed_instruction_t& inst,
                                          const uint16_t operand_index);

#endif

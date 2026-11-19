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

#ifndef SOURCE_OPERAND_H_
#define SOURCE_OPERAND_H_

#include <functional>
#include <vector>

#include "source/table.h"
#include "source/util/span.h"
#include "spirv-tools/libspirv.h"













using spv_operand_pattern_t = std::vector<spv_operand_type_t>;


const char* spvOperandTypeStr(spv_operand_type_t type);


bool spvOperandIsOptional(spv_operand_type_t type);






bool spvOperandIsVariable(spv_operand_type_t type);



void spvPushOperandTypes(
    const spvtools::utils::Span<const spv_operand_type_t>& types,
    spv_operand_pattern_t* pattern);









void spvPushOperandTypesForMask(const spv_operand_type_t mask_type,
                                const uint32_t mask,
                                spv_operand_pattern_t* pattern);




















bool spvExpandOperandSequenceOnce(spv_operand_type_t type,
                                  spv_operand_pattern_t* pattern);







spv_operand_type_t spvTakeFirstMatchableOperand(spv_operand_pattern_t* pattern);



spv_operand_pattern_t spvAlternatePatternFollowingImmediate(
    const spv_operand_pattern_t& pattern);


bool spvIsIdType(spv_operand_type_t type);


bool spvIsInIdType(spv_operand_type_t type);





std::function<bool(unsigned)> spvOperandCanBeForwardDeclaredFunction(
    spv::Op opcode);





std::function<bool(unsigned)> spvDbgInfoExtOperandCanBeForwardDeclaredFunction(
    spv::Op opcode, spv_ext_inst_type_t ext_type, uint32_t key);


spv_fp_encoding_t spvFPEncodingFromOperandFPEncoding(spv::FPEncoding encoding);

#endif

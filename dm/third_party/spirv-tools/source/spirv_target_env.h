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

#ifndef SOURCE_SPIRV_TARGET_ENV_H_
#define SOURCE_SPIRV_TARGET_ENV_H_

#include <string>
#include <utility>
#include <vector>

#include "spirv-tools/libspirv.h"


bool spvIsVulkanEnv(spv_target_env env);


bool spvIsOpenCLEnv(spv_target_env env);


bool spvIsOpenGLEnv(spv_target_env env);


bool spvIsValidEnv(spv_target_env env);


uint32_t spvVersionForTargetEnv(spv_target_env env);



std::string spvLogStringForEnv(spv_target_env env);







std::string spvTargetEnvList(const int pad, const int wrap);



bool spvReadEnvironmentFromText(const std::vector<char>& text,
                                spv_target_env* env);

#endif

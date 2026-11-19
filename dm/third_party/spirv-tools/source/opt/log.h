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

#ifndef SOURCE_OPT_LOG_H_
#define SOURCE_OPT_LOG_H_

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

#include "spirv-tools/libspirv.hpp"












#if !defined(NDEBUG)
#define SPIRV_ASSERT(consumer, ...) SPIRV_ASSERT_IMPL(consumer, __VA_ARGS__)
#else


#define SPIRV_ASSERT(consumer, ...) (void)(consumer)
#endif








#if !defined(NDEBUG) && defined(SPIRV_LOG_DEBUG)
#define SPIRV_DEBUG(consumer, ...) SPIRV_DEBUG_IMPL(consumer, __VA_ARGS__)
#else


#define SPIRV_DEBUG(consumer, ...) (void)(consumer)
#endif


#define SPIRV_CONCATENATE(a, b) SPIRV_CONCATENATE_(a, b)
#define SPIRV_CONCATENATE_(a, b) a##b


#define PP_EXPAND(x) x

namespace spvtools {



inline void Log(const MessageConsumer& consumer, spv_message_level_t level,
                const char* source, const spv_position_t& position,
                const char* message) {
  if (consumer != nullptr) consumer(level, source, position, message);
}




template <typename... Args>
void Logf(const MessageConsumer& consumer, spv_message_level_t level,
          const char* source, const spv_position_t& position,
          const char* format, Args&&... args) {
#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf _snprintf
#endif

  enum { kInitBufferSize = 256 };

  char message[kInitBufferSize];
  const int size =
      snprintf(message, kInitBufferSize, format, std::forward<Args>(args)...);

  if (size >= 0 && size < kInitBufferSize) {
    Log(consumer, level, source, position, message);
    return;
  }

  if (size >= 0) {



    std::vector<char> longer_message(size + 1u);
    snprintf(longer_message.data(), longer_message.size(), format,
             std::forward<Args>(args)...);
    Log(consumer, level, source, position, longer_message.data());
    return;
  }

  Log(consumer, level, source, position, "cannot compose log message");

#if defined(_MSC_VER) && _MSC_VER < 1900
#undef snprintf
#endif
}



inline void Error(const MessageConsumer& consumer, const char* source,
                  const spv_position_t& position, const char* message) {
  Log(consumer, SPV_MSG_ERROR, source, position, message);
}




template <typename... Args>
inline void Errorf(const MessageConsumer& consumer, const char* source,
                   const spv_position_t& position, const char* format,
                   Args&&... args) {
  Logf(consumer, SPV_MSG_ERROR, source, position, format,
       std::forward<Args>(args)...);
}

}

#define SPIRV_ASSERT_IMPL(consumer, ...)                             \
  PP_EXPAND(SPIRV_CONCATENATE(SPIRV_ASSERT_, PP_NARGS(__VA_ARGS__))( \
      consumer, __VA_ARGS__))

#define SPIRV_DEBUG_IMPL(consumer, ...)                             \
  PP_EXPAND(SPIRV_CONCATENATE(SPIRV_DEBUG_, PP_NARGS(__VA_ARGS__))( \
      consumer, __VA_ARGS__))

#define SPIRV_ASSERT_1(consumer, condition)                     \
  do {                                                          \
    if (!(condition)) {                                         \
      spvtools::Log(consumer, SPV_MSG_INTERNAL_ERROR, __FILE__, \
                    {static_cast<size_t>(__LINE__), 0, 0},      \
                    "assertion failed: " #condition);           \
      std::exit(EXIT_FAILURE);                                  \
    }                                                           \
  } while (0)

#define SPIRV_ASSERT_2(consumer, condition, message)            \
  do {                                                          \
    if (!(condition)) {                                         \
      spvtools::Log(consumer, SPV_MSG_INTERNAL_ERROR, __FILE__, \
                    {static_cast<size_t>(__LINE__), 0, 0},      \
                    "assertion failed: " message);              \
      std::exit(EXIT_FAILURE);                                  \
    }                                                           \
  } while (0)

#define SPIRV_ASSERT_more(consumer, condition, format, ...)      \
  do {                                                           \
    if (!(condition)) {                                          \
      spvtools::Logf(consumer, SPV_MSG_INTERNAL_ERROR, __FILE__, \
                     {static_cast<size_t>(__LINE__), 0, 0},      \
                     "assertion failed: " format, __VA_ARGS__);  \
      std::exit(EXIT_FAILURE);                                   \
    }                                                            \
  } while (0)

#define SPIRV_ASSERT_3(consumer, condition, format, ...) \
  SPIRV_ASSERT_more(consumer, condition, format, __VA_ARGS__)

#define SPIRV_ASSERT_4(consumer, condition, format, ...) \
  SPIRV_ASSERT_more(consumer, condition, format, __VA_ARGS__)

#define SPIRV_ASSERT_5(consumer, condition, format, ...) \
  SPIRV_ASSERT_more(consumer, condition, format, __VA_ARGS__)

#define SPIRV_DEBUG_1(consumer, message)                           \
  do {                                                             \
    spvtools::Log(consumer, SPV_MSG_DEBUG, __FILE__,               \
                  {static_cast<size_t>(__LINE__), 0, 0}, message); \
  } while (0)

#define SPIRV_DEBUG_more(consumer, format, ...)                   \
  do {                                                            \
    spvtools::Logf(consumer, SPV_MSG_DEBUG, __FILE__,             \
                   {static_cast<size_t>(__LINE__), 0, 0}, format, \
                   __VA_ARGS__);                                  \
  } while (0)

#define SPIRV_DEBUG_2(consumer, format, ...) \
  SPIRV_DEBUG_more(consumer, format, __VA_ARGS__)

#define SPIRV_DEBUG_3(consumer, format, ...) \
  SPIRV_DEBUG_more(consumer, format, __VA_ARGS__)

#define SPIRV_DEBUG_4(consumer, format, ...) \
  SPIRV_DEBUG_more(consumer, format, __VA_ARGS__)

#define SPIRV_DEBUG_5(consumer, format, ...) \
  SPIRV_DEBUG_more(consumer, format, __VA_ARGS__)


#define PP_NARGS(...) PP_EXPAND(PP_ARG_N(__VA_ARGS__, 5, 4, 3, 2, 1, 0))
#define PP_ARG_N(_1, _2, _3, _4, _5, N, ...) N


static_assert(PP_NARGS(0) == 1, "PP_NARGS macro error");
static_assert(PP_NARGS(0, 0) == 2, "PP_NARGS macro error");
static_assert(PP_NARGS(0, 0, 0) == 3, "PP_NARGS macro error");
static_assert(PP_NARGS(0, 0, 0, 0) == 4, "PP_NARGS macro error");
static_assert(PP_NARGS(0, 0, 0, 0, 0) == 5, "PP_NARGS macro error");
static_assert(PP_NARGS(1 + 1, 2, 3 / 3) == 3, "PP_NARGS macro error");
static_assert(PP_NARGS((1, 1), 2, (3, 3)) == 3, "PP_NARGS macro error");

#endif

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

#ifndef SOURCE_OPT_SCALAR_ANALYSIS_H_
#define SOURCE_OPT_SCALAR_ANALYSIS_H_

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/instruction.h"
#include "source/opt/scalar_analysis_nodes.h"

namespace spvtools {
namespace opt {

class IRContext;
class Loop;







class ScalarEvolutionAnalysis {
 public:
  explicit ScalarEvolutionAnalysis(IRContext* context);


  SENode* CreateNegation(SENode* operand);



  SENode* CreateSubtraction(SENode* operand_1, SENode* operand_2);




  SENode* CreateAddNode(SENode* operand_1, SENode* operand_2);


  SENode* CreateMultiplyNode(SENode* operand_1, SENode* operand_2);


  SENode* CreateConstant(int64_t integer);


  SENode* CreateValueUnknownNode(const Instruction* inst);


  SENode* CreateCantComputeNode();



  SENode* CreateRecurrentExpression(const Loop* loop, SENode* offset,
                                    SENode* coefficient);


  SENode* AnalyzeInstruction(const Instruction* inst);











  SENode* SimplifyExpression(SENode* node);



  SENode* GetCachedOrAdd(std::unique_ptr<SENode> prospective_node);


  bool IsLoopInvariant(const Loop* loop, const SENode* node) const;




  bool IsAlwaysGreaterThanZero(SENode* node, bool* is_gt_zero) const;



  bool IsAlwaysGreaterOrEqualToZero(SENode* node, bool* is_ge_zero) const;





  SENode* GetCoefficientFromRecurrentTerm(SENode* node, const Loop* loop);



  SENode* BuildGraphWithoutRecurrentTerm(SENode* node, const Loop* loop);



  SERecurrentNode* GetRecurrentTerm(SENode* node, const Loop* loop);

  SENode* UpdateChildNode(SENode* parent, SENode* child, SENode* new_child);




  void AddLoopsToPretendAreTheSame(
      const std::pair<const Loop*, const Loop*>& loop_pair) {
    pretend_equal_[std::get<1>(loop_pair)] = std::get<0>(loop_pair);
  }

 private:
  SENode* AnalyzeConstant(const Instruction* inst);




  SENode* AnalyzeAddOp(const Instruction* instruction);

  SENode* AnalyzeMultiplyOp(const Instruction* multiply);

  SENode* AnalyzePhiInstruction(const Instruction* phi);

  IRContext* context_;





  std::map<const Instruction*, SENode*> recurrent_node_map_;



  SENode* cached_cant_compute_;




  struct NodePointersEquality {
    bool operator()(const std::unique_ptr<SENode>& lhs,
                    const std::unique_ptr<SENode>& rhs) const {
      return *lhs == *rhs;
    }
  };



  std::unordered_set<std::unique_ptr<SENode>, SENodeHash, NodePointersEquality>
      node_cache_;



  std::map<const Loop*, const Loop*> pretend_equal_;
};


class SExpression {
 public:

  SExpression(SENode* node)
      : node_(node->GetParentAnalysis()->SimplifyExpression(node)),
        scev_(node->GetParentAnalysis()) {}

  inline operator SENode*() const { return node_; }
  inline SENode* operator->() const { return node_; }
  const SENode& operator*() const { return *node_; }

  inline ScalarEvolutionAnalysis* GetScalarEvolutionAnalysis() const {
    return scev_;
  }

  inline SExpression operator+(SENode* rhs) const;
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  inline SExpression operator+(T integer) const;
  inline SExpression operator+(SExpression rhs) const;

  inline SExpression operator-() const;
  inline SExpression operator-(SENode* rhs) const;
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  inline SExpression operator-(T integer) const;
  inline SExpression operator-(SExpression rhs) const;

  inline SExpression operator*(SENode* rhs) const;
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  inline SExpression operator*(T integer) const;
  inline SExpression operator*(SExpression rhs) const;

  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  inline std::pair<SExpression, int64_t> operator/(T integer) const;



  std::pair<SExpression, int64_t> operator/(SExpression rhs) const;

 private:
  SENode* node_;
  ScalarEvolutionAnalysis* scev_;
};

inline SExpression SExpression::operator+(SENode* rhs) const {
  return scev_->CreateAddNode(node_, rhs);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression SExpression::operator+(T integer) const {
  return *this + scev_->CreateConstant(integer);
}

inline SExpression SExpression::operator+(SExpression rhs) const {
  return *this + rhs.node_;
}

inline SExpression SExpression::operator-() const {
  return scev_->CreateNegation(node_);
}

inline SExpression SExpression::operator-(SENode* rhs) const {
  return *this + scev_->CreateNegation(rhs);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression SExpression::operator-(T integer) const {
  return *this - scev_->CreateConstant(integer);
}

inline SExpression SExpression::operator-(SExpression rhs) const {
  return *this - rhs.node_;
}

inline SExpression SExpression::operator*(SENode* rhs) const {
  return scev_->CreateMultiplyNode(node_, rhs);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression SExpression::operator*(T integer) const {
  return *this * scev_->CreateConstant(integer);
}

inline SExpression SExpression::operator*(SExpression rhs) const {
  return *this * rhs.node_;
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline std::pair<SExpression, int64_t> SExpression::operator/(T integer) const {
  return *this / scev_->CreateConstant(integer);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression operator+(T lhs, SExpression rhs) {
  return rhs + lhs;
}
inline SExpression operator+(SENode* lhs, SExpression rhs) { return rhs + lhs; }

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression operator-(T lhs, SExpression rhs) {

  return SExpression{rhs.GetScalarEvolutionAnalysis()->CreateConstant(lhs)} -
         rhs;
}
inline SExpression operator-(SENode* lhs, SExpression rhs) {

  return SExpression{lhs} - rhs;
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline SExpression operator*(T lhs, SExpression rhs) {
  return rhs * lhs;
}
inline SExpression operator*(SENode* lhs, SExpression rhs) { return rhs * lhs; }

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type>
inline std::pair<SExpression, int64_t> operator/(T lhs, SExpression rhs) {

  return SExpression{rhs.GetScalarEvolutionAnalysis()->CreateConstant(lhs)} /
         rhs;
}
inline std::pair<SExpression, int64_t> operator/(SENode* lhs, SExpression rhs) {

  return SExpression{lhs} / rhs;
}

}
}
#endif

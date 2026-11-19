// Copyright (c) 2018 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASI,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SOURCE_OPT_SCALAR_ANALYSIS_NODES_H_
#define SOURCE_OPT_SCALAR_ANALYSIS_NODES_H_

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "source/opt/tree_iterator.h"

namespace spvtools {
namespace opt {

class Loop;
class ScalarEvolutionAnalysis;
class SEConstantNode;
class SERecurrentNode;
class SEAddNode;
class SEMultiplyNode;
class SENegative;
class SEValueUnknown;
class SECantCompute;






class SENode {
 public:
  enum SENodeType {
    Constant,
    RecurrentAddExpr,
    Add,
    Multiply,
    Negative,
    ValueUnknown,
    CanNotCompute
  };

  using ChildContainerType = std::vector<SENode*>;

  explicit SENode(ScalarEvolutionAnalysis* parent_analysis)
      : parent_analysis_(parent_analysis), unique_id_(++NumberOfNodes) {}

  virtual SENodeType GetType() const = 0;

  virtual ~SENode() {}

  virtual inline void AddChild(SENode* child) {

    if (AsSEConstantNode()) {
      assert(false && "Trying to add a child node to a constant!");
    }



    auto find_first_less_than = [child](const SENode* node) {
      return child->unique_id_ <= node->unique_id_;
    };

    auto position = std::find_if_not(children_.begin(), children_.end(),
                                     find_first_less_than);


    children_.insert(position, child);
  }



  std::string AsString() const;




  void DumpDot(std::ostream& out, bool recurse = false) const;


  bool operator==(const SENode& other) const;


  bool operator!=(const SENode& other) const;


  inline SENode* GetChild(size_t index) { return children_[index]; }
  inline const SENode* GetChild(size_t index) const { return children_[index]; }


  using iterator = ChildContainerType::iterator;
  using const_iterator = ChildContainerType::const_iterator;


  iterator begin() { return children_.begin(); }
  iterator end() { return children_.end(); }


  const_iterator begin() const { return children_.cbegin(); }
  const_iterator end() const { return children_.cend(); }
  const_iterator cbegin() { return children_.cbegin(); }
  const_iterator cend() { return children_.cend(); }


  std::vector<SERecurrentNode*> CollectRecurrentNodes() {
    std::vector<SERecurrentNode*> recurrent_nodes{};

    if (auto recurrent_node = AsSERecurrentNode()) {
      recurrent_nodes.push_back(recurrent_node);
    }

    for (auto child : GetChildren()) {
      auto child_recurrent_nodes = child->CollectRecurrentNodes();
      recurrent_nodes.insert(recurrent_nodes.end(),
                             child_recurrent_nodes.begin(),
                             child_recurrent_nodes.end());
    }

    return recurrent_nodes;
  }


  std::vector<SEValueUnknown*> CollectValueUnknownNodes() {
    std::vector<SEValueUnknown*> value_unknown_nodes{};

    if (auto value_unknown_node = AsSEValueUnknown()) {
      value_unknown_nodes.push_back(value_unknown_node);
    }

    for (auto child : GetChildren()) {
      auto child_value_unknown_nodes = child->CollectValueUnknownNodes();
      value_unknown_nodes.insert(value_unknown_nodes.end(),
                                 child_value_unknown_nodes.begin(),
                                 child_value_unknown_nodes.end());
    }

    return value_unknown_nodes;
  }




  using dag_iterator = TreeDFIterator<SENode>;
  using const_dag_iterator = TreeDFIterator<const SENode>;


  dag_iterator graph_begin() { return dag_iterator(this); }
  dag_iterator graph_end() { return dag_iterator(); }
  const_dag_iterator graph_begin() const { return graph_cbegin(); }
  const_dag_iterator graph_end() const { return graph_cend(); }
  const_dag_iterator graph_cbegin() const { return const_dag_iterator(this); }
  const_dag_iterator graph_cend() const { return const_dag_iterator(); }


  const ChildContainerType& GetChildren() const { return children_; }
  ChildContainerType& GetChildren() { return children_; }


  bool IsCantCompute() const { return GetType() == CanNotCompute; }



#define DeclareCastMethod(target)                  \
  virtual target* As##target() { return nullptr; } \
  virtual const target* As##target() const { return nullptr; }
  DeclareCastMethod(SEConstantNode)
  DeclareCastMethod(SERecurrentNode)
  DeclareCastMethod(SEAddNode)
  DeclareCastMethod(SEMultiplyNode)
  DeclareCastMethod(SENegative)
  DeclareCastMethod(SEValueUnknown)
  DeclareCastMethod(SECantCompute)
#undef DeclareCastMethod


  inline ScalarEvolutionAnalysis* GetParentAnalysis() const {
    return parent_analysis_;
  }

 protected:
  ChildContainerType children_;

  ScalarEvolutionAnalysis* parent_analysis_;



  uint32_t unique_id_;


  static uint32_t NumberOfNodes;
};





struct SENodeHash {
  size_t operator()(const std::unique_ptr<SENode>& node) const;
  size_t operator()(const SENode* node) const;
};


class SEConstantNode : public SENode {
 public:
  SEConstantNode(ScalarEvolutionAnalysis* parent_analysis, int64_t value)
      : SENode(parent_analysis), literal_value_(value) {}

  SENodeType GetType() const final { return Constant; }

  int64_t FoldToSingleValue() const { return literal_value_; }

  SEConstantNode* AsSEConstantNode() override { return this; }
  const SEConstantNode* AsSEConstantNode() const override { return this; }

  inline void AddChild(SENode*) final {
    assert(false && "Attempting to add a child to a constant node!");
  }

 protected:
  int64_t literal_value_;
};







class SERecurrentNode : public SENode {
 public:
  SERecurrentNode(ScalarEvolutionAnalysis* parent_analysis, const Loop* loop)
      : SENode(parent_analysis), loop_(loop) {}

  SENodeType GetType() const final { return RecurrentAddExpr; }

  inline void AddCoefficient(SENode* child) {
    coefficient_ = child;
    SENode::AddChild(child);
  }

  inline void AddOffset(SENode* child) {
    offset_ = child;
    SENode::AddChild(child);
  }

  inline const SENode* GetCoefficient() const { return coefficient_; }
  inline SENode* GetCoefficient() { return coefficient_; }

  inline const SENode* GetOffset() const { return offset_; }
  inline SENode* GetOffset() { return offset_; }


  const Loop* GetLoop() const { return loop_; }

  SERecurrentNode* AsSERecurrentNode() override { return this; }
  const SERecurrentNode* AsSERecurrentNode() const override { return this; }

 private:
  SENode* coefficient_;
  SENode* offset_;
  const Loop* loop_;
};


class SEAddNode : public SENode {
 public:
  explicit SEAddNode(ScalarEvolutionAnalysis* parent_analysis)
      : SENode(parent_analysis) {}

  SENodeType GetType() const final { return Add; }

  SEAddNode* AsSEAddNode() override { return this; }
  const SEAddNode* AsSEAddNode() const override { return this; }
};


class SEMultiplyNode : public SENode {
 public:
  explicit SEMultiplyNode(ScalarEvolutionAnalysis* parent_analysis)
      : SENode(parent_analysis) {}

  SENodeType GetType() const final { return Multiply; }

  SEMultiplyNode* AsSEMultiplyNode() override { return this; }
  const SEMultiplyNode* AsSEMultiplyNode() const override { return this; }
};


class SENegative : public SENode {
 public:
  explicit SENegative(ScalarEvolutionAnalysis* parent_analysis)
      : SENode(parent_analysis) {}

  SENodeType GetType() const final { return Negative; }

  SENegative* AsSENegative() override { return this; }
  const SENegative* AsSENegative() const override { return this; }
};



class SEValueUnknown : public SENode {
 public:



  SEValueUnknown(ScalarEvolutionAnalysis* parent_analysis, uint32_t result_id)
      : SENode(parent_analysis), result_id_(result_id) {}

  SENodeType GetType() const final { return ValueUnknown; }

  SEValueUnknown* AsSEValueUnknown() override { return this; }
  const SEValueUnknown* AsSEValueUnknown() const override { return this; }

  inline uint32_t ResultId() const { return result_id_; }

 private:
  uint32_t result_id_;
};


class SECantCompute : public SENode {
 public:
  explicit SECantCompute(ScalarEvolutionAnalysis* parent_analysis)
      : SENode(parent_analysis) {}

  SENodeType GetType() const final { return CanNotCompute; }

  SECantCompute* AsSECantCompute() override { return this; }
  const SECantCompute* AsSECantCompute() const override { return this; }
};

}
}
#endif

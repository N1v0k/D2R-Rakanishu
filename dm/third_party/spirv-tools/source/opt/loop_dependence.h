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

#ifndef SOURCE_OPT_LOOP_DEPENDENCE_H_
#define SOURCE_OPT_LOOP_DEPENDENCE_H_

#include <algorithm>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/scalar_analysis.h"

namespace spvtools {
namespace opt {



















class DistanceEntry {
 public:
  enum DependenceInformation {
    UNKNOWN = 0,
    DIRECTION = 1,
    DISTANCE = 2,
    PEEL = 3,
    IRRELEVANT = 4,
    POINT = 5
  };
  enum Directions {
    NONE = 0,
    LT = 1,
    EQ = 2,
    LE = 3,
    GT = 4,
    NE = 5,
    GE = 6,
    ALL = 7
  };
  DependenceInformation dependence_information;
  Directions direction;
  int64_t distance;
  bool peel_first;
  bool peel_last;
  int64_t point_x;
  int64_t point_y;

  DistanceEntry()
      : dependence_information(DependenceInformation::UNKNOWN),
        direction(Directions::ALL),
        distance(0),
        peel_first(false),
        peel_last(false),
        point_x(0),
        point_y(0) {}

  explicit DistanceEntry(Directions direction_)
      : dependence_information(DependenceInformation::DIRECTION),
        direction(direction_),
        distance(0),
        peel_first(false),
        peel_last(false),
        point_x(0),
        point_y(0) {}

  DistanceEntry(Directions direction_, int64_t distance_)
      : dependence_information(DependenceInformation::DISTANCE),
        direction(direction_),
        distance(distance_),
        peel_first(false),
        peel_last(false),
        point_x(0),
        point_y(0) {}

  DistanceEntry(int64_t x, int64_t y)
      : dependence_information(DependenceInformation::POINT),
        direction(Directions::ALL),
        distance(0),
        peel_first(false),
        peel_last(false),
        point_x(x),
        point_y(y) {}

  bool operator==(const DistanceEntry& rhs) const {
    return direction == rhs.direction && peel_first == rhs.peel_first &&
           peel_last == rhs.peel_last && distance == rhs.distance &&
           point_x == rhs.point_x && point_y == rhs.point_y;
  }

  bool operator!=(const DistanceEntry& rhs) const { return !(*this == rhs); }
};





class DistanceVector {
 public:
  explicit DistanceVector(size_t size) : entries(size, DistanceEntry{}) {}

  explicit DistanceVector(std::vector<DistanceEntry> entries_)
      : entries(entries_) {}

  DistanceEntry& GetEntry(size_t index) { return entries[index]; }
  const DistanceEntry& GetEntry(size_t index) const { return entries[index]; }

  std::vector<DistanceEntry>& GetEntries() { return entries; }
  const std::vector<DistanceEntry>& GetEntries() const { return entries; }

  bool operator==(const DistanceVector& rhs) const {
    if (entries.size() != rhs.entries.size()) {
      return false;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i] != rhs.entries[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const DistanceVector& rhs) const { return !(*this == rhs); }

 private:
  std::vector<DistanceEntry> entries;
};

class DependenceLine;
class DependenceDistance;
class DependencePoint;
class DependenceNone;
class DependenceEmpty;

class Constraint {
 public:
  explicit Constraint(const Loop* loop) : loop_(loop) {}
  enum ConstraintType { Line, Distance, Point, None, Empty };

  virtual ConstraintType GetType() const = 0;

  virtual ~Constraint() {}


  const Loop* GetLoop() const { return loop_; }

  bool operator==(const Constraint& other) const;

  bool operator!=(const Constraint& other) const;


#define DeclareCastMethod(target)                  \
  virtual target* As##target() { return nullptr; } \
  virtual const target* As##target() const { return nullptr; }
  DeclareCastMethod(DependenceLine)
  DeclareCastMethod(DependenceDistance)
  DeclareCastMethod(DependencePoint)
  DeclareCastMethod(DependenceNone)
  DeclareCastMethod(DependenceEmpty)
#undef DeclareCastMethod

 protected:
  const Loop* loop_;
};


class DependenceLine : public Constraint {
 public:
  DependenceLine(SENode* a, SENode* b, SENode* c, const Loop* loop)
      : Constraint(loop), a_(a), b_(b), c_(c) {}

  ConstraintType GetType() const final { return Line; }

  DependenceLine* AsDependenceLine() final { return this; }
  const DependenceLine* AsDependenceLine() const final { return this; }

  SENode* GetA() const { return a_; }
  SENode* GetB() const { return b_; }
  SENode* GetC() const { return c_; }

 private:
  SENode* a_;
  SENode* b_;
  SENode* c_;
};

class DependenceDistance : public Constraint {
 public:
  DependenceDistance(SENode* distance, const Loop* loop)
      : Constraint(loop), distance_(distance) {}

  ConstraintType GetType() const final { return Distance; }

  DependenceDistance* AsDependenceDistance() final { return this; }
  const DependenceDistance* AsDependenceDistance() const final { return this; }

  SENode* GetDistance() const { return distance_; }

 private:
  SENode* distance_;
};

class DependencePoint : public Constraint {
 public:
  DependencePoint(SENode* source, SENode* destination, const Loop* loop)
      : Constraint(loop), source_(source), destination_(destination) {}

  ConstraintType GetType() const final { return Point; }

  DependencePoint* AsDependencePoint() final { return this; }
  const DependencePoint* AsDependencePoint() const final { return this; }

  SENode* GetSource() const { return source_; }
  SENode* GetDestination() const { return destination_; }

 private:
  SENode* source_;
  SENode* destination_;
};

class DependenceNone : public Constraint {
 public:
  DependenceNone() : Constraint(nullptr) {}
  ConstraintType GetType() const final { return None; }

  DependenceNone* AsDependenceNone() final { return this; }
  const DependenceNone* AsDependenceNone() const final { return this; }
};

class DependenceEmpty : public Constraint {
 public:
  DependenceEmpty() : Constraint(nullptr) {}
  ConstraintType GetType() const final { return Empty; }

  DependenceEmpty* AsDependenceEmpty() final { return this; }
  const DependenceEmpty* AsDependenceEmpty() const final { return this; }
};


























class LoopDependenceAnalysis {
 public:
  LoopDependenceAnalysis(IRContext* context, std::vector<const Loop*> loops)
      : context_(context),
        loops_(loops),
        scalar_evolution_(context),
        debug_stream_(nullptr),
        constraints_{} {}







  bool GetDependence(const Instruction* source, const Instruction* destination,
                     DistanceVector* distance_vector);



  bool IsZIV(const std::pair<SENode*, SENode*>& subscript_pair);



  bool IsSIV(const std::pair<SENode*, SENode*>& subscript_pair);



  bool IsMIV(const std::pair<SENode*, SENode*>& subscript_pair);



  SENode* GetLowerBound(const Loop* loop);



  SENode* GetUpperBound(const Loop* loop);


  bool IsWithinBounds(int64_t value, int64_t bound_one, int64_t bound_two);




  SENode* GetTripCount(const Loop* loop);




  SENode* GetFirstTripInductionNode(const Loop* loop);




  SENode* GetFinalTripInductionNode(const Loop* loop,
                                    SENode* induction_coefficient);


  std::set<const Loop*> CollectLoops(
      const std::vector<SERecurrentNode*>& nodes);


  std::set<const Loop*> CollectLoops(SENode* source, SENode* destination);






  bool IsProvablyOutsideOfLoopBounds(const Loop* loop, SENode* distance,
                                     SENode* coefficient);


  void SetDebugStream(std::ostream& debug_stream) {
    debug_stream_ = &debug_stream;
  }


  void ClearDebugStream() { debug_stream_ = nullptr; }


  ScalarEvolutionAnalysis* GetScalarEvolution() { return &scalar_evolution_; }


  template <typename T, typename... Args>
  Constraint* make_constraint(Args&&... args) {
    constraints_.push_back(
        std::unique_ptr<Constraint>(new T(std::forward<Args>(args)...)));

    return constraints_.back().get();
  }







  using PartitionedSubscripts =
      std::vector<std::set<std::pair<Instruction*, Instruction*>>>;
  PartitionedSubscripts PartitionSubscripts(
      const std::vector<Instruction*>& source_subscripts,
      const std::vector<Instruction*>& destination_subscripts);



  const Loop* GetLoopForSubscriptPair(
      const std::pair<SENode*, SENode*>& subscript_pair);



  DistanceEntry* GetDistanceEntryForSubscriptPair(
      const std::pair<SENode*, SENode*>& subscript_pair,
      DistanceVector* distance_vector);


  DistanceEntry* GetDistanceEntryForLoop(const Loop* loop,
                                         DistanceVector* distance_vector);



  std::vector<Instruction*> GetSubscripts(const Instruction* instruction);



  bool DeltaTest(
      const std::vector<std::pair<SENode*, SENode*>>& coupled_subscripts,
      DistanceVector* dv_entry);



  std::pair<SENode*, SENode*> PropagateConstraints(
      const std::pair<SENode*, SENode*>& subscript_pair,
      const std::vector<Constraint*>& constraints);



  Constraint* IntersectConstraints(Constraint* constraint_0,
                                   Constraint* constraint_1,
                                   const SENode* lower_bound,
                                   const SENode* upper_bound);





  bool CheckSupportedLoops(std::vector<const Loop*> loops);




  bool IsSupportedLoop(const Loop* loop);

 private:
  IRContext* context_;


  std::vector<const Loop*> loops_;



  ScalarEvolutionAnalysis scalar_evolution_;


  std::ostream* debug_stream_;


  std::list<std::unique_ptr<Constraint>> constraints_;


  bool ZIVTest(const std::pair<SENode*, SENode*>& subscript_pair);



  bool SIVTest(const std::pair<SENode*, SENode*>& subscript_pair,
               DistanceVector* distance_vector);








  bool StrongSIVTest(SENode* source, SENode* destination, SENode* coeff,
                     DistanceEntry* distance_entry);




  bool SymbolicStrongSIVTest(SENode* source, SENode* destination,
                             SENode* coefficient,
                             DistanceEntry* distance_entry);





  bool WeakZeroSourceSIVTest(SENode* source, SERecurrentNode* destination,
                             SENode* coefficient,
                             DistanceEntry* distance_entry);





  bool WeakZeroDestinationSIVTest(SERecurrentNode* source, SENode* destination,
                                  SENode* coefficient,
                                  DistanceEntry* distance_entry);





  bool WeakCrossingSIVTest(SENode* source, SENode* destination,
                           SENode* coefficient, DistanceEntry* distance_entry);



  Instruction* GetOperandDefinition(const Instruction* instruction, int id);



  bool GCDMIVTest(const std::pair<SENode*, SENode*>& subscript_pair);



  int64_t CountInductionVariables(SENode* node);




  int64_t CountInductionVariables(SENode* source, SENode* destination);




  SENode* GetConstantTerm(const Loop* loop, SERecurrentNode* induction);




  void MarkUnsusedDistanceEntriesAsIrrelevant(const Instruction* source,
                                              const Instruction* destination,
                                              DistanceVector* distance_vector);



  template <typename valueT>
  std::string ToString(valueT value) {
    std::ostringstream string_stream;
    string_stream << value;
    return string_stream.str();
  }



  void PrintDebug(std::string debug_msg);
};

}
}

#endif

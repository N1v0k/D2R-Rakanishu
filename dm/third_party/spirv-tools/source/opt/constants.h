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

#ifndef SOURCE_OPT_CONSTANTS_H_
#define SOURCE_OPT_CONSTANTS_H_

#include <cinttypes>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/module.h"
#include "source/opt/type_manager.h"
#include "source/opt/types.h"
#include "source/util/hex_float.h"
#include "source/util/make_unique.h"

namespace spvtools {
namespace opt {

class IRContext;

namespace analysis {





class Constant;
class ScalarConstant;
class IntConstant;
class FloatConstant;
class BoolConstant;
class CompositeConstant;
class StructConstant;
class VectorConstant;
class MatrixConstant;
class ArrayConstant;
class TensorConstant;
class NullConstant;
class ConstantManager;



class Constant {
 public:
  Constant() = delete;
  virtual ~Constant() = default;


  virtual std::unique_ptr<Constant> Copy() const = 0;


  virtual ScalarConstant* AsScalarConstant() { return nullptr; }
  virtual IntConstant* AsIntConstant() { return nullptr; }
  virtual FloatConstant* AsFloatConstant() { return nullptr; }
  virtual BoolConstant* AsBoolConstant() { return nullptr; }
  virtual CompositeConstant* AsCompositeConstant() { return nullptr; }
  virtual StructConstant* AsStructConstant() { return nullptr; }
  virtual VectorConstant* AsVectorConstant() { return nullptr; }
  virtual MatrixConstant* AsMatrixConstant() { return nullptr; }
  virtual ArrayConstant* AsArrayConstant() { return nullptr; }
  virtual TensorConstant* AsTensorConstant() { return nullptr; }
  virtual NullConstant* AsNullConstant() { return nullptr; }

  virtual const ScalarConstant* AsScalarConstant() const { return nullptr; }
  virtual const IntConstant* AsIntConstant() const { return nullptr; }
  virtual const FloatConstant* AsFloatConstant() const { return nullptr; }
  virtual const BoolConstant* AsBoolConstant() const { return nullptr; }
  virtual const CompositeConstant* AsCompositeConstant() const {
    return nullptr;
  }
  virtual const StructConstant* AsStructConstant() const { return nullptr; }
  virtual const VectorConstant* AsVectorConstant() const { return nullptr; }
  virtual const MatrixConstant* AsMatrixConstant() const { return nullptr; }
  virtual const ArrayConstant* AsArrayConstant() const { return nullptr; }
  virtual const TensorConstant* AsTensorConstant() const { return nullptr; }
  virtual const NullConstant* AsNullConstant() const { return nullptr; }



  float GetFloat() const;



  double GetDouble() const;



  double GetValueAsDouble() const;



  uint32_t GetU32() const;



  uint64_t GetU64() const;



  int32_t GetS32() const;



  int64_t GetS64() const;



  uint64_t GetZeroExtendedValue() const;



  int64_t GetSignExtendedValue() const;


  virtual bool IsZero() const { return false; }

  const Type* type() const { return type_; }



  std::vector<const Constant*> GetVectorComponents(
      ConstantManager* const_mgr) const;

 protected:
  Constant(const Type* ty) : type_(ty) {}


  const Type* type_;
};


class ScalarConstant : public Constant {
 public:
  ScalarConstant() = delete;
  ScalarConstant* AsScalarConstant() override { return this; }
  const ScalarConstant* AsScalarConstant() const override { return this; }


  virtual const std::vector<uint32_t>& words() const { return words_; }


  bool IsZero() const override {
    bool is_zero = true;
    for (uint32_t v : words()) {
      if (v != 0) {
        is_zero = false;
        break;
      }
    }
    return is_zero;
  }

  uint32_t GetU32BitValue() const {


    assert(words().size() == 1);
    return words()[0];
  }

  uint64_t GetU64BitValue() const {


    assert(words().size() == 2);
    return static_cast<uint64_t>(words()[1]) << 32 |
           static_cast<uint64_t>(words()[0]);
  }

 protected:
  ScalarConstant(const Type* ty, const std::vector<uint32_t>& w)
      : Constant(ty), words_(w) {}
  ScalarConstant(const Type* ty, std::vector<uint32_t>&& w)
      : Constant(ty), words_(std::move(w)) {}
  std::vector<uint32_t> words_;
};


class IntConstant : public ScalarConstant {
 public:
  IntConstant(const Integer* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  IntConstant(const Integer* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  IntConstant* AsIntConstant() override { return this; }
  const IntConstant* AsIntConstant() const override { return this; }

  int32_t GetS32BitValue() const {


    assert(words().size() == 1);
    return words()[0];
  }

  int64_t GetS64BitValue() const {


    assert(words().size() == 2);
    return static_cast<uint64_t>(words()[1]) << 32 |
           static_cast<uint64_t>(words()[0]);
  }


  std::unique_ptr<IntConstant> CopyIntConstant() const {
    return MakeUnique<IntConstant>(type_->AsInteger(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyIntConstant().release());
  }
};


class FloatConstant : public ScalarConstant {
 public:
  FloatConstant(const Float* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  FloatConstant(const Float* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  FloatConstant* AsFloatConstant() override { return this; }
  const FloatConstant* AsFloatConstant() const override { return this; }


  std::unique_ptr<FloatConstant> CopyFloatConstant() const {
    return MakeUnique<FloatConstant>(type_->AsFloat(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyFloatConstant().release());
  }



  float GetFloatValue() const {
    assert(type()->AsFloat()->width() == 32 &&
           "Not a 32-bit floating point value.");
    utils::FloatProxy<float> a(words()[0]);
    return a.getAsFloat();
  }



  double GetDoubleValue() const {
    assert(type()->AsFloat()->width() == 64 &&
           "Not a 32-bit floating point value.");
    uint64_t combined_words = words()[1];
    combined_words = combined_words << 32;
    combined_words |= words()[0];
    utils::FloatProxy<double> a(combined_words);
    return a.getAsFloat();
  }
};


class BoolConstant : public ScalarConstant {
 public:
  BoolConstant(const Bool* ty, bool v)
      : ScalarConstant(ty, {static_cast<uint32_t>(v)}), value_(v) {}

  BoolConstant* AsBoolConstant() override { return this; }
  const BoolConstant* AsBoolConstant() const override { return this; }


  std::unique_ptr<BoolConstant> CopyBoolConstant() const {
    return MakeUnique<BoolConstant>(type_->AsBool(), value_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyBoolConstant().release());
  }

  bool value() const { return value_; }

 private:
  bool value_;
};


class CompositeConstant : public Constant {
 public:
  CompositeConstant() = delete;
  CompositeConstant* AsCompositeConstant() override { return this; }
  const CompositeConstant* AsCompositeConstant() const override { return this; }



  virtual const std::vector<const Constant*>& GetComponents() const {
    return components_;
  }

  bool IsZero() const override {
    for (const Constant* c : GetComponents()) {
      if (!c->IsZero()) {
        return false;
      }
    }
    return true;
  }

 protected:
  CompositeConstant(const Type* ty) : Constant(ty), components_() {}
  CompositeConstant(const Type* ty,
                    const std::vector<const Constant*>& components)
      : Constant(ty), components_(components) {}
  CompositeConstant(const Type* ty, std::vector<const Constant*>&& components)
      : Constant(ty), components_(std::move(components)) {}
  std::vector<const Constant*> components_;
};


class StructConstant : public CompositeConstant {
 public:
  StructConstant(const Struct* ty) : CompositeConstant(ty) {}
  StructConstant(const Struct* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  StructConstant(const Struct* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  StructConstant* AsStructConstant() override { return this; }
  const StructConstant* AsStructConstant() const override { return this; }


  std::unique_ptr<StructConstant> CopyStructConstant() const {
    return MakeUnique<StructConstant>(type_->AsStruct(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyStructConstant().release());
  }
};


class VectorConstant : public CompositeConstant {
 public:
  VectorConstant(const Vector* ty)
      : CompositeConstant(ty), component_type_(ty->element_type()) {}
  VectorConstant(const Vector* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components),
        component_type_(ty->element_type()) {}
  VectorConstant(const Vector* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)),
        component_type_(ty->element_type()) {}

  VectorConstant* AsVectorConstant() override { return this; }
  const VectorConstant* AsVectorConstant() const override { return this; }


  std::unique_ptr<VectorConstant> CopyVectorConstant() const {
    auto another = MakeUnique<VectorConstant>(type_->AsVector());
    another->components_.insert(another->components_.end(), components_.begin(),
                                components_.end());
    return another;
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyVectorConstant().release());
  }

  const Type* component_type() const { return component_type_; }

 private:
  const Type* component_type_;
};


class MatrixConstant : public CompositeConstant {
 public:
  MatrixConstant(const Matrix* ty)
      : CompositeConstant(ty), component_type_(ty->element_type()) {}
  MatrixConstant(const Matrix* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components),
        component_type_(ty->element_type()) {}
  MatrixConstant(const Vector* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)),
        component_type_(ty->element_type()) {}

  MatrixConstant* AsMatrixConstant() override { return this; }
  const MatrixConstant* AsMatrixConstant() const override { return this; }


  std::unique_ptr<MatrixConstant> CopyMatrixConstant() const {
    auto another = MakeUnique<MatrixConstant>(type_->AsMatrix());
    another->components_.insert(another->components_.end(), components_.begin(),
                                components_.end());
    return another;
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyMatrixConstant().release());
  }

  const Type* component_type() { return component_type_; }

 private:
  const Type* component_type_;
};


class ArrayConstant : public CompositeConstant {
 public:
  ArrayConstant(const Array* ty) : CompositeConstant(ty) {}
  ArrayConstant(const Array* ty, const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  ArrayConstant(const Array* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  ArrayConstant* AsArrayConstant() override { return this; }
  const ArrayConstant* AsArrayConstant() const override { return this; }


  std::unique_ptr<ArrayConstant> CopyArrayConstant() const {
    return MakeUnique<ArrayConstant>(type_->AsArray(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyArrayConstant().release());
  }
};


class TensorConstant : public CompositeConstant {
 public:
  TensorConstant(const TensorARM* ty) : CompositeConstant(ty) {}
  TensorConstant(const TensorARM* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  TensorConstant(const TensorARM* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  TensorConstant* AsTensorConstant() override { return this; }
  const TensorConstant* AsTensorConstant() const override { return this; }


  std::unique_ptr<TensorConstant> CopyTensorConstant() const {
    return MakeUnique<TensorConstant>(type_->AsTensorARM(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyTensorConstant().release());
  }
};


class NullConstant : public Constant {
 public:
  NullConstant(const Type* ty) : Constant(ty) {}
  NullConstant* AsNullConstant() override { return this; }
  const NullConstant* AsNullConstant() const override { return this; }


  std::unique_ptr<NullConstant> CopyNullConstant() const {
    return MakeUnique<NullConstant>(type_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyNullConstant().release());
  }
  bool IsZero() const override { return true; }
};



struct ConstantHash {
  void add_pointer(std::u32string* h, const void* p) const {
    uint64_t ptr_val = reinterpret_cast<uint64_t>(p);
    h->push_back(static_cast<uint32_t>(ptr_val >> 32));
    h->push_back(static_cast<uint32_t>(ptr_val));
  }

  size_t operator()(const Constant* const_val) const {
    std::u32string h;
    add_pointer(&h, const_val->type());
    if (const auto scalar = const_val->AsScalarConstant()) {
      for (const auto& w : scalar->words()) {
        h.push_back(w);
      }
    } else if (const auto composite = const_val->AsCompositeConstant()) {
      for (const auto& c : composite->GetComponents()) {
        add_pointer(&h, c);
      }
    } else if (const_val->AsNullConstant()) {
      h.push_back(0);
    } else {
      assert(
          false &&
          "Tried to compute the hash value of an invalid Constant instance.");
    }

    return std::hash<std::u32string>()(h);
  }
};


struct ConstantEqual {
  bool operator()(const Constant* c1, const Constant* c2) const {
    if (c1->type() != c2->type()) {
      return false;
    }

    if (const auto& s1 = c1->AsScalarConstant()) {
      const auto& s2 = c2->AsScalarConstant();
      return s2 && s1->words() == s2->words();
    } else if (const auto& composite1 = c1->AsCompositeConstant()) {
      const auto& composite2 = c2->AsCompositeConstant();
      return composite2 &&
             composite1->GetComponents() == composite2->GetComponents();
    } else if (c1->AsNullConstant()) {
      return c2->AsNullConstant() != nullptr;
    } else {
      assert(false && "Tried to compare two invalid Constant instances.");
    }
    return false;
  }
};


class ConstantManager {
 public:
  ConstantManager(IRContext* ctx);

  IRContext* context() const { return ctx_; }







  const Constant* GetConstant(
      const Type* type, const std::vector<uint32_t>& literal_words_or_ids);

  template <class C>
  const Constant* GetConstant(const Type* type, const C& literal_words_or_ids) {
    return GetConstant(type, std::vector<uint32_t>(literal_words_or_ids.begin(),
                                                   literal_words_or_ids.end()));
  }







  const Constant* GetNullCompositeConstant(const Type* type);






  const Constant* GetNumericVectorConstantWithWords(
      const Vector* type, const std::vector<uint32_t>& literal_words);




  const Constant* GetConstantFromInst(const Instruction* inst);










  Instruction* GetDefiningInstruction(const Constant* c, uint32_t type_id = 0,
                                      Module::inst_iterator* pos = nullptr);













  Instruction* BuildInstructionAndAddToModule(const Constant* c,
                                              Module::inst_iterator* pos,
                                              uint32_t type_id = 0);



  Type* GetType(const Instruction* inst) const;




  const Constant* FindDeclaredConstant(uint32_t id) const {
    auto iter = id_to_const_val_.find(id);
    return (iter != id_to_const_val_.end()) ? iter->second : nullptr;
  }



  uint32_t FindDeclaredConstant(const Constant* c, uint32_t type_id) const;






  const Constant* FindConstant(const Constant* c) const {
    auto it = const_pool_.find(c);
    return (it != const_pool_.end()) ? *it : nullptr;
  }




  const Constant* RegisterConstant(std::unique_ptr<Constant> cst) {
    auto ret = const_pool_.insert(cst.get());
    if (ret.second) {
      owned_constants_.emplace_back(std::move(cst));
    }
    return *ret.first;
  }




  std::vector<const Constant*> GetConstantsFromIds(
      const std::vector<uint32_t>& ids) const;



  std::vector<const Constant*> GetOperandConstants(
      const Instruction* inst) const;




  bool MapInst(Instruction* inst) {
    if (auto cst = GetConstantFromInst(inst)) {
      MapConstantToInst(cst, inst);
      return true;
    }
    return false;
  }

  void RemoveId(uint32_t id) {
    auto it = id_to_const_val_.find(id);
    if (it != id_to_const_val_.end()) {
      const_val_to_id_.erase(it->second);
      id_to_const_val_.erase(it);
    }
  }



  void MapConstantToInst(const Constant* const_value, Instruction* inst) {
    if (id_to_const_val_.insert({inst->result_id(), const_value}).second) {
      const_val_to_id_.insert({const_value, inst->result_id()});
    }
  }


  uint32_t GetFloatConstId(float val);


  const Constant* GetFloatConst(float val);


  uint32_t GetDoubleConstId(double val);


  const Constant* GetDoubleConst(double val);


  uint32_t GetSIntConstId(int32_t val);





  const Constant* GetIntConst(uint64_t val, int32_t bitWidth, bool isSigned);


  uint32_t GetUIntConstId(uint32_t val);


  uint32_t GetNullConstId(const Type* type);



  const Constant* GenerateIntegerConstant(const analysis::Integer* integer_type,
                                          uint64_t result);

 private:












  std::unique_ptr<Constant> CreateConstant(
      const Type* type,
      const std::vector<uint32_t>& literal_words_or_ids) const;










  std::unique_ptr<Instruction> CreateInstruction(uint32_t result_id,
                                                 const Constant* c,
                                                 uint32_t type_id = 0) const;










  std::unique_ptr<Instruction> CreateCompositeInstruction(
      uint32_t result_id, const CompositeConstant* cc,
      uint32_t type_id = 0) const;


  IRContext* ctx_;





  std::unordered_map<uint32_t, const Constant*> id_to_const_val_;





  std::multimap<const Constant*, uint32_t> const_val_to_id_;


  std::unordered_set<const Constant*, ConstantHash, ConstantEqual> const_pool_;



  std::vector<std::unique_ptr<Constant>> owned_constants_;
};

}
}
}

#endif

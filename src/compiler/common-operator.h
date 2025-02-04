// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_COMMON_OPERATOR_H_
#define V8_COMPILER_COMMON_OPERATOR_H_

#include "src/assembler.h"
#include "src/compiler/linkage.h"
#include "src/compiler/machine-type.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/operator.h"
#include "src/unique.h"

namespace v8 {
namespace internal {

class OStream;

namespace compiler {

class ControlOperator FINAL : public Operator1<int> {
 public:
  ControlOperator(IrOpcode::Value opcode, Properties properties, int inputs,
                  int outputs, int controls, const char* mnemonic)
      : Operator1<int>(opcode, properties, inputs, outputs, mnemonic,
                       controls) {}

  virtual OStream& PrintParameter(OStream& os) const OVERRIDE {  // NOLINT
    return os;
  }
  int ControlInputCount() const { return parameter(); }
};

class CallOperator FINAL : public Operator1<CallDescriptor*> {
 public:
  // TODO(titzer): Operator still uses int, whereas CallDescriptor uses size_t.
  CallOperator(CallDescriptor* descriptor, const char* mnemonic)
      : Operator1<CallDescriptor*>(
            IrOpcode::kCall, descriptor->properties(),
            static_cast<int>(descriptor->InputCount() +
                             descriptor->FrameStateCount()),
            static_cast<int>(descriptor->ReturnCount()), mnemonic, descriptor) {
  }

  virtual OStream& PrintParameter(OStream& os) const OVERRIDE {  // NOLINT
    return os << "[" << *parameter() << "]";
  }
};

// Flag that describes how to combine the current environment with
// the output of a node to obtain a framestate for lazy bailout.
enum OutputFrameStateCombine {
  kPushOutput,   // Push the output on the expression stack.
  kIgnoreOutput  // Use the frame state as-is.
};


class FrameStateCallInfo {
 public:
  FrameStateCallInfo(BailoutId bailout_id,
                     OutputFrameStateCombine state_combine)
      : bailout_id_(bailout_id), frame_state_combine_(state_combine) {}

  BailoutId bailout_id() const { return bailout_id_; }
  OutputFrameStateCombine state_combine() const { return frame_state_combine_; }

 private:
  BailoutId bailout_id_;
  OutputFrameStateCombine frame_state_combine_;
};

// Interface for building common operators that can be used at any level of IR,
// including JavaScript, mid-level, and low-level.
// TODO(titzer): Move the mnemonics into SimpleOperator and Operator1 classes.
class CommonOperatorBuilder {
 public:
  explicit CommonOperatorBuilder(Zone* zone) : zone_(zone) {}

#define CONTROL_OP(name, inputs, controls)                                   \
  return new (zone_) ControlOperator(IrOpcode::k##name, Operator::kFoldable, \
                                     inputs, 0, controls, #name);

  Operator* Start(int num_formal_parameters) {
    // Outputs are formal parameters, plus context, receiver, and JSFunction.
    int outputs = num_formal_parameters + 3;
    return new (zone_) ControlOperator(IrOpcode::kStart, Operator::kFoldable, 0,
                                       outputs, 0, "Start");
  }
  Operator* Dead() { CONTROL_OP(Dead, 0, 0); }
  Operator* End() { CONTROL_OP(End, 0, 1); }
  Operator* Branch() { CONTROL_OP(Branch, 1, 1); }
  Operator* IfTrue() { CONTROL_OP(IfTrue, 0, 1); }
  Operator* IfFalse() { CONTROL_OP(IfFalse, 0, 1); }
  Operator* Throw() { CONTROL_OP(Throw, 1, 1); }

  Operator* Return() {
    return new (zone_) ControlOperator(
        IrOpcode::kReturn, Operator::kNoProperties, 1, 0, 1, "Return");
  }

  Operator* Merge(int controls) {
    return new (zone_) ControlOperator(IrOpcode::kMerge, Operator::kFoldable, 0,
                                       0, controls, "Merge");
  }

  Operator* Loop(int controls) {
    return new (zone_) ControlOperator(IrOpcode::kLoop, Operator::kFoldable, 0,
                                       0, controls, "Loop");
  }

  Operator* Parameter(int index) {
    return new (zone_) Operator1<int>(IrOpcode::kParameter, Operator::kPure, 1,
                                      1, "Parameter", index);
  }
  Operator* Int32Constant(int32_t value) {
    return new (zone_)
        Operator1<int32_t>(IrOpcode::kInt32Constant, Operator::kPure, 0, 1,
                           "Int32Constant", value);
  }
  Operator* Int64Constant(int64_t value) {
    return new (zone_)
        Operator1<int64_t>(IrOpcode::kInt64Constant, Operator::kPure, 0, 1,
                           "Int64Constant", value);
  }
  Operator* Float64Constant(double value) {
    return new (zone_)
        Operator1<double>(IrOpcode::kFloat64Constant, Operator::kPure, 0, 1,
                          "Float64Constant", value);
  }
  Operator* ExternalConstant(ExternalReference value) {
    return new (zone_) Operator1<ExternalReference>(IrOpcode::kExternalConstant,
                                                    Operator::kPure, 0, 1,
                                                    "ExternalConstant", value);
  }
  Operator* NumberConstant(double value) {
    return new (zone_)
        Operator1<double>(IrOpcode::kNumberConstant, Operator::kPure, 0, 1,
                          "NumberConstant", value);
  }
  Operator* HeapConstant(Unique<Object> value) {
    return new (zone_) Operator1<Unique<Object> >(
        IrOpcode::kHeapConstant, Operator::kPure, 0, 1, "HeapConstant", value);
  }
  Operator* Phi(MachineType type, int arguments) {
    DCHECK(arguments > 0);  // Disallow empty phis.
    return new (zone_) Operator1<MachineType>(IrOpcode::kPhi, Operator::kPure,
                                              arguments, 1, "Phi", type);
  }
  Operator* EffectPhi(int arguments) {
    DCHECK(arguments > 0);  // Disallow empty phis.
    return new (zone_) Operator1<int>(IrOpcode::kEffectPhi, Operator::kPure, 0,
                                      0, "EffectPhi", arguments);
  }
  Operator* ControlEffect() {
    return new (zone_) SimpleOperator(IrOpcode::kControlEffect, Operator::kPure,
                                      0, 0, "ControlEffect");
  }
  Operator* ValueEffect(int arguments) {
    DCHECK(arguments > 0);  // Disallow empty value effects.
    return new (zone_) SimpleOperator(IrOpcode::kValueEffect, Operator::kPure,
                                      arguments, 0, "ValueEffect");
  }
  Operator* Finish(int arguments) {
    DCHECK(arguments > 0);  // Disallow empty finishes.
    return new (zone_) Operator1<int>(IrOpcode::kFinish, Operator::kPure, 1, 1,
                                      "Finish", arguments);
  }
  Operator* StateValues(int arguments) {
    return new (zone_) Operator1<int>(IrOpcode::kStateValues, Operator::kPure,
                                      arguments, 1, "StateValues", arguments);
  }
  Operator* FrameState(BailoutId bailout_id, OutputFrameStateCombine combine) {
    return new (zone_) Operator1<FrameStateCallInfo>(
        IrOpcode::kFrameState, Operator::kPure, 4, 1, "FrameState",
        FrameStateCallInfo(bailout_id, combine));
  }
  Operator* Call(CallDescriptor* descriptor) {
    return new (zone_) CallOperator(descriptor, "Call");
  }
  Operator* Projection(size_t index) {
    return new (zone_) Operator1<size_t>(IrOpcode::kProjection, Operator::kPure,
                                         1, 1, "Projection", index);
  }

 private:
  Zone* zone_;
};


template <typename T>
struct CommonOperatorTraits {
  static inline bool Equals(T a, T b);
  static inline bool HasValue(const Operator* op);
  static inline T ValueOf(const Operator* op);
};

template <>
struct CommonOperatorTraits<int32_t> {
  static inline bool Equals(int32_t a, int32_t b) { return a == b; }
  static inline bool HasValue(const Operator* op) {
    return op->opcode() == IrOpcode::kInt32Constant ||
           op->opcode() == IrOpcode::kNumberConstant;
  }
  static inline int32_t ValueOf(const Operator* op) {
    if (op->opcode() == IrOpcode::kNumberConstant) {
      // TODO(titzer): cache the converted int32 value in NumberConstant.
      return FastD2I(OpParameter<double>(op));
    }
    CHECK_EQ(IrOpcode::kInt32Constant, op->opcode());
    return OpParameter<int32_t>(op);
  }
};

template <>
struct CommonOperatorTraits<uint32_t> {
  static inline bool Equals(uint32_t a, uint32_t b) { return a == b; }
  static inline bool HasValue(const Operator* op) {
    return CommonOperatorTraits<int32_t>::HasValue(op);
  }
  static inline uint32_t ValueOf(const Operator* op) {
    if (op->opcode() == IrOpcode::kNumberConstant) {
      // TODO(titzer): cache the converted uint32 value in NumberConstant.
      return FastD2UI(OpParameter<double>(op));
    }
    return static_cast<uint32_t>(CommonOperatorTraits<int32_t>::ValueOf(op));
  }
};

template <>
struct CommonOperatorTraits<int64_t> {
  static inline bool Equals(int64_t a, int64_t b) { return a == b; }
  static inline bool HasValue(const Operator* op) {
    return op->opcode() == IrOpcode::kInt32Constant ||
           op->opcode() == IrOpcode::kInt64Constant ||
           op->opcode() == IrOpcode::kNumberConstant;
  }
  static inline int64_t ValueOf(const Operator* op) {
    if (op->opcode() == IrOpcode::kInt32Constant) {
      return static_cast<int64_t>(CommonOperatorTraits<int32_t>::ValueOf(op));
    }
    CHECK_EQ(IrOpcode::kInt64Constant, op->opcode());
    return OpParameter<int64_t>(op);
  }
};

template <>
struct CommonOperatorTraits<uint64_t> {
  static inline bool Equals(uint64_t a, uint64_t b) { return a == b; }
  static inline bool HasValue(const Operator* op) {
    return CommonOperatorTraits<int64_t>::HasValue(op);
  }
  static inline uint64_t ValueOf(const Operator* op) {
    return static_cast<uint64_t>(CommonOperatorTraits<int64_t>::ValueOf(op));
  }
};

template <>
struct CommonOperatorTraits<double> {
  static inline bool Equals(double a, double b) {
    return DoubleRepresentation(a).bits == DoubleRepresentation(b).bits;
  }
  static inline bool HasValue(const Operator* op) {
    return op->opcode() == IrOpcode::kFloat64Constant ||
           op->opcode() == IrOpcode::kInt32Constant ||
           op->opcode() == IrOpcode::kNumberConstant;
  }
  static inline double ValueOf(const Operator* op) {
    if (op->opcode() == IrOpcode::kFloat64Constant ||
        op->opcode() == IrOpcode::kNumberConstant) {
      return OpParameter<double>(op);
    }
    return static_cast<double>(CommonOperatorTraits<int32_t>::ValueOf(op));
  }
};

template <>
struct CommonOperatorTraits<ExternalReference> {
  static inline bool Equals(ExternalReference a, ExternalReference b) {
    return a == b;
  }
  static inline bool HasValue(const Operator* op) {
    return op->opcode() == IrOpcode::kExternalConstant;
  }
  static inline ExternalReference ValueOf(const Operator* op) {
    CHECK_EQ(IrOpcode::kExternalConstant, op->opcode());
    return OpParameter<ExternalReference>(op);
  }
};

template <typename T>
struct CommonOperatorTraits<Unique<T> > {
  static inline bool HasValue(const Operator* op) {
    return op->opcode() == IrOpcode::kHeapConstant;
  }
  static inline Unique<T> ValueOf(const Operator* op) {
    CHECK_EQ(IrOpcode::kHeapConstant, op->opcode());
    return OpParameter<Unique<T> >(op);
  }
};

template <typename T>
struct CommonOperatorTraits<Handle<T> > {
  static inline bool HasValue(const Operator* op) {
    return CommonOperatorTraits<Unique<T> >::HasValue(op);
  }
  static inline Handle<T> ValueOf(const Operator* op) {
    return CommonOperatorTraits<Unique<T> >::ValueOf(op).handle();
  }
};


template <typename T>
inline T ValueOf(const Operator* op) {
  return CommonOperatorTraits<T>::ValueOf(op);
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_COMMON_OPERATOR_H_

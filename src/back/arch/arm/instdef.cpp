#include "instdef.h"

namespace lava::back {
LLOperand::LLOperand(State state, int n)
  : _state(state), _virtual_num(-1), _imm_num(-1), _allocated(nullptr) {
  switch (state) {
    case State::Immediate: _imm_num = n;                  break;
    case State::Virtual:   _virtual_num = n;              break;
    case State::RealReg:   _reg = static_cast<ArmReg>(n); break;
    default: break;
  }
}

void LLOperand::ReplaceWith(const LLOperandPtr &V) {
  _state = V->_state;
  switch (_state) {
    case State::RealReg:    _reg = V->_reg;                 break;
    case State::Virtual:    _virtual_num = V->_virtual_num; break;
    case State::Immediate:  _imm_num = V->_imm_num;         break;
    default: break;
  }

  _allocated = V->_allocated;
}

ArmShift::operator std::string() const {
  const char *name;
  switch (_type) {
    case ShiftType::Asr:
      name = "asr";
      break;
    case ShiftType::Lsl:
      name = "lsl";
      break;
    case ShiftType::Lsr:
      name = "lsr";
      break;
    case ShiftType::Ror:
      name = "ror";
      break;
    case ShiftType::Rrx:
      name = "rrx";
      break;
    default:
      name = "";
      ERROR("should not reach here");
  }
  return std::string(name) + " #" + std::to_string(_shift);
}

void LLBinaryInst::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _lhs = opr; break;
    case 1: _rhs = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLMove::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _src = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLBranch::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _cond = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLLoad::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _addr   = opr; break;
    case 1: _offset = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLStore::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _data   = opr; break;
    case 1: _addr   = opr; break;
    case 2: _offset = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLCompare::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _lhs = opr; break;
    case 1: _rhs = opr; break;
    default: ERROR("should not reach here");
  }
}

void LLFMA::set_operand(const LLOperandPtr &opr, std::size_t index) {
  switch (index) {
    case 0: _lhs = opr; break;
    case 1: _rhs = opr; break;
    case 2: _acc = opr; break;
    default: ERROR("should not reach here");
  }
}

LLOperandList LLBinaryInst::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_lhs);
  ops.push_back(_rhs);
  return ops;
}

LLOperandList LLMove::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_src);
  return ops;
}

LLOperandList LLLoad::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_addr);
  ops.push_back(_offset);
  return ops;
}

LLOperandList LLStore::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_data);
  ops.push_back(_addr);
  ops.push_back(_offset);
  return ops;
}

LLOperandList LLCompare::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_lhs);
  ops.push_back(_rhs);
  return ops;
}

LLOperandList LLFMA::operands() {
  std::vector<LLOperandPtr> ops;
  ops.push_back(_lhs);
  ops.push_back(_rhs);
  ops.push_back(_acc);
  return ops;
}

}
#include "instdef.h"

namespace lava::back {

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
      ERROR("should not reach here");
  }
  return std::string(name) + " #" + std::to_string(_shift);
}

std::ostream &operator<<(std::ostream &os, const ArmShift &shift) {
  os << std::string(shift);
  return os;
}

std::ostream &operator<<(std::ostream &os, const ArmCond &cond) {
  if (cond == ArmCond::Eq) {
    os << "eq";
  } else if (cond == ArmCond::Ne) {
    os << "ne";
  } else if (cond == ArmCond::Any) {
    os << "";
  } else if (cond == ArmCond::Gt) {
    os << "gt";
  } else if (cond == ArmCond::Ge) {
    os << "ge";
  } else if (cond == ArmCond::Lt) {
    os << "lt";
  } else if (cond == ArmCond::Le) {
    os << "le";
  } else {
    ERROR("should not reach here");
  }
  return os;
}

}
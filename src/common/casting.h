#ifndef XY_LANG_CASTSSA_H
#define XY_LANG_CASTSSA_H


namespace lava {
template <typename SSA, typename TYPE>
std::shared_ptr<SSA> dyn_cast(const TYPE &ssa) {
  if (ssa && SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename SSA, typename TYPE>
std::shared_ptr<SSA> dyn_cast(TYPE &ssa) {
  if (ssa && SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename SSA, typename TYPE>
bool IsSSA(const TYPE &ssa) {
  DBG_ASSERT(ssa != nullptr, "SSA is nullptr");
  if (SSA::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

template <typename SSA, typename TYPE>
bool IsSSA(TYPE &ssa) {
  DBG_ASSERT(ssa != nullptr, "SSA is nullptr");
  if (SSA::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

}

#endif //XY_LANG_CASTSSA_H

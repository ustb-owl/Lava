#ifndef XY_LANG_CASTSSA_H
#define XY_LANG_CASTSSA_H


namespace lava {
template <typename CLS, typename TYPE>
std::shared_ptr<CLS> dyn_cast(const TYPE &ssa) {
  if (ssa && CLS::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<CLS>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast CLS failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename CLS, typename TYPE>
std::shared_ptr<CLS> dyn_cast(TYPE &ssa) {
  if (ssa && CLS::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<CLS>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast CLS failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename CLS, typename TYPE>
const CLS *dyn_cast(const TYPE *ssa) {
  if (ssa && CLS::classof(ssa)) {
    const CLS *ptr = static_cast<const CLS *>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast CLS failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename CLS, typename TYPE>
CLS *dyn_cast(TYPE *ssa) {
  if (ssa && CLS::classof(ssa)) {
    CLS *ptr = static_cast<CLS *>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast CLS failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename CLS, typename TYPE>
bool IsSSA(const TYPE &ssa) {
  DBG_ASSERT(ssa != nullptr, "CLS is nullptr");
  if (CLS::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

template <typename CLS, typename TYPE>
bool IsSSA(TYPE &ssa) {
  DBG_ASSERT(ssa != nullptr, "CLS is nullptr");
  if (CLS::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

template <typename CLS, typename TYPE>
bool IsSSA(TYPE *ssa) {
  DBG_ASSERT(ssa != nullptr, "CLS is nullptr");
  if (CLS::classof(ssa)) {
    return true;
  } else {
    return false;
  }
}

template <typename CLS, typename TYPE>
bool IsSSA(const TYPE *ssa) {
  DBG_ASSERT(ssa != nullptr, "CLS is nullptr");
  if (CLS::classof(ssa)) {
    return true;
  } else {
    return false;
  }
}

}

#endif //XY_LANG_CASTSSA_H

#include <iostream>
#include "common/idmanager.h"
#include "mid/ir/usedef/user.h"
#include "mid/ir/usedef/value.h"

namespace lava::mid {

void Value::RemoveFromUser() {
  // remove from all users
  while (!_use_list.empty()) {
    _use_list.front()->getUser()->RemoveValue(this);
  }
}

/* ---------------------------- Methods of dumping DEBUG IR ------------------------------- */

// used for dumping ssa in gdb
static IdManager dbg_mgr;
static std::set<const Value *> visited;

void Value::dump() const {
  AssignId(dbg_mgr);
  Dump(std::cerr, dbg_mgr);
  std::cerr << std::endl;
  dbg_mgr.Reset();
}

void Value::AssignId(IdManager &id_mgr) const {
  visited.insert(this);
  if (isUser()) {
    const User * user = static_cast<const User *>(this);
    for (const auto &it : *user) {
      auto &value = it.value();
      if ((value == nullptr) || visited.count(value.get())) continue;
      value->AssignId(id_mgr);
    }
  }
  id_mgr.GetId(this);
}



}

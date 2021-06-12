#include "idmanager.h"

namespace lava::mid {


void IdManager::Reset() {
  _cur_id           = 0;
  _block_id         = 0;
  _if_cond_id       = 0;
  _then_id          = 0;
  _else_id          = 0;
  _if_end_id        = 0;
  _while_cond_id    = 0;
  _loop_body_id     = 0;
  _while_end_id     = 0;
  _ids.clear();
}


std::optional<std::size_t> IdManager::findValue(const Value *value, IdType idType) {
  if (idType == IdType::_ID_VAR) {
   auto it = _ids.find(value);
   if (it == _ids.end()) {
//    auto id = _cur_id++;
//    _ids.insert({value, id});
     return {};
   } else {
     return it->second;
   }
  } else {
    auto it = _blocks.find(value);
    if (it == _blocks.end()) {
      return {};
    } else {
      return it->second;
    }
  }
}

std::size_t IdManager::GetId(const Value *value, IdType idType) {
  auto res = findValue(value, idType);
  if (res.has_value()) return res.value();

  std::size_t id;

  switch (idType) {
    case IdType::_ID_VAR: {
      id = _cur_id++;
      _ids.insert({value, id});
      break;
    }
    case IdType::_ID_BLOCK:      id = _block_id++;      break;
    case IdType::_ID_IF_COND:    id = _if_cond_id++;    break;
    case IdType::_ID_THEN:       id = _then_id++;       break;
    case IdType::_ID_ELSE:       id = _else_id++;       break;
    case IdType::_ID_IF_END:     id = _if_end_id++;     break;
    case IdType::_ID_WHILE_COND: id = _while_cond_id++; break;
    case IdType::_ID_LOOP_BODY:  id = _loop_body_id++;  break;
    case IdType::_ID_WHILE_END:  id = _while_end_id++;  break;
  }
  _blocks.insert({value, id});
  return id;
}

std::size_t IdManager::GetId(const SSAPtr &value, IdType idType) {
   return GetId(value.get(), idType);
}

void IdManager::RecordName(const Value *value, std::string_view name) {
  if (_names.find(value) == _names.end()) {
    _names.insert({value, name});
  }
}

std::optional<std::string_view> IdManager::GetName(const Value *value) {
  auto it = _names.find(value);
  if (it != _names.end()) {
    return it->second;
  } else {
    return {};
  }
}


}
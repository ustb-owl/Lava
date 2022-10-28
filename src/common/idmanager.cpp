#include "common/idmanager.h"

namespace lava {


void IdManager::Reset() {
  // IR
  _cur_id           = 0;
  _block_id         = 0;
  _if_cond_id       = 0;
  _then_id          = 0;
  _else_id          = 0;
  _if_end_id        = 0;
  _while_cond_id    = 0;
  _loop_body_id     = 0;
  _while_end_id     = 0;
  _lhs_true_id      = 0;
  _lhs_false_id     = 0;
  _land_end         = 0;
  _lor_end          = 0;
  _phi_id           = 0;

  // LLIR
  _ll_block_id      = 0;
  _ll_if_cond_id    = 0;
  _ll_then_id       = 0;
  _ll_else_id       = 0;
  _ll_if_end_id     = 0;
  _ll_while_cond_id = 0;
  _ll_loop_body_id  = 0;
  _ll_while_end_id  = 0;
  _ll_lhs_true_id   = 0;
  _ll_lhs_false_id  = 0;
  _ll_land_end      = 0;
  _ll_lor_end       = 0;

  _ids.clear();
  _blocks.clear();
  _names.clear();
  _ll_blocks.clear();
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
    case IdType::_ID_LHS_TRUE:   id = _lhs_true_id++;   break;
    case IdType::_ID_LHS_FALSE:  id = _lhs_false_id++;  break;
    case IdType::_ID_LAND_END:   id = _land_end++;      break;
    case IdType::_ID_LOR_END:    id = _lor_end++;       break;
    case IdType::_ID_PHI:        id = _phi_id++;        break;
    default:
      ERROR("should not reach here");
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

std::optional<std::size_t> IdManager::findBlock(const lava::back::LLBasicBlock *block) {
  auto it = _ll_blocks.find(block);
  if (it == _ll_blocks.end()) {
    return {};
  } else {
    return it->second;
  }
}

std::size_t IdManager::GetId(const lava::back::LLBasicBlock *block, IdType idType) {
  auto res = findBlock(block);
  if (res.has_value()) return res.value();

  std::size_t id;

  switch (idType) {
    case IdType::_ID_LL_BLOCK:      id = _ll_block_id++;      break;
    case IdType::_ID_LL_IF_COND:    id = _ll_if_cond_id++;    break;
    case IdType::_ID_LL_THEN:       id = _ll_then_id++;       break;
    case IdType::_ID_LL_ELSE:       id = _ll_else_id++;       break;
    case IdType::_ID_LL_IF_END:     id = _ll_if_end_id++;     break;
    case IdType::_ID_LL_WHILE_COND: id = _ll_while_cond_id++; break;
    case IdType::_ID_LL_LOOP_BODY:  id = _ll_loop_body_id++;  break;
    case IdType::_ID_LL_WHILE_END:  id = _ll_while_end_id++;  break;
    case IdType::_ID_LL_LHS_TRUE:   id = _ll_lhs_true_id++;   break;
    case IdType::_ID_LL_LHS_FALSE:  id = _ll_lhs_false_id++;  break;
    case IdType::_ID_LL_LAND_END:   id = _ll_land_end++;      break;
    case IdType::_ID_LL_LOR_END:    id = _ll_lor_end++;       break;
    default:
      ERROR("should not reach here");
  }
  _ll_blocks.insert({block, id});
  return id;
}


}
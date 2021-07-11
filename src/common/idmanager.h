#ifndef LAVA_IDMANAGER_H
#define LAVA_IDMANAGER_H

#include <optional>
#include <unordered_map>
#include "mid/ir/usedef/value.h"
#include "back/arch/arm/instdef.h"

using namespace lava::mid;

namespace lava {
enum class IdType {
  // IR
  _ID_VAR           = 0,
  _ID_BLOCK         = 1,
  _ID_IF_COND       = 2,
  _ID_THEN          = 3,
  _ID_ELSE          = 4,
  _ID_IF_END        = 5,
  _ID_WHILE_COND    = 6,
  _ID_LOOP_BODY     = 7,
  _ID_WHILE_END     = 8,
  _ID_LHS_TRUE      = 9,
  _ID_LHS_FALSE     = 10,
  _ID_LAND_END      = 11,
  _ID_LOR_END       = 12,

  // LLIR
  _ID_LL_VIRTUAL    = 13,
  _ID_LL_BLOCK      = 14,
  _ID_LL_IF_COND    = 15,
  _ID_LL_THEN       = 16,
  _ID_LL_ELSE       = 17,
  _ID_LL_IF_END     = 18,
  _ID_LL_WHILE_COND = 19,
  _ID_LL_LOOP_BODY  = 20,
  _ID_LL_WHILE_END  = 21,
  _ID_LL_REAL       = 22,
  _ID_LL_LHS_TRUE   = 23,
  _ID_LL_LHS_FALSE  = 24,
  _ID_LL_LAND_END   = 25,
  _ID_LL_LOR_END    = 26,
};

class IdManager {
private:
  // IR
  std::size_t                                         _cur_id;        // current id
  std::size_t                                         _block_id;      // current block id
  std::size_t                                         _if_cond_id;    // current cond id
  std::size_t                                         _then_id;       // current then block id
  std::size_t                                         _else_id;       // current else block id
  std::size_t                                         _if_end_id;     // current else block id
  std::size_t                                         _while_cond_id; // current cond id
  std::size_t                                         _loop_body_id;  // current loop block id
  std::size_t                                         _while_end_id;  // current while end id
  std::size_t                                         _lhs_true_id;   // current lhs true id
  std::size_t                                         _lhs_false_id;  // current lhs false id
  std::size_t                                         _land_end;      // current land id
  std::size_t                                         _lor_end;       // current lor id

  // LLIR
  std::size_t                                         _ll_block_id;      // current block id
  std::size_t                                         _ll_if_cond_id;    // current cond id
  std::size_t                                         _ll_then_id;       // current then block id
  std::size_t                                         _ll_else_id;       // current else block id
  std::size_t                                         _ll_if_end_id;     // current else block id
  std::size_t                                         _ll_while_cond_id; // current cond id
  std::size_t                                         _ll_loop_body_id;  // current loop block id
  std::size_t                                         _ll_while_end_id;  // current while end id
  std::size_t                                         _ll_lhs_true_id;   // current lhs true id
  std::size_t                                         _ll_lhs_false_id;  // current lhs false id
  std::size_t                                         _ll_land_end;      // current land id
  std::size_t                                         _ll_lor_end;       // current lor id


  // IR
  std::unordered_map<const Value *, std::size_t>      _ids;           // local values id
  std::unordered_map<const Value *, std::size_t>      _blocks;        // store blocks name
  std::unordered_map<const Value *, std::string_view> _names;         // store global variables name

  // LLIR
  std::unordered_map<const lava::back::LLBasicBlock*, std::size_t> _ll_blocks; // store blocks name


  std::optional<std::size_t> findValue(const Value *value, IdType type);
  std::optional<std::size_t> findBlock(const lava::back::LLBasicBlock *block);
public:

  IdManager()
    : _cur_id(0), _block_id(0), _if_cond_id(0), _then_id(0), _else_id(0),
      _if_end_id(0), _while_cond_id(0), _loop_body_id(0), _while_end_id(0),
      _lhs_true_id(0), _lhs_false_id(0), _land_end(0), _lor_end(0) {}

  void Reset();

  // get id for local vale
  std::size_t GetId(const Value *value, IdType idType = IdType::_ID_VAR);
  std::size_t GetId(const SSAPtr &value, IdType idType = IdType::_ID_VAR);

  // get id for LLBasicBlock
  std::size_t GetId(const lava::back::LLBasicBlock *block, IdType idType);
  std::size_t GetId(const lava::back::LLBlockPtr &block, IdType idType) {
    return GetId(block.get(), idType);
  }

  // record global values(variables and functions)
  void RecordName(const Value *value, std::string_view name);

  // get name of global values
  std::optional<std::string_view> GetName(const Value *value);
  std::optional<std::string_view> GetName(const SSAPtr &value) { return GetName(value.get()); }
};

}
#endif //LAVA_IDMANAGER_H

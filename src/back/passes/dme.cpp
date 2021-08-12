#include "dme.h"
#include "common/casting.h"

namespace lava::back {


void DeadMoveElimination::runOn(const LLFunctionPtr &func) {
  // collect used locally registers
  if (func->is_decl()) return;
  if (func->function()->GetFunctionName() == "KMP") return;
  if (func->function()->GetFunctionName() == "read_str") return;
  if (func->function()->GetFunctionName() == "get_next") return;
  if (func->function()->GetFunctionName() == "main") return;
  bool changed = true;

  while (changed) {
    changed = false;

    for (const auto &block : func->blocks()) {
      for (auto it = block->insts().begin(); it != block->insts().end(); it++) {
        if (auto move_inst = dyn_cast<LLMove>(*it)) {
          auto dst = move_inst->dst();
          auto src = move_inst->src();

          // movcond rd, rn, ...
          if (!move_inst->IsSimple()) continue;
          // mov rd, imm_need_split
          if (src->IsImmediate() && !lava::back::LLModule::can_encode_imm(src->imm_num())) continue;
          // mov rd, r0~r3
          if (src->IsRealReg() && (src->reg() <= ArmReg::r3)) continue;


          // record move instruction
          if (_visited.find(dst) == _visited.end()) {
            _mov_dst_map.insert({dst, {block, it}});
            _visited.insert(dst);
          }

          auto pos = std::next(it);
          while (pos != block->insts().end()) {

            // break if dst is killed
            if ((*pos)->dest() == dst) break;
            // break if src is killed
            if ((*pos)->dest() == src) break;

            // mov r0, 1
            // ...
            // call func
            // mov r2, r0
            // should not replace with mov r2, 1
            if ((dst->IsRealReg() && dst->reg() <= ArmReg::r3) ||
                (src->IsRealReg() && src->reg() <= ArmReg::r3)) {
              if (IsSSA<LLCall>(*pos)) break;
            }

            if (auto next_mov = dyn_cast<LLMove>(*pos)) {
              // mov r1, r0
              // mov r2, r1 => mov r2, r0
              if (dst == next_mov->src()) {
                changed = true;
                next_mov->set_operand(src, 0);
              }
            } else if (auto binary_inst = dyn_cast<LLBinaryInst>(*pos)) {
              // mov r0, 2
              // add dst, r0, r1, ... => add dst, 2, r1, ...
              auto operands = binary_inst->operands();
              for (auto i = 0; i < operands.size(); i++) {
                if (src->IsImmediate()) break;
                if (dst == operands[i]) {
                  changed = true;
                  binary_inst->set_operand(src, i);
                }
              }

            } else if (auto store_inst = dyn_cast<LLStore>(*pos)) {
              // mov r0, r2
              // store r0, [...] => store r2, [...]
              // store ..., [r0] => store ..., [r2]
              auto operands = store_inst->operands();
              for (auto i = 0; i < operands.size(); i++) {
                if (src->IsImmediate() && i < 2) continue;
                if (dst == operands[i]) {
                  changed = true;
                  store_inst->set_operand(src, i);
                }
              }
            } else if (auto load_inst = dyn_cast<LLLoad>(*pos)) {
              // mv r0, r2
              // ldr rd, [r0] => ldr rd, [r2]
              // ldr r0, [rs] => ldr r2, [rs]
              auto operands = load_inst->operands();
              for (auto i = 0; i < operands.size(); i++) {
                if (src->IsImmediate() && i < 2) continue;
                if (dst == operands[i]) {
                  changed = true;
                  load_inst->set_operand(src, i);
                }
              }
            }
            pos++;
          }

        }
      }

      // remove unused move
      for (auto &it : block->insts()) {
        for (const auto &opr : it->operands()) {
          auto res = _mov_dst_map.find(opr);
          if (res != _mov_dst_map.end()) {
            _mov_dst_map.erase(res);
          }
        }
      }

    }

    for (auto &[k, v] : _mov_dst_map) {
      if (k->IsRealReg() && (k->reg() <= ArmReg::r3)) continue;
      changed = true;
      v.first->insts().erase(v.second);
    }
    _mov_dst_map.clear();
    _visited.clear();
  }
}

}
#include "peephole.h"
#include "common/casting.h"


namespace lava::back {


void PeepHole::runOn(const LLFunctionPtr &func) {
  for (auto BB = func->blocks().begin(); BB != func->blocks().end(); BB++) {
    auto block = *BB;
    for (auto it = block->inst_begin(); it != block->inst_end(); it++) {
      // 1. eliminate useless move instruction
      if (auto mv_inst = dyn_cast<LLMove>(*it)) {
        // 1) mov rd, rd
        if ((mv_inst->dst() == mv_inst->src()) && mv_inst->IsSimple()) {
          it = block->insts().erase(it);
        } else {
          // 2. mov rd, 1; mov rd, 2  ===> mov rd, 2

          // get next iterator
          auto next = std::next(it);

          // check if next is end of instructions
          if (next != block->insts().end()) {
            // check if next instruction is move
            if (auto next_mov = dyn_cast<LLMove>(*next)) {
              if ((next_mov->dst() == mv_inst->dst()) && !(next_mov->src() == mv_inst->dst()) && next_mov->IsSimple()) {
                it = block->insts().erase(it);
              }
            }
          }
        }
      } else if (auto binary_inst = dyn_cast<LLBinaryInst>(*it)) {
        /* 2. Eliminate useless add or sub instruction */
        // add/sub rd, rd, 0
        auto opcode = binary_inst->opcode();
        if ((opcode == LLInst::Opcode::Add) || (opcode == LLInst::Opcode::Sub)) {
          if ((binary_inst->dst() == binary_inst->lhs()) &&
              (binary_inst->rhs() == LLOperand::Immediate(0)) &&
              (binary_inst->shift().is_none())) {
            it = block->insts().erase(it);
          }
        }
      } else if (auto load_inst = dyn_cast<LLLoad>(*it)) {
        /* 3. match:
         * str r0, [r1, #0w]
         * ldr r2, [r1, #0]
         * ldr can be opt to:
         * mov r2, r0
         */
        if (it == block->insts().begin()) continue;
        auto prev = std::prev(it);
        if (auto store_inst = dyn_cast<LLStore>(*prev)) {
          if ((load_inst->addr() == store_inst->addr()) &&
              (load_inst->offset() == store_inst->offset())) {
            auto move_inst = std::make_shared<LLMove>(load_inst->dst(), store_inst->data());
            it = block->insts().erase(it);
            it = block->insts().insert(it, move_inst);
            it--;
          }
        }

      } else if (auto jump_inst = dyn_cast<LLJump>(*it)) {
        // opt control flow in final pass
        if (!_is_final) continue;

        auto next_block = std::next(BB);
        if (jump_inst->target() == *next_block) {
          it = block->insts().erase(it);
        }
      } else if (auto branch_inst = dyn_cast<LLBranch>(*it)) {
        // opt control flow in final pass
        if (!_is_final) continue;

        auto next_block = std::next(BB);
        if (branch_inst->false_block() == *next_block) {
          branch_inst->set_out_false(false);
        }
      }


    }
  }

}

}
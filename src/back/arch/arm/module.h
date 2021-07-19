#ifndef LAVA_BACK_MODULE_H
#define LAVA_BACK_MODULE_H

#include "instdef.h"

namespace lava::back {

using LLFunctionList = std::vector<LLFunctionPtr>;

class LLModule {
private:
  LLFunctionList                                 _functions;
  std::vector<mid::GlobalVariable *>             _glob_decl;

  std::size_t                                    _virtual_max;
  LLFunctionPtr                                  _insert_function;
  LLBlockPtr                                     _insert_point;
  LLInstList::iterator                           _insert_pos;
  std::unordered_map<mid::BlockPtr, LLBlockPtr>  _block_map;
  std::unordered_map<mid::SSAPtr, LLOperandPtr>  _value_map;

  std::unordered_map<mid::SSAPtr, std::pair<LLInstPtr, ArmCond>>  _cond_map;
  std::unordered_map<std::shared_ptr<mid::GlobalVariable>, LLOperandPtr> _glob_map;
public:

  // create a new LLIR
  template <typename T, typename... Args>
  auto MakeLLIR(Args &&... args) {
    static_assert(std::is_base_of_v<LLInst, T>);
    auto llir = std::make_shared<T>(std::forward<Args>(args)...);
    llir->SetParent(_insert_point);
    return llir;
  }

  // create a new instruction LLIR, and push into current block
  template <typename T, typename... Args>
  auto AddInst(Args &&... args) {
    auto inst = MakeLLIR<T>(std::forward<Args>(args)...);
    _insert_pos = ++_insert_point->insts().insert(_insert_pos, inst);
    return inst;
  }

  void reset();

  LLModule() { reset(); }

  void SetInsertPoint(const LLBlockPtr &BB, LLInstList::iterator it) {
    _insert_point = BB;
    _insert_pos = it;
  }

  void SetInsertPoint(const LLBlockPtr &BB) {
    SetInsertPoint(BB, BB->inst_end());
  }

  void SetGlobalVariables(std:: vector<mid::GlobalVariable *> glob_decl) {
    _glob_decl = glob_decl;
  }

  // see https://alisdair.mcdiarmid.org/arm-immediate-value-encoding/
  static inline bool can_encode_imm(int imm) {
    unsigned encoding = imm;
    for (int ror = 0; ror < 32; ror += 2) {
      if (!(encoding & ~0xFFu)) {
        return true;
      }
      encoding = (encoding << 2u) | (encoding >> 30u);
    }
    return false;
  }

  void ClearVirtualMax() { _virtual_max = 0; }

  void DumpASM(std::ostream &os) const;

  /* Creators */
  LLOperandPtr  CreateOperand(const mid::SSAPtr &value);
  LLOperandPtr  CreateNoImmOperand(const mid::SSAPtr &value);
  LLOperandPtr  CreateImmediate(int value);
  LLFunctionPtr CreateFunction(const mid::FuncPtr &function);
  LLBlockPtr    CreateBasicBlock(const mid::BlockPtr &block, const LLFunctionPtr& parent);


  // getter/setter
  typedef LLFunctionList::iterator       iterator;
  typedef LLFunctionList::const_iterator const_iterator;

  LLFunctionList       &Functions()    { return _functions;         }
  LLBlockPtr           &InsertPoint()  { return _insert_point;      }
  LLInstList::iterator  InsertPos()    { return _insert_pos;        }
  std::size_t           VirtualMax()   { return _virtual_max;       }

  iterator              begin()        { return _functions.begin(); }
  iterator              end()          { return _functions.end();   }
  const_iterator        begin() const  { return _functions.begin(); }
  const_iterator        end()   const  { return _functions.end();   }

};

/* Methods of dumping ASM codes */
std::ostream &operator<<(std::ostream &os, const LLFunctionPtr &function);
std::ostream &operator<<(std::ostream &os, const LLBlockPtr &block);
std::ostream &operator<<(std::ostream &os, const LLInstPtr &inst);
std::ostream &operator<<(std::ostream &os, const LLOperandPtr &operand);
std::ostream &operator<<(std::ostream &os, ArmReg armReg);

}

#endif //LAVA_BACK_MODULE_H

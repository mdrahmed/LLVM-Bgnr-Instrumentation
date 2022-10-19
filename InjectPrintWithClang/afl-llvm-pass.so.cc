

#define AFL_LLVM_PASS
#define AFL_R(x) (random() % (x))
#define MAP_SIZE  (1 << 16)

// #include "../config.h"
// #include "../debug.h"

#include <unordered_set>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;

namespace {

  class AFLCoverage : public ModulePass {

    public:

      static char ID;
      AFLCoverage() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;

      // StringRef getPassName() const override {
      //  return "American Fuzzy Lop Instrumentation";
      // }

  };

}


char AFLCoverage::ID = 0;


bool AFLCoverage::runOnModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      errs() << "Bad value of AFL_INST_RATIO (must be between 1 and 100)";

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

//   GlobalVariable *AFLMapPtr =
//       new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
//                          GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");
// 
//   GlobalVariable *AFLPrevLoc = new GlobalVariable(
//       M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
//       0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0;

  for (auto &F : M)
    for (auto &BB : F) {

      bool skipBB = false;
      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      /* Global values in current BB */
      std::unordered_set<GlobalVariable *> globalSet;
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio) continue;

      /* Make up cur_loc */

      unsigned int cur_loc = AFL_R(MAP_SIZE);
      errs() << cur_loc << "\n";


      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      for (Instruction &II : BB)
      {
        Instruction *I = &II;
        /* for global values */
        for (Value *Op : I->operands())
        {
          if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Op))
          {
            // Do something with GV
            if (!(GV->isConstant()) )
            {
              globalSet.insert(GV);
              errs() << "GVi - " << *GV << "\n";
            }
          }
          else if (GEPOperator *gepo = dyn_cast<GEPOperator>(Op))
          {
            if (GlobalVariable *gv = dyn_cast<GlobalVariable>(gepo->getPointerOperand()))
            {
              if (!(gv->isConstant()) )
              {
                globalSet.insert(gv);
                errs() << "GVi - " << *gv << "\n";
              }
            }
            for (auto it = gepo->idx_begin(), et = gepo->idx_end(); it != et; ++it)
            {
              if (GlobalVariable *gv = dyn_cast<GlobalVariable>(*it))
              {
                errs() << "GVi - " << *gv << "\n";
              }
            }
          }
        }
      }

      /* Insert code for logging global variables in each BB */
      if (globalSet.size() > 0)
      {
        /* FILE *fileHandle = fopen("./fs_globals.txt", "a"); */
        std::string dumpFilename =  "./" + M.getName().str() + ".fs_globals.txt";
        auto *fopenType = FunctionType::get(
            IRB.getInt8PtrTy(),
            SmallVector<Type *, 2>{IRB.getInt8PtrTy(), IRB.getInt8PtrTy()},
            false);

        auto fopenFunc = M.getOrInsertFunction("fopen", fopenType);
        auto *fileHandle = IRB.CreateCall(fopenFunc, SmallVector<Value *, 2>{
                                                         IRB.CreateGlobalStringPtr(dumpFilename),
                                                         IRB.CreateGlobalStringPtr("a")});

        // Printed lines will be in the format "name<tab>address<tab>size"
        auto *fprintfType = FunctionType::get(
            IRB.getInt32Ty(),
            SmallVector<Type *, 1>{IRB.getInt8PtrTy()},
            true);

        auto fprintfFunc = M.getOrInsertFunction("fprintf", fprintfType);

        DataLayout dataLayout(&M);

        for (auto *global : globalSet)
        {
          // fprintf("%s\t%p\t%lld\n", name, address, size);
          // errs() << "GV - " << *global << "\n";
          if (!global->getValueType()->isSized())
            continue;

          auto *name = IRB.CreateGlobalStringPtr(global->getName());
          //auto *size = ConstantInt::get(
            //  IRB.getInt64Ty(),
              //dataLayout.getTypeSizeInBits(global->getValueType()).getFixedSize() / 8);
          
	  auto *size = ConstantInt::get(
              IRB.getInt64Ty(),
              dataLayout.getTypeSizeInBits(global->getValueType()) / 8);
	  IRB.CreateCall(fprintfFunc, SmallVector<Value *, 5>{
                                          fileHandle,
                                          IRB.CreateGlobalStringPtr("%s\t%p\t%lld\n"),
                                          name,
                                          global,
                                          size});
        }

        /* fclose(fileHandle); */
        auto *fcloseType = FunctionType::get(
            IRB.getInt8PtrTy(),
            SmallVector<Type *, 1>{IRB.getInt8PtrTy()},
            false);

        auto fcloseFunc = M.getOrInsertFunction("fclose", fcloseType);
        IRB.CreateCall(fcloseFunc, SmallVector<Value *, 1>{fileHandle});
      }

      /* Load prev_loc */

      // LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
      // PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      // Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      // /* Load SHM pointer */

      // LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
      // MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      // Value *MapPtrIdx =
      //     IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      // /* Update bitmap */

      // LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      // Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      // Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      // IRB.CreateStore(Incr, MapPtrIdx)
      //     ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      // /* Set prev_loc to cur_loc >> 1 */

      // StoreInst *Store =
      //     IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
      // Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      // inst_blocks++;


    }

  /* Say something nice. */


  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);




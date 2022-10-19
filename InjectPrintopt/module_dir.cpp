#include "InjectFuncCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

#define DEBUG_TYPE "inject-func-call"

//-----------------------------------------------------------------------------
// InjectFuncCall implementation
//-----------------------------------------------------------------------------

inline const char * const BoolToString(bool b)
{
  return b ? "true" : "false";
}


bool InjectFuncCall::runOnModule(Module &M) {
  bool InsertedAtLeastOnePrintf = false;

  auto &CTX = M.getContext();
  PointerType *PrintfArgTy = PointerType::getUnqual(Type::getInt8Ty(CTX));

  // STEP 1: Inject the declaration of printf
  // ----------------------------------------
  // Create (or _get_ in cases where it's already available) the following
  // declaration in the IR module:
  //    declare i32 @printf(i8*, ...)
  // It corresponds to the following C declaration:
  //    int printf(char *, ...)
  FunctionType *PrintfTy = FunctionType::get(
      IntegerType::getInt32Ty(CTX),
      PrintfArgTy,
      /*IsVarArgs=*/true);

  FunctionCallee Printf = M.getOrInsertFunction("printf", PrintfTy);

  // Set attributes as per inferLibFuncAttributes in BuildLibCalls.cpp
  Function *PrintfF = dyn_cast<Function>(Printf.getCallee());
  PrintfF->setDoesNotThrow();
  PrintfF->addParamAttr(0, Attribute::NoCapture);
  PrintfF->addParamAttr(0, Attribute::ReadOnly);


  // STEP 2: Inject a global variable that will hold the printf format string
  // ------------------------------------------------------------------------
  llvm::Constant *PrintfFormatStr = llvm::ConstantDataArray::getString(
      CTX, "Condition name: %s\n   number of arguments: %d\n");

  Constant *PrintfFormatStrVar =
      M.getOrInsertGlobal("PrintfFormatStr", PrintfFormatStr->getType());
  dyn_cast<GlobalVariable>(PrintfFormatStrVar)->setInitializer(PrintfFormatStr);

  // STEP 3: For each function in the module, inject a call to printf
  // ----------------------------------------------------------------
  for (auto &F : M) {
   // if (F.isDeclaration())
     // continue;
    //for(Function::iterator bb=F.begin();bb != F.end();bb++){
    for(auto &bb : F){  
      	  errs()<<"BasicBlock name = "<<bb.getName()<<"\n";
          errs()<<"BasicBlock size = "<<bb.size()<<"\n";
	 
//// This if with bb is inserting printf inside basic block, if I remove that bb->getName() with F.getName() then it will insert printf only inside that function e.g., foo		 
	 //if (bb -> getName() == "entry")
	 // {
	 //       Instruction* first_ins = bb->getFirstNonPHI();
         //       errs()<<"1st instruciton: "<<*first_ins<<"\n";
         //       errs()<<"Inside the "<< F.getName()  <<"\n";

	 //       IRBuilder<> Builder(first_ins);
	
         //       // Inject a global variable that contains the function name
         //       auto FuncName = Builder.CreateGlobalStringPtr(F.getName());

         //       // Printf requires i8*, but PrintfFormatStrVar is an array: [n x i8]. Add
         //       // a cast: [n x i8] -> i8*
         //       llvm::Value *FormatStrPtr =
         //           Builder.CreatePointerCast(PrintfFormatStrVar, PrintfArgTy, "formatStr");

         //       // The following is visible only if you pass -debug on the command line
         //       // *and* you have an assert build.
         //       LLVM_DEBUG(dbgs() << " Injecting call to printf inside " << F.getName()
         //                         << "\n");

         //       // Finally, inject a call to printf
         //       Builder.CreateCall(
         //           Printf, {FormatStrPtr, FuncName, Builder.getInt32(F.arg_size())});
	 //}
	 

	  for(auto &I : bb){
	          if(auto *op = dyn_cast<BranchInst>(&I)){
	        	  IRBuilder<> Builder(op);
                	  //Inject a global variable that contains the function name
			  //auto FuncName = Builder.CreateGlobalStringPtr("Function name");
			  
			  Value* condition;
			  Value* false_dst;
                          Value* true_dst;

			  //Value *lhs = op->getOperand(0);
                          //Value *rhs = op->getOperand(1);
                          //*lhs = op->getOperand(0);
                          //*rhs = op->getOperand(1);
                          //errs()<<"Predicate: "<<op->getPredicate()<<'\n';
                          //errs()<<"Arg operator: "<<op->get()<<'\n';
                          //errs()<<"value at left hand side: "<<*lhs<<"\n";
                          //errs()<<"value at right hand side: "<<*rhs<<"\n";
                          if (op->isConditional()) {
                              condition = op->getOperand(0);
                              false_dst = op->getOperand(1);
                              true_dst = op->getOperand(2);
                          } else {
                              condition = Builder.getInt32(1);
                              false_dst = op->getOperand(0);
                              true_dst = op->getOperand(0);
                          }
				
			  //got error
			  //auto FuncName = Builder.CreateGlobalStringPtr(StringRef(op -> isConditional(),5));

			// auto FuncName = Builder.CreateGlobalStringPtr(F.getName());
			  
			  // got the boolean result
			  //auto FuncName = Builder.CreateGlobalStringPtr(BoolToString(op -> isConditional()));
                	 
			  
			   auto FuncName = Builder.CreateGlobalStringPtr(condition->getName());
			  
			 // auto FuncName = Builder.CreateGlobalStringPtr(F.getName());

			  // Printf requires i8*, but PrintfFormatStrVar is an array: [n x i8]. Add
                	  // a cast: [n x i8] -> i8*
                	  llvm::Value *FormatStrPtr =
                	      Builder.CreatePointerCast(PrintfFormatStrVar, PrintfArgTy, "formatStr");

                	  // The following is visible only if you pass -debug on the command line
                	  // *and* you have an assert build.
                	  LLVM_DEBUG(dbgs() << " Injecting call to printf inside " << F.getName()
                	                    << "\n");

                	  // Finally, inject a call to printf
                	  Builder.CreateCall(Printf, {FormatStrPtr, FuncName, Builder.getInt32(F.arg_size())});
	          }
	  }

    }
    // Get an IR builder. Sets the insertion point to the top of the function
    InsertedAtLeastOnePrintf = true;
  }

  return InsertedAtLeastOnePrintf;
}

PreservedAnalyses InjectFuncCall::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &) {
  bool Changed =  runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyInjectFuncCall::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getInjectFuncCallPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "inject-func-call", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "inject-func-call") {
                    MPM.addPass(InjectFuncCall());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInjectFuncCallPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyInjectFuncCall::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyInjectFuncCall>
    X(/*PassArg=*/"legacy-inject-func-call", /*Name=*/"LegacyInjectFuncCall",
      /*CFGOnly=*/false, /*is_analysis=*/false);

static void registerLegacyInjectFuncCall(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new LegacyInjectFuncCall());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerLegacyInjectFuncCall);



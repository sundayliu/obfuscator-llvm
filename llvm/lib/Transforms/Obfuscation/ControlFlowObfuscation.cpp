#include "llvm/Transforms/Obfuscation/ControlFlowObfuscation.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace{
	struct ControlFlowObfuscation :public FunctionPass{
		static char ID;
		ControlFlowObfuscation() :FunctionPass(ID){}
		bool runOnFunction(Function &F){
			return false;
		}

	};
}

char ControlFlowObfuscation::ID = 0;
static RegisterPass<ControlFlowObfuscation> X("controlFlowObfuscation", "Obfuscation-control flow");
Pass* llvm::createControlFlowObfuscation(){
	return new ControlFlowObfuscation();
}
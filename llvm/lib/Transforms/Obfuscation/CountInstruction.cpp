
#include "llvm/Transforms/Obfuscation/CountInstruction.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"


#include <map>

#define DEBUG_TYPE "opCount"

using namespace llvm;
namespace{
	struct CountInstruction : public FunctionPass{
		std::map<std::string, int> opCounter;
		static char ID;
		CountInstruction() :FunctionPass(ID){}
		virtual bool runOnFunction(Function &F){
			errs() << "Function " << F.getName() << "\n";
			for (Function::iterator bb = F.begin(), e = F.end(); bb != e; ++bb){
				for (BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i){
					if (opCounter.find(i->getOpcodeName()) == opCounter.end()){
						opCounter[i->getOpcodeName()] = 1;
					}
					else{
						opCounter[i->getOpcodeName()] += 1;
					}
				}
			}

			std::map<std::string, int>::iterator i = opCounter.begin();
			std::map<std::string, int>::iterator e = opCounter.end();
			while (i != e){
				errs() << i->first << ":" << i->second << "\n";
				i++;
			}
			errs() << "\n";
			opCounter.clear();
			return false;
		}
	};
}

char CountInstruction::ID = 0;
static RegisterPass<CountInstruction> X("opCounter", "Count opcodes per function");
Pass* llvm::createCountInstruction(){
	return new CountInstruction();
}
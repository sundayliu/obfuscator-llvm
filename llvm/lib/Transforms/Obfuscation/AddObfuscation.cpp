#include "llvm/Transforms/Obfuscation/AddObfuscation.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <vector>

using namespace llvm;

static cl::opt<int> ObfTimes("add-loop", 
	cl::desc("How many loops on a function"), 
	cl::desc("number of times"),
	cl::init(1),
	cl::Optional);

namespace{
	struct AddObfuscation :public FunctionPass{
		static char ID;
		AddObfuscation() :FunctionPass(ID){}
		bool runOnFunction(Function &F){

			int times = ObfTimes;
			do{
				std::vector<Instruction*> needDeleteInstruction;
				for (Function::iterator bb = F.begin(), e = F.end(); bb != e; ++bb){
					for (BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i){
						if (i->isBinaryOp()){
							switch (i->getOpcode()){
							case BinaryOperator::Add:
								if (addNeg(cast<BinaryOperator>(i))){
									needDeleteInstruction.push_back(i);
								}
								break;
							case BinaryOperator::Sub:
								if (subNeg(cast<BinaryOperator>(i))){
									needDeleteInstruction.push_back(i);
								}
								break;
							default:
								break;
							}
						}
					}
				}

				for (std::vector<Instruction*>::iterator i = needDeleteInstruction.begin(), e = needDeleteInstruction.end();
					i != e; ++i){
					(*i)->eraseFromParent();
				}
			} while (--times > 0);
			return false;
		}

	private:
		// a = b + c
		// a = b - (-c)
		// -c
		// b - (-c)
		bool addNeg(BinaryOperator* bo){
			BinaryOperator* op = NULL;
			if (bo->getOpcode() == Instruction::Add){
				op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
				op = BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), op, "", bo);
				// check sign
				// Set or clear the nsw flag on this instruction, which must be an operator which supports this flag.
				op->setHasNoSignedWrap(bo->hasNoSignedWrap());
				op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
				bo->replaceAllUsesWith(op);
				return true;
			}
			return false;
		}

		// a = b - c => a = b + (-c)
		bool subNeg(BinaryOperator* bo){
			BinaryOperator* op = NULL;
			if (bo->getOpcode() == Instruction::Sub){
				op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
				op = BinaryOperator::Create(Instruction::Add, bo->getOperand(0), op, "", bo);
				// check sign
				// Set or clear the nsw flag on this instruction, which must be an operator which supports this flag.
				op->setHasNoSignedWrap(bo->hasNoSignedWrap());
				op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
				bo->replaceAllUsesWith(op);
				return true;
			}
			return false;
		}

	};
}

char AddObfuscation::ID = 0;
static RegisterPass<AddObfuscation> X("addObfuscation", "Instruction add obfuscation");
Pass* llvm::createAddObfuscation(){
	return new AddObfuscation();
}
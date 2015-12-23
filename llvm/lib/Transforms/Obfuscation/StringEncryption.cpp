#include "llvm/Transforms/Obfuscation/StringEncryption.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace llvm;

namespace{
	struct StringEncryption :public ModulePass{
		static char ID;


		std::vector<GlobalVariable*> m_NeedEncryptStrings;
		std::vector<GlobalVariable*> m_NeedDeleteStrings;
		std::vector<Instruction*> m_NeedDeleteInstructions;
		std::map<std::string, GlobalVariable*> m_StringNames;
		uint64_t m_counter;

		StringEncryption() :ModulePass(ID){
			m_NeedEncryptStrings.clear();
			m_NeedDeleteStrings.clear();
			m_NeedDeleteInstructions.clear();
			m_StringNames.clear();
			m_counter = 0;
		}
		bool runOnModule(Module& M){
			//errs() << "StringEncryption::runOnModule\n";
			//M.dump();
			initNeedEncryptStrings(M);
			encryptString(M);
			insertDecryptCode(M);
			deleteInstructionAndGlobalVariable(M);
			return true;
		}

		void initNeedEncryptStrings(Module& M){
			for (Module::global_iterator I = M.global_begin(), E = M.global_end();
				I != E; ++I){
				GlobalVariable* gv = I;
				if (gv->getLinkage() == GlobalVariable::PrivateLinkage 
					|| gv->getLinkage() == GlobalVariable::InternalLinkage
					|| gv->getLinkage() == GlobalVariable::LinkOnceAnyLinkage
					|| gv->getLinkage() == GlobalVariable::LinkOnceODRLinkage){
					if (!gv->hasInitializer()){
						continue;
					}

					if (!gv->isConstant()){
						continue;
					}

					ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(gv->getInitializer());
					if (cds){
						if (cds->isString() || cds->isCString()){
							m_NeedEncryptStrings.push_back(gv);
						}
					}
				}
			}
		}

		void encryptString(Module& M){
			for (std::vector<GlobalVariable*>::iterator i = m_NeedEncryptStrings.begin(), e = m_NeedEncryptStrings.end();
				i != e; ++i){
				GlobalVariable* gv = *i;
				ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(gv->getInitializer());
				if (!cds){
					continue;
				}

				std::string plainValue = cds->isString() ? cds->getAsString() : cds->getAsCString();
				std::string encryptedValue = encrypt(plainValue);
				Constant* c = ConstantDataArray::getString(M.getContext(), encryptedValue, false);
				std::ostringstream name;
				name << ".ollvmstr" << m_counter;
				m_counter++;
				GlobalVariable* gvEncryptedString = new GlobalVariable(M,
					c->getType(),
					true,
					gv->getLinkage(),
					c,
					name.str()
					);
				m_StringNames[name.str()] = gvEncryptedString;
				gv->replaceAllUsesWith(gvEncryptedString);
				m_NeedDeleteStrings.push_back(gv);
			}
		}

		void insertDecryptCode(Module& M){

			for (Module::iterator FI = M.begin(), FE = M.end();
				FI != FE; ++FI){
				for (Function::iterator BI = FI->begin(), BE = FI->end();
					BI != BE; ++BI){
					for (BasicBlock::iterator II = BI->begin(), IE = BI->end();
						II != IE; ++II){
						if (II->getOpcode() == Instruction::Call){
							// Invoke Store ...
							handleCall(M, dyn_cast<CallInst>(II));
						}
					}
				}
			}


		}

		void handleGEP(Module& M, GetElementPtrInst* gep){
			StringRef name = gep->getPointerOperand()->getName();
			std::map<std::string, GlobalVariable*>::iterator it = m_StringNames.find(name.str());
			if (it != m_StringNames.end()){
				ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(it->second->getInitializer());
				uint64_t len = cds->getNumElements();
				Value* decryptedString = decrypt(M, it->second, len, gep);
				std::vector<Value*> idx;
				idx.push_back(gep->getOperand(gep->getNumOperands() - 1));
				GetElementPtrInst* newgep = GetElementPtrInst::Create(nullptr,
					decryptedString,
					ArrayRef<Value*>(idx),
					"",
					gep);
				gep->replaceAllUsesWith(newgep);
				m_NeedDeleteInstructions.push_back(gep);
			}
		}

		void handleCall(Module& M, CallInst* call){
			for (unsigned int i = 0; i < call->getNumArgOperands(); ++i){
				ConstantExpr* ce = dyn_cast<ConstantExpr>(call->getArgOperand(i));
				if (ce == 0){
					continue;
				}

				if (ce->getOpcode() != Instruction::GetElementPtr){
					continue;
				}

				GlobalVariable* gv = dyn_cast<GlobalVariable>(ce->op_begin());
				if (gv == 0){
					continue;
				}

				StringRef name = gv->getName();
				std::map<std::string, GlobalVariable*>::iterator it = m_StringNames.find(name.str());
				if (it != m_StringNames.end()){
					ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(it->second->getInitializer());
					uint64_t len = cds->getNumElements();
					Value* decryptedString = decrypt(M, it->second, len, call);
					call->setArgOperand(i, decryptedString);
				}
			}
		}

		void deleteInstructionAndGlobalVariable(Module& M){
			std::vector<GlobalVariable*>::iterator IG = m_NeedDeleteStrings.begin();
			std::vector<GlobalVariable*>::iterator EG = m_NeedDeleteStrings.end();
			while (IG != EG){
				(*IG)->eraseFromParent();
				++IG;
			}

			std::vector<Instruction*>::iterator II = m_NeedDeleteInstructions.begin();
			std::vector<Instruction*>::iterator EI = m_NeedDeleteInstructions.end();
			while (II != EI){
				(*II)->eraseFromParent();
				++II;
			}
		}


		std::string encrypt(const std::string& src){
			// per string per key or encrypt method
			std::string result = src;
			for (std::string::size_type i = 0; i < result.size(); ++i){
				result[i] = result[i] ^ 0x88;
			}
			return result;
		}

		Value* decrypt(Module& M, Value* data, uint64_t len, Instruction* insertBefore){
			// %1 = alloca i8,i64 len
			AllocaInst* alloca = new AllocaInst(IntegerType::getInt8Ty(M.getContext()),
				ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), len),
				"",
				insertBefore);

			for (uint64_t i = 0; i < len; i++){
				std::vector<Value*> idx;

				// %dst = getelementptr inbounds i8* %1, i64 i
				idx.push_back(ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), i));
				GetElementPtrInst* dst = GetElementPtrInst::CreateInBounds(alloca, ArrayRef<Value*>(idx), "", insertBefore);

				// %src = getelemntptr [size x i8]* data, i64 0, i64 i
				if (!dyn_cast<LoadInst>(data)){
					idx.clear();
					idx.push_back(ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), 0));
					idx.push_back(ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), i));
				}

				GetElementPtrInst* src = GetElementPtrInst::Create(nullptr, data, ArrayRef<Value*>(idx), "", insertBefore);

				// %load = load i8* %src, algin 8
				LoadInst* load = new LoadInst(src, "", false, 8, insertBefore);

				// %xor = xor i8 %load,0x88
				BinaryOperator* xor = BinaryOperator::CreateXor(load,
					ConstantInt::get(IntegerType::getInt8Ty(M.getContext()), 0x88),
					"",
					insertBefore);

				// store i8 %xor, i8* %dst, algin 8
				new StoreInst(xor, dst, false, 8, insertBefore);
			}

			return alloca;
		}
	};
}

char StringEncryption::ID = 0;
static RegisterPass<StringEncryption> X("stringEncryption", "Obfuscation-string encryption");
Pass* llvm::createStringEncryption(){
	return new StringEncryption();
}
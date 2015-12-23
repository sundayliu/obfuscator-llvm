#ifndef __OBFUSCATION_COUNT_INSTRUCTION_H__
#define __OBFUSCATION_COUNT_INSTRUCTION_H__

#include "llvm/Pass.h"


namespace llvm{
	Pass* createCountInstruction();
}


#endif // __OBFUSCATION_COUNT_INSTRUCTION_H__
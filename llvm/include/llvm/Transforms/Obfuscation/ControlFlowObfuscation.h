#ifndef __OBFUSCATION_CONTROL_FLOW_OBFUSCATION_H__
#define __OBFUSCATION_CONTROL_FLOW_OBFUSCATION_H__

#include "llvm/Pass.h"


namespace llvm{
	Pass* createControlFlowObfuscation();
}


#endif // __OBFUSCATION_CONTROL_FLOW_OBFUSCATION_H__
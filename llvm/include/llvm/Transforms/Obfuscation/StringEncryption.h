#ifndef __OBFUSCATION_STRING_ENCRYPTION_H__
#define __OBFUSCATION_STRING_ENCRYPTION_H__

#include "llvm/Pass.h"


namespace llvm{
	Pass* createStringEncryption();
}


#endif // __OBFUSCATION_STRING_ENCRYPTION_H__
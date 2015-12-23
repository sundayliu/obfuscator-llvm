add_llvm_library(LLVMObfuscation
    CountInstruction.cpp
    StringEncryption.cpp
    AddObfuscation.cpp
    ControlFlowObfuscation.cpp
    )
    
add_dependencies(LLVMObfuscation intrinsics_gen)
#ifndef JIT_H
#define JIT_H

#include "llvm.h"
#include "runtime.h"

namespace mila {

/** The Rift Memory manager extends the default LLVM memory manager with
    support for resolving the Rift runtime functions. This is achieved by
    extending the behavior of the getSymbolAddress function.
  */
class MemoryManager : public llvm::SectionMemoryManager {
public:
#define NAME_IS(name) if (Name == #name) return reinterpret_cast<uint64_t>(::name)
    /** Return the address of symbol, or nullptr if undefind. We extend the
        default LLVM resolution with the list of RIFT runtime functions.
      */
    uint64_t getSymbolAddress(const std::string & Name) override {
        uint64_t addr = SectionMemoryManager::getSymbolAddress(Name);
        if (addr != 0) return addr;
        // This bit is for some OSes (Windows and OSX where the MCJIT symbol
        // loading is broken)
        NAME_IS(read_);
        NAME_IS(write_);
        llvm::report_fatal_error("Extern function '" + Name + "' couldn't be resolved!");
    }
};

class JIT {
public:

    typedef int (*MainPtr)();

    static MainPtr compile(llvm::Function * main) {
        llvm::Module * m = main->getParent();

        std::string err;

        llvm::TargetOptions opts;
        llvm::ExecutionEngine* engine =
            llvm::EngineBuilder(std::unique_ptr<llvm::Module>(m))
                .setErrorStr(&err)
                .setMCJITMemoryManager(std::unique_ptr<llvm::RTDyldMemoryManager>(new MemoryManager()))
                .setEngineKind(llvm::EngineKind::JIT)
                .setTargetOptions(opts)
                .create();
        if (engine == nullptr)
            throw CompilerError(STR("Could not create ExecutionEngine: " << err));

        engine->finalizeObject();

        /*llvm::ExecutionEngine * engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(m))
            .setMCJITMemoryManager(std::unique_ptr<MemoryManager>(new MemoryManager()))
            .create();
        engine->finalizeObject(); */
        return reinterpret_cast<MainPtr>(engine->getPointerToFunction(main));
    }
};




}


#endif

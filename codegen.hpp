#include <stack>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include "llvm/IR/LegacyPassManager.h"
#include <llvm/IR/Instructions.h>
#include "llvm/IR/IRBuilder.h"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include "llvm/ExecutionEngine/ExecutionEngine.h"

static llvm::LLVMContext GlobalContext;

class NBlock;

class CodeGenBlock
{
public:
    llvm::BasicBlock *block;
    llvm::Value *returnValue;
    std::map<std::string, llvm::Value *> locals;
};

class CodeGenContext
{
    std::stack<CodeGenBlock *> blocks;
    llvm::Function *mainFunction;
    llvm::Function *createPrintfFunction();
    void createPrintFunction(llvm::Function *);

public:
    void createCoreFunctions();
    llvm::Module *module;

    CodeGenContext()
    {
        module = new llvm::Module("main", GlobalContext);
    }

    void generateCode(NBlock &, bool, const char *);
    llvm::GenericValue runCode();
    std::map<std::string, llvm::Value *> &locals() { return blocks.top()->locals; }
    llvm::BasicBlock *currentBlock();
    void pushBlock(llvm::BasicBlock *block);
    void popBlock();
    void setCurrentReturnValue(llvm::Value *value);
    llvm::Value *getCurrentReturnValue();
};
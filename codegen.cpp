#include "node.hpp"
#include "codegen.hpp"
#include "parser.hpp"

llvm::BasicBlock *CodeGenContext::currentBlock()
{
    return blocks.top()->block;
}

void CodeGenContext::pushBlock(llvm::BasicBlock *block)
{
    blocks.push(new CodeGenBlock());
    blocks.top()->returnValue = NULL;
    blocks.top()->block = block;
}

void CodeGenContext::popBlock()
{
    CodeGenBlock *top = blocks.top();
    blocks.pop();
    delete top;
}

void CodeGenContext::setCurrentReturnValue(llvm::Value *value)
{
    blocks.top()->returnValue = value;
}

llvm::Value *CodeGenContext::getCurrentReturnValue()
{
    return blocks.top()->returnValue;
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock &root, bool print_bitcode, const char *bitcode_filename)
{
    std::cout << "Generating code...\n";

    /* Create the top level interpreter function to call as entry */
    std::vector<llvm::Type *> argTypes;
    llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(GlobalContext), llvm::makeArrayRef(argTypes), false);
    mainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::ExternalLinkage, "main", module);
    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(GlobalContext, "entry", mainFunction, 0);

    /* Push a new variable/block context */
    pushBlock(bblock);
    root.codeGen(*this); /* emit bytecode for the toplevel block */
    llvm::ReturnInst::Create(GlobalContext, bblock);
    popBlock();

    if (print_bitcode)
    {
        /* Print the bytecode in a human-readable format 
       to see if our program compiled properly
       */
        llvm::legacy::PassManager pm;
        pm.add(llvm::createPrintModulePass(llvm::outs()));
        pm.run(*module);
    }

    std::error_code EC;
    llvm::raw_fd_ostream OS(bitcode_filename, EC, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(*module, OS);
    OS.flush();
}

/* Executes the AST by running the main function */
llvm::GenericValue CodeGenContext::runCode()
{
    std::cout << "Running code...\n";
    llvm::EngineBuilder *eb = new llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module));
    llvm::ExecutionEngine *ee = eb->create();
    std::vector<llvm::GenericValue> noargs;
    llvm::GenericValue v = ee->runFunction(mainFunction, noargs);
    std::cout << "Code was run.\n";
    return v;
}

llvm::Function *CodeGenContext::createPrintfFunction()
{
    std::vector<llvm::Type *> printf_arg_types;
    printf_arg_types.push_back(llvm::Type::getInt8PtrTy(GlobalContext)); //char*

    llvm::FunctionType *printf_type =
        llvm::FunctionType::get(
            llvm::Type::getInt32Ty(GlobalContext), printf_arg_types, true);

    llvm::Function *func = llvm::Function::Create(
        printf_type, llvm::Function::ExternalLinkage,
        llvm::Twine("printf"),
        *module);
    func->setCallingConv(llvm::CallingConv::C);
    return func;
}
void CodeGenContext::createPrintFunction(llvm::Function *printfFn)
{
    std::vector<llvm::Type *> echo_arg_types;
    echo_arg_types.push_back(llvm::Type::getInt64Ty(GlobalContext));

    llvm::FunctionType *echo_type =
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(GlobalContext), echo_arg_types, false);

    llvm::Function *func = llvm::Function::Create(
        echo_type, llvm::Function::InternalLinkage,
        llvm::Twine("print"),
        *module);
    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(GlobalContext, "entry", func, 0);
    pushBlock(bblock);

    const char *constValue = "%d\n";
    llvm::Constant *format_const = llvm::ConstantDataArray::getString(GlobalContext, constValue);
    llvm::GlobalVariable *var =
        new llvm::GlobalVariable(
            *module, llvm::ArrayType::get(llvm::IntegerType::get(GlobalContext, 8), strlen(constValue) + 1),
            true, llvm::GlobalValue::PrivateLinkage, format_const, ".str");
    llvm::Constant *zero =
        llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(GlobalContext));

    std::vector<llvm::Constant *> indices;
    indices.push_back(zero);
    indices.push_back(zero);
    llvm::Constant *var_ref = llvm::ConstantExpr::getGetElementPtr(
        llvm::ArrayType::get(llvm::IntegerType::get(GlobalContext, 8), strlen(constValue) + 1),
        var, indices);

    std::vector<llvm::Value *> args;
    args.push_back(var_ref);

    llvm::Function::arg_iterator argsValues = func->arg_begin();
    llvm::Value *toPrint = &*argsValues++;
    toPrint->setName("toPrint");
    args.push_back(toPrint);

    llvm::CallInst *call = llvm::CallInst::Create(printfFn, makeArrayRef(args), "", bblock);
    llvm::ReturnInst::Create(GlobalContext, bblock);
    popBlock();
}

void CodeGenContext::createCoreFunctions()
{
    llvm::Function *printfFn = createPrintfFunction();
    createPrintFunction(printfFn);
}

/* Returns an LLVM type based on the identifier */
static llvm::Type *typeOf(const NIdentifier &type)
{
    if (type.name.compare("int") == 0)
    {
        return llvm::Type::getInt64Ty(GlobalContext);
    }
    else if (type.name.compare("double") == 0)
    {
        return llvm::Type::getDoubleTy(GlobalContext);
    }
    return llvm::Type::getVoidTy(GlobalContext);
}

/* -- Code Generation -- */

llvm::Value *NInteger::codeGen(CodeGenContext &context)
{
    std::cout << "Creating integer: " << value << std::endl;
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(GlobalContext), value, true);
}

llvm::Value *NDouble::codeGen(CodeGenContext &context)
{
    std::cout << "Creating double: " << value << std::endl;
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(GlobalContext), value);
}

llvm::Value *NPrintCall::codeGen(CodeGenContext &context)
{
    llvm::Function *function = context.module->getFunction(id.name.c_str());
    if (function == NULL)
    {
        std::cerr << "no such function " << id.name << std::endl;
    }
    std::vector<llvm::Value *> args;
    ExpressionList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++)
    {
        args.push_back((**it).codeGen(context));
    }
    llvm::CallInst *call = llvm::CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
    return call;
}

llvm::Value *NBinaryOperator::codeGen(CodeGenContext &context)
{
    llvm::Instruction::BinaryOps instr;
    switch (op)
    {
    case TPLUS:
        instr = llvm::Instruction::Add;
        break;
    case TMINUS:
        instr = llvm::Instruction::Sub;
        break;
    case TMUL:
        instr = llvm::Instruction::Mul;
        break;
    case TDIV:
        instr = llvm::Instruction::SDiv;
    }

    return llvm::BinaryOperator::Create(instr, lhs.codeGen(context),
                                        rhs.codeGen(context), "", context.currentBlock());
}

llvm::Value *NAssignment::codeGen(CodeGenContext &context)
{
    if (context.locals().find(lhs.name) == context.locals().end())
    {
        const NIdentifier &type = NIdentifier("int");
        llvm::AllocaInst *alloc = new llvm::AllocaInst(typeOf(type), 0, lhs.name.c_str(), context.currentBlock());
        context.locals()[lhs.name] = alloc;
    }
    return new llvm::StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

llvm::Value *NIdentifier::codeGen(CodeGenContext &context)
{
    std::cout << "Creating identifier reference: " << name << std::endl;
    if (context.locals().find(name) == context.locals().end())
    {
        std::cerr << "undeclared variable " << name << std::endl;
        exit(1);
    }
    return new llvm::LoadInst(context.locals()[name], "", false, context.currentBlock());

    // return context.locals()[name];
}

llvm::Value *NBlock::codeGen(CodeGenContext &context)
{
    StatementList::const_iterator it;
    llvm::Value *last = NULL;
    for (it = statements.begin(); it != statements.end(); it++)
    {
        std::cout << "Generating code for " << typeid(**it).name() << std::endl;
        last = (**it).codeGen(context);
    }
    std::cout << "Creating block" << std::endl;
    return last;
}
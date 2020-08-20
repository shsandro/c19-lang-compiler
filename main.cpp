#include <iostream>
#include <unistd.h>
#include "codegen.hpp"
#include "node.hpp"

extern int yyparse();
extern NBlock *programBlock;
extern FILE *yyin;

void help()
{
    std::cout << "c19 -i <input filename> -o <output filename> [-b] [-s] [-h]" << std::endl;
    std::cout << "  -b: print llvm bitcode" << std::endl;
    std::cout << "  -s: generate assembly file" << std::endl;
    std::cout << "  -h: display what you are seeing now" << std::endl;
}

int main(int argc, char *argv[])
{
    std::string input, output;
    bool print_bitcode = false;
    bool generate_assembly = false;

    FILE *fd;

    int opt;

    while ((opt = getopt(argc, argv, "i:o:bsh")) != -1)
        switch (opt)
        {
        case 'i':
            input = optarg;
            break;
        case 'o':
            output = optarg;
            break;
        case 'b':
            print_bitcode = true;
            break;
        case 's':
            generate_assembly = true;
            break;
        case 'h':
            help();
            exit(0);
        default:
            help();
            exit(1);
        }

    if (input.empty() || output.empty())
    {
        std::cout << "Error: an input file and an output filename are required." << std::endl;
        help();
        exit(1);
    }

    unsigned long pos = input.find(".");
    unsigned long len = input.length();
    std::string file_name, extension, bitcode_filename;

    if (pos < len)
    {
        file_name = input.substr(0, pos);
        extension = input.substr(pos + 1);
    }

    if (extension.compare("c19") != 0 || file_name.empty())
    {
        std::cout << "Error: wrong extension. Only c19 is accepted. Found " << extension << " file." << std::endl;
        exit(1);
    }

    bitcode_filename = input;
    bitcode_filename.replace(pos, len, ".bc");

    fd = fopen(input.c_str(), "r");
    if (fd)
        yyin = fd;
    else
    {
        std::cout << "Error: " << input << " file not found." << std::endl;
        return 1;
    }

    yyparse();                                                                    // invoca o analisador sintático
    llvm::InitializeNativeTarget();                                               // inicializa o native target para a máquina em questão
    llvm::InitializeNativeTargetAsmPrinter();                                     // inicializa o native target para impressão de assembly
    llvm::InitializeNativeTargetAsmParser();                                      // inicializa o native target para parser de assembly
    CodeGenContext context;                                                       // cria um contexto de geração de código
    context.createCoreFunctions();                                                // cria as funções nativas
    context.generateCode(*programBlock, print_bitcode, bitcode_filename.c_str()); // gera código intermediário (bytecodes)

    char *cmd;
    if (generate_assembly)
    {
        cmd = strdup("clang-10 -S ");
        strcat(cmd, bitcode_filename.c_str());
        strcat(cmd, " -o ");
        strcat(cmd, file_name.c_str());
        strcat(cmd, ".asm");
        system(cmd);
    }

    cmd = strdup("clang-10 ");
    strcat(cmd, bitcode_filename.c_str());
    strcat(cmd, " -o ");
    strcat(cmd, output.c_str());
    system(cmd);

    cmd = strdup("rm ");
    strcat(cmd, bitcode_filename.c_str());
    system(cmd);

    return 0;
}

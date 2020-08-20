# A Toy Compiler using LLVM Infrastructure

That is a toy compiler for C19 language - created only for this specific porpouse. Developed to learn more about the LLVM Infrastructure and front-end compiler. The development was based on [Loren's tutorial](https://github.com/lsegal/my_toy_compiler). You are invited to explore and learn more about LLVM Infrastructure!

## Dependencies

- `clang++ 10`
- `clang 10`
- `llvm 10`

To compile we need `clang++-10`, and when the compiler runs, we need `clang-10` to compile the llvm bitcode. So if your `clang` links differ from these, I suggest you to creat symbolic links for them.

## C19 Language

The grammar of the language is very simple, as follow:

```
program: statments

statments: statment
           | statments statment

statment: assignment
          | print

assignment: identifier = expression;

print: print(expression);

expression: expression + expression
            | expression - expression
            | expression * expression
            | expression / expression
            | (expression)
            | identifier
            | number

```

## Usage

### Compile

- `make`

### Run

- `./c19 -i <input c19 file> -o <output file name> [-b] [-s] [-h]`

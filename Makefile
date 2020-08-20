all: c19

CXX = clang++-10

OBJS = parser.o  \
       codegen.o \
       main.o    \
       lexer.o  \

LLVMCONFIG = llvm-config-10
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++17 -g
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`

clean:
	$(RM) -rf parser.cpp parser.hpp lexer.cpp parser.tab.* $(OBJS) *.out

parser.cpp: parser.y
	bison -d -o $@ $^
	
parser.hpp: parser.cpp

lexer.cpp: lexer.l parser.hpp
	flex -o $@ $^

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<


c19: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

test: c19 example.c19
	./c19 -i example.c19 -o example.out -b
	./example.out
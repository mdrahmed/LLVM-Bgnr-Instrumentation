# llvm-insert-printf

Inserting printf function to a demo file using llvm pass. 

## Build
- [Clone my repo inside LLVM-project](https://github.com/MdRaihanAhmed-coder/LLVM-Clang-Instrumentation.git)
- Build your llvm again with `make or ninja`
- Create a C file,
```
#include <stdio.h>
#include<stdio.h>

int foo(int a) {
  return a * 2;
}

int bar(int a, int b) {
  return (a + foo(b) * 2);
}

int fez(int a, int b, int c) {
  return (a + bar(a, b) * 2 + c * 3);
}

int main()
{
        int a,b;
        a=10;
        b=5;
        return 0;
}

``` 
- run 
- `opt -load <llvm path>/build/lib/LLVMmodule_dir.so -enable-new-pm=0 -legacy-inject-func-call bb.ll -o bb.bin`
- `<path>/lli bb.bin`


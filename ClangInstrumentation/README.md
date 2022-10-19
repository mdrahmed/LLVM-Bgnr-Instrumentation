# LLVM-Clang-Instrumentation
A simple example of instrumentation for C using LLVM and Clang 14. LLVM Pass is used to implement the instrumentation. In this project, we use replaced addition with multiplication. For example, the code in test/example.c is as follows.


## Build
- [Clone my repo inside LLVM-project](https://github.com/MdRaihanAhmed-coder/LLVM-Clang-Instrumentation.git)
- Build your llvm again with `make or ninja`
- Create a C file,
```
#include <stdio.h>
int main(int argc, const char** argv) {
    int num;
    scanf("%i", &num);
    printf("%i\n", num + 2);
    return 0;
}
``` 
- run 
`clang -flegacy-pass-manager -Xclang -load -Xclang ../build/lib/libModifyPass.so example.c -o example`
- then`./example`

## Reference
- https://github.com/sampsyo/llvm-pass-skeleton
- https://github.com/LuoRongLuoRong/llvm_clang_instrument_example

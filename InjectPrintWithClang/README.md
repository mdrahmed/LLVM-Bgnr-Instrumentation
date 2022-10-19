# LLVM-InjectPrint-clang
Injecting printf to a C or, CPP file using LLVM pass and running it with Clang

# Introduction
In the InjectPrintf.so.cc this pass is injecting printf function into a C/CPP file. This file after compiled with necessary options, can insert printf in each and every basic block of the code.

##Build
 - Create C or, CPP files like example.c or, example.cpp
 - Flags
```
export CXXFLAGS="-O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DVERSION=1  -Wno-variadic-macros"
export CLANG_CFL="`llvm-config-14 --cxxflags` -Wl,-znodelete -fno-rtti -fpic $CXXFLAGS"
export CLANG_LFL=`llvm-config-14 --ldflags --libs`
```
 - Then create the pass
```
clang++-14  $CLANG_CFL  -shared injectPrintf.so.cc -o printf.so $CLANG_LFL
```
 - Now run the C file with pass,
 ```
 clang-14 -flegacy-pass-manager -g -Xclang -load -Xclang ./printf.so example.c -o ex1
 ```
 -  Now run the CPP file with pass,
 ```
 clang++-14 -flegacy-pass-manager -g -Xclang -load -Xclang ./printf.so example.cpp -o ex2
 ```
Now, run that binary using `./ex1` or, `ex2`, it will print `test` which is not printed in the C/CPP file.

 - Build binary for arm32, simply add this pass while compiling,
```
clang++-14 --target=arm-linux-gnueabihf -flegacy-pass-manager -g -Xclang -load -Xclang ./printf.so example.cpp -o ex2
```

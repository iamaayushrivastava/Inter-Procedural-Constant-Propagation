# Inter Procedural Constant Propagation
This repository implements an inter-procedural, context-insensitive, and flow-sensitive constant propagation analysis and transformation pass. It optimizes code by identifying and substituting constant values across function boundaries.

## Overview

This repository contains an implementation of an inter-procedural, context-insensitive, and flow-sensitive constant propagation optimization pass for the LLVM compiler framework. Constant propagation is an optimization technique used in compilers to replace variables with constant values with the constants themselves at compile-time, simplifying expressions and potentially improving performance.

## Directory Structure

```
InterProcConstProp
├── assign
│   └── IR
│       └── public_test_cases.ll
├── output
│   └── IR
│       └── transformed_test_cases.ll
└── constprop.cpp
```

- **assign**: Contains the input IR (Intermediate Representation) code files for public test cases.
- **output**: Will contain the transformed IR code files after running the optimization pass.
- **constprop.cpp**: The main implementation file for the constant propagation pass.

## Implementation Details

### Features

- **Inter-Procedural Analysis**: Analyzes across function boundaries to propagate constants.
- **Context-Insensitive**: Does not distinguish between different call contexts.
- **Flow-Sensitive**: Takes into account the control flow to determine possible constant values.

### Example

#### Input
```c
void main() {
    int l = 40;
    fun(10, 100, 1000);
    bar(1000, 400, 300);
    foo(100, 400, 1000, l);
}

void fun(int i, int j, int k) {
    bar(2000, i, 1000);
    int o = i * 2;
    int q = 2;
    int a = q * o * i;
    foo(100, a, k, 800);
}

void foo(int i, int j, int k, int x) {
    x = i + j + k;
    printf("%d", x);
}

void bar(int i, int j, int k) {
    j = (k * i) / 2;
    printf("%d", j);
}
```

#### Output
```c
void main() {
    fun(10, 100, 1000);
    bar(1000, 400, 300);
    foo(100, 400, 1000, 40);
}

void fun(int i, int j, int k) {
    bar(2000, 10, 1000);
    foo(100, 400, 1000, 800);
}

void foo(int i, int j, int k, int x) {
    printf("%d", 1500);
}

void bar(int i, int j, int k) {
    j = (k * i) / 2;
    printf("%d", j);
}
```

## Getting Started

### Prerequisites

- LLVM installed and configured
- C++ compiler supporting C++11 or later
- Basic understanding of LLVM IR and compiler optimization techniques

## Hints

- Familiarize yourself with C++ STL containers such as `std::set`, `std::map`, and `std::vector`.

- **Test Case Variations**:
  - Data Types: Boolean, integer, float
  - Operations: Arithmetic, logical
  - Instructions: Cast, load, store

- **Exclusions**:
  - No need to handle global variables and macros.
  - Avoid using `mem2reg` pass or the built-in LLVM constant propagation pass.

## Debugging

If the provided test case files contain debug information, consider building your LLVM in debug mode to utilize this information effectively.

## Conclusion

Implementing an inter-procedural constant propagation pass involves analyzing and transforming IR code to replace variables with constant values wherever applicable. By carefully considering the flow of values across function calls, this optimization can lead to more efficient generated code.

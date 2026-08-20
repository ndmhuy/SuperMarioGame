# `nn::` — vendored C++ deep learning framework

Source: **CS200-Cpp** (`/Users/huynguyen/Documents/CS200-Cpp`), the author's
own from-scratch deep learning library. Vendored here, not referenced by path.

## Why vendored rather than a path dependency

A `find_package`/relative-path dependency on a sibling checkout breaks the
build on any machine that does not happen to have `CS200-Cpp` cloned next to
`SuperMarioGame` — a grader's machine, CI, or the other team member's laptop.
`AGENTS.md`'s build directive ("`mkdir build && cd build && cmake .. && make`")
implies the repository is self-contained, so the framework is copied in.

## What was copied

- `include/nn/**` — all headers **except** `Compute/MetalBackend.hpp`.
- `src/{Tensor,Autograd,Module,Optim,Loss,Core,Data,Training}/*.cpp`
- `src/Compute/CPUBackend.cpp`

Deliberately **not** copied: `MetalBackend.mm`, `*.metal` shaders, and the
upstream demo programs (`main.cpp`, `vae.cpp`, `poker_rl.cpp`, …).

## The one patch

`src/Core/Device.cpp` — upstream includes `MetalBackend.hpp` and constructs a
`MetalBackend` for `Device::metal()`. Since the Metal backend is not vendored,
that include is removed and `MetalDevice` is patched to return a `CPUBackend`.
The patch is commented in-file so a re-sync from upstream does not silently
lose it.

**Why CPU-only is the right call here, not a compromise:** the policy network
is ~123K parameters (1899-wide observation × {64,32} hidden × 7 outputs). That
is small enough for the NEON SIMD CPU backend to do a training step in
microseconds, and vendoring Metal would add an Objective-C++ language
requirement to this project's CMake for no measured benefit.

## C++ standard

The framework requires **C++20** (`Tensor.hpp` and `Expression.hpp` use
concepts). SuperMarioGame is **C++17** and `AGENTS.md` requires it to stay
that way.

These coexist: the `nn` static library target is compiled as C++20 in
isolation, and the only game translation unit that includes `nn/` headers
(`src/Entities/NeuralPolicy.cpp`) is compiled as C++20 via a per-source
property. `NeuralPolicy.hpp` uses the PIMPL idiom so that **no `nn/` header is
ever visible to C++17 game code** — every other file in the game continues to
compile as C++17 and never sees a concept.

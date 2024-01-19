Stanford CS 144 Networking Lab
==============================

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`

- [x] Checkpoint 0: networking warmup

[![check0.png](https://s11.ax1x.com/2024/01/20/pFEUeMD.png)](https://imgse.com/i/pFEUeMD)

- [ ] Checkpoint 1: stitching substrings into a byte stream
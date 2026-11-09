# cross-platform-actions-check

C++23 / CMake / clang project whose cross-platform support is verified by GitHub Actions
on every pull request. The demo app (`src/main.cpp`) exercises `std::expected` and
`std::filesystem`; it exits non-zero if any check fails.

| Workflow | Runner | Toolchain |
|----------|--------|-----------|
| `linux.yml` | `ubuntu-24.04` | clang 18 + libc++ 18 |
| `macos.yml` | `macos-15` | Homebrew LLVM (clang + libc++) |
| `windows.yml` | `windows-2022` | clang-cl (VS 2022 ClangCL toolset) + MSVC STL |

## Local build

```bash
CXX=clang++-18 cmake --preset linux      # or: macos / windows
cmake --build --preset linux
ctest --preset linux
```

On Linux the preset uses libc++, because libstdc++ 13 does not expose `<expected>` to clang 18.

# CNA Samples

C++ ports of the official **Microsoft XNA Game Studio 4.0** sample collection,
running on [CNA](https://github.com/openeggbert/cna) — a C++ reimplementation of
the XNA 4.0 programming model built on SDL3.

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.20 |
| C++ compiler | C++23 (GCC 13+, Clang 16+, MSVC 19.38+) |
| CNA | sibling directory `../cna` |
| sharp-runtime | sibling directory `../sharp-runtime` |

Clone all three side-by-side:

```
openeggbert/
├── cna/
├── sharp-runtime/
└── cna-samples/       ← this repo
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Run a sample:

```bash
./build/samples/PrimitivesSample/cna_sample_primitives
```

## Samples

See [PLAN.md](PLAN.md) for the full inventory and migration roadmap.

## License

Microsoft Permissive License (Ms-PL) — see [LICENSE](LICENSE).
Derived from the XNA Game Studio 4.0 sample collection © Microsoft Corporation.

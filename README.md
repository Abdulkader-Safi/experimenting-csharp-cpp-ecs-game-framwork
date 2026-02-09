# C++ / C# Bridge with Mono

A minimal example of calling C++ functions from C# using Mono's P/Invoke mechanism.

## How It Works

1. **C++ shared library** (`native/hello.cpp`) — Exports C-linkage functions (`add`, `greet`)
2. **C# program** (`managed/Program.cs`) — Uses `[DllImport("hello")]` to call those functions at runtime
3. **Mono runtime** — Loads the `.dylib` and marshals calls between managed and native code

## Prerequisites

- **Mono** (provides `mcs` and `mono`)
- **g++** (C++ compiler)
- **make**

## Quick Start

```sh
make run
```

Expected output:

```
Hello from C++, Safi!
3 + 4 = 7
```

## Make Targets

| Target  | Description                                   |
| ------- | --------------------------------------------- |
| `all`   | Build native `.dylib` and C# `.exe` (default) |
| `run`   | Build and run the program with Mono           |
| `clean` | Remove build artifacts                        |
| `help`  | Show available targets                        |

## Project Structure

```
.
├── Makefile
├── native/
│   └── hello.cpp          # C++ shared library source
├── managed/
│   └── Program.cs         # C# program source
└── build/                 # Generated build artifacts
    ├── libhello.dylib
    └── Program.exe
```

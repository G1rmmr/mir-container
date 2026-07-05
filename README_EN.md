# zet

> C++20 Standard Container & Memory Library (ZET - Zero-allocated Execution Toolkit)
> 
> This library is a collection of high-performance, lightweight custom data structures and custom memory management utilities. It is designed to satisfy the ZET specifications for zero runtime heap allocations. It offers both the flexibility of a header-only design and the robustness of static library (.a/.lib) compilation.

---

## Key Features

* **Zero Runtime Allocation**: Designed for safety and speed by avoiding arbitrary runtime heap allocations.
* **C++20 Standard Compliance**: Type-safe template constraints using concepts and requires clauses.
* **Lightweight Design**: Minimized dependencies, maximizing memory and allocation efficiency.
* **Xmake System Integration**: Fully automated builds, packaging, and unit testing via the xmake build system.
* **Cross-Platform Validation**: Verified to compile and run seamlessly on Windows (MSVC), Linux (GCC), and macOS (Clang).

---

## Component List

### 1. Containers
* **List.hpp**: Array-based dynamic list supporting static allocation constraints and destructor safety.
* **String.hpp**: Fixed-size buffer optimized string class with implicit conversion to `std::string_view` and concatenation support.
* **Map.hpp**: High-speed key-value hash map using `std::hash` with duplicate key handling and iteration support.
* **Pool.hpp**: High-performance resource pool featuring generation-based dangling handle safety and object reuse.
* **SparseSet.hpp**: Sparse set optimized for entity/component management using swap-on-remove dense packing and a **Paged Architecture** to completely prevent memory explosion at massive entity scale.
* **CommandBuffer.hpp**: High-throughput non-owning deferred command buffer for executing bulk operations.

### 2. Memory Allocators
* **LinearAllocator.hpp**: Extremely fast arena allocator that resets all memory at once. It supports registration of object destructors to prevent memory leaks on non-trivial types.
* **StackAllocator.hpp**: LIFO-style stack allocator that allows allocating and deallocating back to a specific marker.
* **PointerHandle.hpp**: A non-owning safety handle (`PointerHandle<T>`) used to safely reference memory blocks inside allocators using offsets rather than raw pointers, making it safe against memory compaction or relocation.

---

## 1. Local Build and Unit Testing

This project integrates doctest, a lightweight C++ unit testing framework, to provide 100% functional test coverage.

### Build and Run Commands

```bash
# 1. Configure the project in debug mode (doctest package will be installed automatically)
xmake config --mode=debug --yes

# 2. Compile the static library and test targets
xmake

# 3. Run all unit tests
xmake test

# 4. Change configuration to release mode for optimized builds
xmake config --mode=release
xmake
```

### Editor Integration (Sublime Text, VS Code, etc.)
The project includes automated compilation database generation (compile_commands.json) integrated into the build rules. This ensures that language servers (like LSP-clangd) correctly parse C++20 features (like requires clauses) and prevent false error highlights. It automatically updates whenever you run `xmake`.

* Manual generation: `xmake project -k compile_commands`

---

## 2. Using the Library in Other Xmake Projects

Here are two modern methods to integrate the `zet` static library into another xmake project.

### Method A: Subproject Integration (Highly Recommended)
Place the `zet` repository as a subdirectory or a Git submodule inside your consumer project, and add the following lines to your consumer's `xmake.lua` file:

```lua
-- 1. Include the zet project configuration
includes("path/to/zet")

target("my_application")
    set_kind("binary")
    add_files("src/*.cpp")
    
    -- 2. Link the dependency
    -- (The include header search paths and static library links will be propagated automatically!)
    add_deps("zet")
```

---

### Method B: GitHub Repository Fetching (GitHub Package)
Fetch and install the library as a remote package directly from the GitHub repository, similar to npm or pip.

Add the following to your consumer's `xmake.lua` file:

```lua
-- 1. Register and request the remote package from the GitHub repository
add_requires("zet", {
    alias = "zet",
    url = "https://github.com/G1rmmr/zet.git",
    on_install = function (package)
        -- Automated, silent build optimized for the user's OS and compiler
        import("package.tools.xmake").install(package)
    end
})

-- 2. Bind the package to your target
target("my_application")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("zet")
```

---

## 3. CI/CD Pipeline (GitHub Actions)

The repository runs a GitHub Actions workflow to validate code health across platforms on every push and pull request.

* Platforms: ubuntu-latest, macos-latest, windows-latest
* Build Caching: Integrated with actions/cache to speed up compilation times by 80%.
* Automation: Performs static library archiving and runs all unit tests automatically.

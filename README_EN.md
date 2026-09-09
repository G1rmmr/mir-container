# zet

> C++20 Standard Container & Memory Library (ZET - Zero-allocated Execution Toolkit)
> 
> ZET is a C++20 container and memory toolkit that never allocates heap storage internally. Capacity is fixed at compile time or supplied explicitly by the caller as an external buffer. ZET performs no hidden growth and has no heap fallback.

---

## Key Features

* **No Library-owned Allocation**: ZET does not obtain storage with `new`, `delete`, `malloc`, or `free`.
* **Explicit Capacity**: Storage is fixed or caller-provided and never grows implicitly.
* **Predictable Failure**: Checked `Try...` APIs return a null pointer, `false`, or `Status` without modifying the container on capacity or validation failure.
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
* **SparseSet.hpp**: Fully fixed-capacity sparse set with dense packing and no runtime page allocation.
* **CommandBuffer.hpp**: High-throughput non-owning deferred command buffer for executing bulk operations.
* **BitSet.hpp**: Fixed bit set for masks, membership tests, and graph traversal state.
* **RingBuffer.hpp / Deque.hpp / SpscQueue.hpp**: Bounded FIFO, deque, and single-producer/single-consumer queues.
* **PriorityQueue.hpp**: Fixed-capacity binary heap for scheduling and path finding.
* **FlatMap.hpp**: Sorted fixed-capacity map and set for cache-friendly ordered lookup.
* **Hierarchy.hpp**: Generation-handle parent/child hierarchy with iterative traversal scratch buffers.
* **FixedGraph.hpp / StaticGraph.hpp**: Mutable handle-based graph plus a build-once CSR-style graph.

### 2. Memory Allocators
* **LinearAllocator.hpp**: Operates exclusively on a caller-provided buffer and reuses it on reset.
* **StackAllocator.hpp**: Provides LIFO allocation and marker rewind over a caller-provided buffer.
* **PointerHandle.hpp**: Stores an allocator-relative offset and epoch. Handles become invalid after reset or rewind.

```cpp
alignas(64) std::array<std::byte, 1024 * 1024> memory{};
zet::memory::LinearAllocator arena(memory);
auto value = arena.CreateHandle<int>(42);
```

Allocations performed by user-provided value types or callbacks are outside ZET's guarantee. For example, `List<std::string, 16>` has fixed container storage, but `std::string` may allocate internally.

### Safety contract

Use `Try...` APIs when invalid input or a full container is expected. The convenience APIs (`Push`, `Insert`, `Get`) retain assertion checks and terminate rather than returning a fabricated reference in release builds. `PoolHandle` records its owning pool, and allocator handles require the allocator and its external buffer to outlive the handle. Graph and hierarchy traversal/path APIs receive caller-owned scratch storage, so traversal does not allocate.

---

## 1. Local Build and Unit Testing

This project uses doctest for behavior tests and allocation-count checks on core paths.

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

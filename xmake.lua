---@diagnostic disable: undefined-global
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })

if is_plat("windows") then
    add_cxflags("/utf-8", { tools = { "clang_cl", "cl" } })
end

-- Add doctest package
add_requires("doctest")

-- Static Library Target
target("zet")
set_kind("static")
set_languages("c++20")
-- Export current directory and subdirectories so dependency targets can automatically include our headers
add_includedirs("src", "src/container", "src/memory", { public = true })
add_headerfiles("src/(**.hpp)", {rootdir = "src"})
add_files("src/*.cpp")

-- Unit Test Target
target("test_suite")
set_kind("binary")
set_languages("c++20")
add_packages("doctest")
add_files("src/tests/*.cpp")
add_deps("zet")

-- Register this binary as a test suite so xmake test can execute it
add_tests("default")

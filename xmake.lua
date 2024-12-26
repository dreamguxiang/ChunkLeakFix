add_rules("mode.debug", "mode.release")

add_repositories("levilamina https://github.com/LiteLDev/xmake-repo.git")
package("endstone")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/EndstoneMC/endstone")
    set_description("Endstone - High-level Plugin API for Bedrock Dedicated Servers (BDS), in both Python and C++")
    set_license("Apache-2.0")

    add_urls("https://github.com/EndstoneMC/endstone/archive/refs/tags/v$(version).tar.gz","https://github.com/EndstoneMC/endstone.git")
    add_versions("0.5.7.1","2020dafeeeaa7fd1393301489dbc8146e43483779852d8e36c7754f82691778b")
    add_patches("0.5.7.1", "https://raw.githubusercontent.com/engsr6982/Js_Engine/refs/heads/develop/patch/cxx20.patch",
                        "547ae3d325b8deb68747179b6bc3aa8772ba4efe36263bf31f34be7a3aac2ceb")

    on_install("windows", "linux", function (package)
        os.cp("include", package:installdir())
    end)
package_end()

add_requires(
    "expected-lite 0.8.0",
    "entt 3.14.0",
    "microsoft-gsl 4.0.0",
    "nlohmann_json 3.11.3",
    "boost 1.85.0",
    "glm 1.0.1",
    "concurrentqueue 1.0.4",
    "endstone 0.5.7.1",
    "magic_enum 0.9.7",
    "fmt >=10.0.0 <11.0.0",
    "lighthook"
)

target("ChunkLeakFix")
    set_kind("shared")
    add_defines(
        "NOMINMAX",
        "UNICODE",
        "_AMD64_",
        "USE_LIGHTHOOK"
    )
    add_files("src/**.cpp")
    add_includedirs("src")
    add_packages(
        "fmt",
        "expected-lite",
        "entt",
        "microsoft-gsl",
        "nlohmann_json",
        "boost",
        "glm",
        "concurrentqueue",
        "endstone",
        "magic_enum",
        "lighthook"
    )  
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    set_languages("c++20")
    set_symbols("debug")
    add_defines("ENTT_SPARSE_PAGE=2048")
    add_defines("ENTT_PACKED_PAGE=128")
    set_exceptions("none")
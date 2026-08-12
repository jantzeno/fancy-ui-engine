local repo_dir = workspace_repo_dir("fancy-ui-engine")
local workspace_dir = path.directory(repo_dir)

target("fancy_ui")
    set_kind("static")
    set_default(false)
    set_warnings("all")
    add_files(path.join(repo_dir, "src", "**.cpp"))
    add_includedirs(path.join(repo_dir, "include"), {public = true})
    add_includedirs(path.join(repo_dir, "src"))
    add_deps("im2d_ui", "workspace_vendor_lunasvg")
    add_packages("imgui")
    if is_plat("windows") then
        add_syslinks("user32")
    elseif is_plat("macosx") then
        add_frameworks("CoreText", "CoreFoundation")
    end

target("fancy_ui_tests")
    set_kind("binary")
    set_default(false)
    set_warnings("all")
    add_files(path.join(repo_dir, "tests", "test_*.cpp"))
    add_includedirs(path.join(repo_dir, "tools", "component_gallery"))
    add_includedirs(path.join(repo_dir, "src"))
    add_defines('FANCY_UI_TEST_SOURCE_ROOT="' .. repo_dir .. '"')
    add_deps("fancy_ui")
    add_packages("catch2", "imgui")

target("fancy_ui_component_gallery")
    set_kind("binary")
    set_default(false)
    set_warnings("all")
    set_rundir(repo_dir)
    add_files(path.join(repo_dir, "tools", "component_gallery", "*.cpp"))
    add_includedirs(path.join(repo_dir, "tools", "component_gallery"))
    add_includedirs(path.join(repo_dir, "src"))
    add_includedirs(
        path.join(workspace_root, "engine_vendor", "nanosvg", "example"))
    add_defines(
        'FANCY_UI_GALLERY_ASSET_ROOT="' ..
        path.join(repo_dir, "assets", "ui") .. '"')
    on_load(function(target)
        local output_dir = path.join(target:autogendir(), "panel-audits")
        target:add("includedirs", output_dir)
    end)
    before_build(function(target)
        local output_dir = path.join(target:autogendir(), "panel-audits")
        local script = path.join(
            workspace_dir, "tools", "ui-mockups", "native_panel_contracts.js")
        os.mkdir(output_dir)
        os.vrunv("deno", {
            "run",
            "--allow-read=" .. workspace_dir,
            "--allow-write=" .. output_dir,
            script,
            output_dir
        })
    end)
    add_deps("fancy_ui")
    add_packages("libsdl3", "glad", "imgui")

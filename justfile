prefix := "/"
working_dir := justfile_directory()
zpp_lib_dir := "deps/zpp_lib"
default_board := "nrf5340dk/nrf5340/cpuapp"

# CREATE A NEW APPLICATION FROM github template
create-app app:
    python {{zpp_lib_dir}}/scripts/create_app.py {{app}}

# BUILDS FOR THE APPLICATIONS RUNNING ON CONFIGURED REAL HARDWARE
build app specs="" pristine="yes":
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --specs {{quote(specs)}} --board {{default_board}} {{ if pristine == "yes" { "--pristine" } else { "" } }}

# QEMU BUILDS
build-qemu app specs="" pristine="yes":
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --specs {{quote(specs)}} --board "qemu_x86" {{ if pristine == "yes" { "--pristine" } else { "" } }}
    
# TESTS ON HARDWARE
test test_suite_root map_file tags="":
    python {{zpp_lib_dir}}/scripts/twister.py --root {{test_suite_root}} --tags {{quote(tags)}} --board {{default_board}} --map-file {{map_file}}

# TESTS ON QEMU
test-qemu test_suite_root tags="":
    python {{zpp_lib_dir}}/scripts/twister.py --root {{test_suite_root}} --tags {{quote(tags)}} --board "qemu_x86"
    
# CLANG-TIDY
clang-tidy app specs="":    
    # Step 1 — build to get compile_commands.json (build with all conf files to get the most complete database)
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --specs {{quote(specs)}} --board "native_sim" --pristine
    
    # Step 2 — filter the compile_commands.json file for compatibility with clang-tidy
    mkdir -p build_clang
    python3 {{zpp_lib_dir}}/scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

    # Step 3 — run clang-tidy against the filtered database
    clang-tidy-22 -p build_clang {{working_dir}}/{{app}}/src/main.cpp --extra-arg=-v    

run-clang-tidy app specs="":
    python {{zpp_lib_dir}}/scripts/run_clang_tidy.py --app {{app}} --specs {{quote(specs)}} --wd {{working_dir}}
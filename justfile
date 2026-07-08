prefix := "/"
working_dir := justfile_directory()
zpp_lib_dir := "deps/zpp_lib"
default_board := "nrf5340dk/nrf5340/cpuapp"
default_shield := "adafruit_2_8_tft_touch_v2"

# CREATE A NEW APPLICATION FROM github template
create-app app:
    python {{zpp_lib_dir}}/scripts/create_app.py {{app}}

# BUILD ALLS

# Build all applications as described in the configuration file
build-all yaml_file="ci/applications_for_build.yaml":
    python {{zpp_lib_dir}}/scripts/build_from_yaml.py --yaml {{quote(yaml_file)}}

# BUILDS FOR THE APPLICATIONS RUNNING ON CONFIGURED REAL HARDWARE

# Build the specified application with all specs described in the configuration file, for the default board
build-yaml app yaml_file="ci/applications_for_build.yaml":
    python {{zpp_lib_dir}}/scripts/build_from_yaml.py --yaml {{quote(yaml_file)}} --app {{app}} --board {{default_board}}

# Build the specified application with the specified configs, for the default board
build app configs pristine="yes" app_config="":
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --configs {{quote(configs)}} --board {{default_board}} \
    --shield {{default_shield}} {{ if pristine == "yes" { "--pristine" } else { "" } }} \
    {{ if app_config != "" { "--app-config " + quote(app_config) } else { "" } }}

# QEMU BUILDS
# Build the specified application with all configs described in the configuration file, for qemu_x86
build-qemu-yaml app yaml_file="ci/applications_for_build.yaml":
    python {{zpp_lib_dir}}/scripts/build_from_yaml.py --yaml {{quote(yaml_file)}} --app {{app}} --board "qemu_x86"

# Build the specified application with the specified configs, for qemu_x86
build-qemu app configs pristine="yes":
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --configs {{quote(configs)}} --board "qemu_x86" {{ if pristine == "yes" { "--pristine" } else { "" } }}

# TESTS ON HARDWARE
# Launch all tests as described in the configuration file, for the default board
test-all map_file yaml_file="ci/applications_for_test.yaml":
    python {{zpp_lib_dir}}/scripts/twister_from_yaml.py --yaml {{quote(yaml_file)}} --board {{default_board}} --map-file {{map_file}}

# Launch tests at the specified root, using hardware specified in the map file
test test_suite_root map_file tags="":
    python {{zpp_lib_dir}}/scripts/twister.py --root {{test_suite_root}} --tags {{quote(tags)}} --board {{default_board}} --map-file {{map_file}}

# TESTS ON QEMU
# Launch all tests as described in the configuration file, using qemu_x86
test-all-qemu yaml_file="ci/applications_for_test.yaml":
    python {{zpp_lib_dir}}/scripts/twister_from_yaml.py --yaml {{quote(yaml_file)}} --board "qemu_x86"

# Launch tests at the specified root, using qemu_x86
test-qemu test_suite_root tags="":
    python {{zpp_lib_dir}}/scripts/twister.py --root {{test_suite_root}} --tags {{quote(tags)}} --board "qemu_x86"
    
# CLANG-TIDY
# Check only the main.cpp file of the application
clang-tidy app configs:    
    # Step 1 — build to get compile_commands.json (build with all conf files to get the most complete database)
    python {{zpp_lib_dir}}/scripts/build.py --app {{app}} --configs {{quote(configs)}} --board "native_sim" --pristine
    
    # Step 2 — filter the compile_commands.json file for compatibility with clang-tidy
    mkdir -p build_clang
    python3 {{zpp_lib_dir}}/scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

    # Step 3 — run clang-tidy against the filtered database
    clang-tidy-22 -p build_clang {{working_dir}}/{{app}}/src/main.cpp --extra-arg=-v    

# Check all application files
run-clang-tidy app configs:
    python {{zpp_lib_dir}}/scripts/run_clang_tidy.py --app {{app}} --configs {{quote(configs)}} --wd {{working_dir}}
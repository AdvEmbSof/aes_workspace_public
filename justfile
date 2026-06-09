set shell := ["bash", "-cxu"]
prefix := "/"
working_dir := justfile_directory()

# BUILDS FOR THE APPLICATIONS RUNNING ON CONFIGURED REAL HARDWARE
build app spec="" clean="yes":
    #!/usr/bin/env bash
    echo {{app}} {{spec}} {{clean}}
    extra_confs=()
    build_args=(build {{app}} --shield adafruit_2_8_tft_touch_v2)
    if [[ "{{clean}}" != "no" ]]; then build_args+=(--pristine); fi
    IFS='+,' read -r -a spec_names <<< "{{spec}}"
    for spec_name in "${spec_names[@]}"; do [[ -n "$spec_name" ]] || continue; extra_confs+=(--extra-conf "prj_${spec_name}.conf"); done
    printf '%q ' west "${build_args[@]}" "${extra_confs[@]}"; printf '\n'
    west "${build_args[@]}" "${extra_confs[@]}"
    
# QEMU BUILDS
build-qemu app spec="" clean="yes":
    #!/usr/bin/env bash
    echo {{app}} {{spec}} {{clean}}
    extra_confs=()
    build_args=(build -b qemu_x86 {{app}})
    if [[ "{{clean}}" != "no" ]]; then build_args+=(--pristine); fi
    IFS='+,' read -r -a spec_names <<< "{{spec}}"
    for spec_name in "${spec_names[@]}"; do [[ -n "$spec_name" ]] || continue; extra_confs+=(--extra-conf "prj_${spec_name}.conf"); done
    printf '%q ' west "${build_args[@]}" "${extra_confs[@]}"; printf '\n'
    west "${build_args[@]}" "${extra_confs[@]}"
    
# TESTS WITH TWISTER
test app="bike_computer" tags="":
    #!/usr/bin/env bash
    echo {{app}} {{tags}}
    IFS='+,' read -r -a tag_names <<< "{{tags}}"
    for tag_name in "${tag_names[@]}"; do [[ -n "$tag_name" ]] || continue; extra_tags+=(--tag "${tag_name}"); done
    printf '%q ' west twister -T {{app}}/tests "${extra_tags[@]}" "--device-testing" "--hardware-map" "nrf5340_map_mint.yaml"; printf '\n'
    west twister -T {{app}}/tests "${extra_tags[@]}" --device-testing --hardware-map nrf5340_map_mint.yaml

test-qemu app="bike_computer" tags="":
    #!/usr/bin/env bash
    echo {{app}} {{tags}}
    IFS='+,' read -r -a tag_names <<< "{{tags}}"
    for tag_name in "${tag_names[@]}"; do [[ -n "$tag_name" ]] || continue; extra_tags+=(--tag "${tag_name}"); done
    printf '%q ' west twister -p qemu_x86 -T {{app}}/tests "${extra_tags[@]}"; printf '\n'
    west twister -p qemu_x86 -T {{app}}/tests "${extra_tags[@]}"

# CLANG-TIDY
clang-tidy app="bike_computer":
    # Step 1 — build to get compile_commands.json (build with all conf files to get the most complete database)
    west build -b native_sim {{app}} --pristine --extra-conf prj_debug.conf --extra-conf prj_log.conf -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    # Step 2 — filter the compile_commands.json file for compatibility with clang-tidy
    mkdir -p build_clang
    python3 scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

    # Step 3 — run clang-tidy against the filtered database
    clang-tidy -p build_clang {{working_dir}}/{{app}}/src/main.cpp --extra-arg=-v    

run-clang-tidy app="bike_computer":
    # Step 1 — build to get compile_commands.json
    west build -b native_sim {{app}} --pristine --extra-conf prj_debug.conf --extra-conf prj_log.conf -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    # Step 2 — filter the compile_commands.json file for compatibility with clang-tidy
    mkdir -p build_clang
    python3 scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

    # Step 3 — run clang-tidy against the filtered database
    run-clang-tidy -p build_clang "{{working_dir}}/{{app}}/src/.*\\.cpp$"
    run-clang-tidy -p build_clang "{{working_dir}}/deps/zpp_lib/.*\\.cpp$"

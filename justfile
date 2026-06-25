prefix := "/"
working_dir := justfile_directory()

# CREATE A NEW APPLICATION FROM github template
create-app app:
    python scripts/create_app.py {{app}}

# BUILDS FOR THE APPLICATIONS RUNNING ON CONFIGURED REAL HARDWARE
build app spec="" clean="yes":
    python scripts/build.py {{app}} {{quote(spec)}} {{clean}} 

# QEMU BUILDS
build-qemu app spec="" clean="yes":
    python scripts/build.py {{app}} {{quote(spec)}} {{clean}} --qemu
    
# TESTS WITH TWISTER
test app="bike_computer" tags="" spec="":
    python scripts/twister.py {{app}} {{quote(tags)}} {{quote(spec)}} nrf5340_map_mint.yaml 

test-qemu app="bike_computer" tags="" spec="":
    python scripts/twister.py {{app}} {{quote(tags)}} {{quote(spec)}} --qemu
    
# CLANG-TIDY
clang-tidy app="bike_computer" spec="":    
    # Step 1 — build to get compile_commands.json (build with all conf files to get the most complete database)
    python scripts/build.py {{app}} {{quote(spec)}} yes --native_sim
    
    # Step 2 — filter the compile_commands.json file for compatibility with clang-tidy
    mkdir -p build_clang
    python3 scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

    # Step 3 — run clang-tidy against the filtered database
    clang-tidy -p build_clang {{working_dir}}/{{app}}/src/main.cpp --extra-arg=-v    

run-clang-tidy app="bike_computer" spec="":
    python scripts/run_clang_tidy.py --app {{app}} --spec {{quote(spec)}} --wd {{working_dir}}
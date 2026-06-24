import sys

status = 0

for f in sys.argv[1:]:
    with open(f, "r", encoding="utf-8", errors="ignore") as file:
        content = file.read()

    if "Copyright" not in content:
        print(f"ERROR: Missing copyright header: {f}")
        status = 1

    if "@author" not in content:
        print(f"ERROR: Missing or invalid author in header: {f}")
        status = 1

sys.exit(status)
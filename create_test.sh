#!/bin/bash

snake_to_pascal() { echo "$1" | perl -pe 's/(^|_)([a-z])/\U$2/g' | sed 's/\.cpp$//'; }
pascal_to_snake() { echo "$1" | perl -pe 's/([a-z0-9])([A-Z])/$1_$2/g; $_=lc'; }

TARGET_NAME=""
FILE_PATH=""
CMAKE_FILE="CMakeLists.txt"

# Parse Args
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--target) TARGET_NAME="$2"; shift 2 ;;
        -f|--file)   FILE_PATH="$2"; shift 2 ;;
        *) echo "Usage: $0 [-t Target] [-f path/file.cpp]"; exit 1 ;;
    esac
done

if [[ -z "$TARGET_NAME" && -z "$FILE_PATH" ]]; then echo "Error: Missing args"; exit 1; fi
if [[ -z "$TARGET_NAME" ]]; then TARGET_NAME=$(snake_to_pascal "$(basename "$FILE_PATH")"); echo "Derived Target: $TARGET_NAME"; fi
if [[ -z "$FILE_PATH" ]]; then FILE_PATH="test/basic/$(pascal_to_snake "$TARGET_NAME").cpp"; echo "Derived Path: $FILE_PATH"; fi

# File Creation
mkdir -p "$(dirname "$FILE_PATH")"
if [[ -f "$FILE_PATH" ]]; then echo "Error: File exists"; exit 1; fi
if [[ -f "template_test.cpp" ]]; then cp "template_test.cpp" "$FILE_PATH"; else touch "$FILE_PATH"; fi

# CMake Injection
if grep -q "add_executable($TARGET_NAME" "$CMAKE_FILE"; then echo "Error: Target exists"; exit 1; fi

TMP=$(mktemp)
ESC_PATH=$(echo "$FILE_PATH" | sed 's/\//\\\//g')

# 1. Add Executable (Marker Priority -> Function Fallback)
if grep -q "# ADD_EXECUTABLE_MARKER" "$CMAKE_FILE"; then
    sed "/# ADD_EXECUTABLE_MARKER/a add_executable($TARGET_NAME $ESC_PATH)" "$CMAKE_FILE" > "$TMP"
else
    sed "/^function(configure_test_target/i add_executable($TARGET_NAME $ESC_PATH)" "$CMAKE_FILE" > "$TMP"
fi
mv "$TMP" "$CMAKE_FILE"

# 2. Configure Target (Marker Priority -> EOF Fallback)
if grep -q "# CONFIGURE_TARGET_MARKER" "$CMAKE_FILE"; then
    sed "/# CONFIGURE_TARGET_MARKER/a configure_test_target($TARGET_NAME)" "$CMAKE_FILE" > "$TMP"
else
    echo "configure_test_target($TARGET_NAME)" >> "$TMP"
    # Merge logic: if we didn't find marker, we need to read current file + append
    cat "$CMAKE_FILE" > "$TMP" && echo "configure_test_target($TARGET_NAME)" >> "$TMP"
fi
mv "$TMP" "$CMAKE_FILE"

echo -e "\n\033[0;32m✓ Test created successfully!\033[0m"
echo "  Target: $TARGET_NAME"
echo "  File:   $FILE_PATH"
echo -e "\033[0;33mNext steps: cmake -B build && cmake --build build --target $TARGET_NAME\033[0m"
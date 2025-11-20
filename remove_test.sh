#!/bin/bash

# Function to convert snake_case to PascalCase
snake_to_pascal() {
    local snake="$1"
    snake="${snake%.cpp}"
    echo "$snake" | awk -F_ '{for(i=1;i<=NF;i++){$i=toupper(substr($i,1,1)) tolower(substr($i,2))}}1' OFS=""
}

# Function to convert PascalCase to snake_case
pascal_to_snake() {
    local pascal="$1"
    echo "$pascal" | sed 's/\([A-Z]\)/_\L\1/g' | sed 's/^_//' | tr '[:upper:]' '[:lower:]'
}

# Parse arguments
TARGET_NAME=""
FILE_PATH=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --target|-t)
            TARGET_NAME="$2"
            shift 2
            ;;
        --file|-f)
            FILE_PATH="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo ""
            echo "Usage:"
            echo "  $0 --target MyTest"
            echo "  $0 --file test/basic/my_test.cpp"
            echo "  $0 --target MyTest --file test/basic/my_test.cpp"
            exit 1
            ;;
    esac
done

# Validate and derive arguments
if [ -z "$TARGET_NAME" ] && [ -z "$FILE_PATH" ]; then
    echo "Error: At least one of --target or --file must be provided"
    echo ""
    echo "Usage:"
    echo "  $0 --target MyTest"
    echo "  $0 --file test/basic/my_test.cpp"
    echo "  $0 --target MyTest --file test/basic/my_test.cpp"
    exit 1
fi

if [ -n "$FILE_PATH" ] && [ -z "$TARGET_NAME" ]; then
    FILE_NAME=$(basename "$FILE_PATH")
    TARGET_NAME=$(snake_to_pascal "$FILE_NAME")
    echo "Derived TargetName: $TARGET_NAME"
fi

if [ -n "$TARGET_NAME" ] && [ -z "$FILE_PATH" ]; then
    # Try to find the file path from CMakeLists.txt
    CMAKE_FILE="CMakeLists.txt"
    if [ -f "$CMAKE_FILE" ]; then
        FILE_PATH=$(grep "add_executable($TARGET_NAME " "$CMAKE_FILE" | sed -n "s/add_executable($TARGET_NAME \(.*\))/\1/p" | tr -d ' ')
        if [ -n "$FILE_PATH" ]; then
            echo "Found FilePath in CMakeLists.txt: $FILE_PATH"
        else
            # Derive from target name
            FILE_NAME=$(pascal_to_snake "$TARGET_NAME").cpp
            FILE_PATH="test/basic/$FILE_NAME"
            echo "Derived FilePath: $FILE_PATH"
        fi
    fi
fi

# Check if CMakeLists.txt exists
CMAKE_FILE="CMakeLists.txt"
if [ ! -f "$CMAKE_FILE" ]; then
    echo "Error: CMakeLists.txt not found"
    exit 1
fi

MODIFIED=false

# Check if target exists in CMakeLists.txt
if ! grep -q "add_executable($TARGET_NAME " "$CMAKE_FILE"; then
    echo "Warning: Target '$TARGET_NAME' not found in CMakeLists.txt"
else
    # Remove add_executable line
    TEMP_FILE=$(mktemp)
    while IFS= read -r line; do
        if [[ ! "$line" =~ ^add_executable\($TARGET_NAME[[:space:]] ]]; then
            echo "$line" >> "$TEMP_FILE"
        else
            echo "Removed from CMakeLists.txt: $line"
            MODIFIED=true
        fi
    done < "$CMAKE_FILE"
    
    # Remove configure_test_target line
    TEMP_FILE2=$(mktemp)
    while IFS= read -r line; do
        if [[ ! "$line" =~ ^configure_test_target\($TARGET_NAME\) ]]; then
            echo "$line" >> "$TEMP_FILE2"
        else
            echo "Removed from CMakeLists.txt: $line"
            MODIFIED=true
        fi
    done < "$TEMP_FILE"
    
    if [ "$MODIFIED" = true ]; then
        mv "$TEMP_FILE2" "$CMAKE_FILE"
    fi
    
    rm -f "$TEMP_FILE" "$TEMP_FILE2"
fi

# Check if test file exists and remove it
if [ -f "$FILE_PATH" ]; then
    rm "$FILE_PATH"
    echo "Deleted test file: $FILE_PATH"
    MODIFIED=true
else
    echo "Warning: Test file not found: $FILE_PATH"
fi

if [ "$MODIFIED" = true ]; then
    echo ""
    echo "✓ Test removed successfully!"
    echo "  Target: $TARGET_NAME"
    echo "  File: $FILE_PATH"
else
    echo ""
    echo "No changes made. Test may not exist."
fi

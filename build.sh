#!/bin/bash

# Paths to include directories and libraries
INCLUDE_DIR="."
LIB_DIR="."
OUTPUT="out.exe"

# Source file
SRC_FILE="main.c"

# Compiler flags
CFLAGS="-Wall -Wextra -g"

# Libraries to link against
LIBS="-lraylib -luser32 -lgdi32 -ladvapi32 -lwinmm -lshell32 -lmsvcrt"

# Compile and link
clang $CFLAGS -o $OUTPUT $SRC_FILE -I$INCLUDE_DIR -L$LIB_DIR $LIBS

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Build succeeded."
    # Run the executable and open the debugger
    echo "Running the program with gdb..."
    ./$OUTPUT
else
    echo "Build failed."
fi

# Keep the terminal open
read -p "Press [Enter] to exit..."

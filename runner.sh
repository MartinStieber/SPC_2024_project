#!/bin/bash

# Get the absolute path to the directory where the script is located
DIR="$(cd "$(dirname "$0")" && pwd)"

# Open a new terminal, run your program, and pass the terminal's PID to the program
gnome-terminal -- bash -c "echo $$ > $DIR/terminal_pid.txt; exec $DIR/cmake-build-debug/SPC_2024_project"
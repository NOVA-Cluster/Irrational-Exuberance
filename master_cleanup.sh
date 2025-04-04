#!/bin/bash

# Master cleanup script to remove all unnecessary components

echo "=== Starting comprehensive cleanup ==="

# Run the MIDI, Star, and StarSequence cleanup
echo -e "\n=== Removing MIDI, Star, and StarSequence components ==="
./cleanup.sh

# Run the Enable cleanup
echo -e "\n=== Removing Enable component ==="
./remove_enable.sh

# Remove all backup files
echo -e "\n=== Removing backup files ==="
rm -f src/*.bak

# Remove temporary scripts
echo -e "\n=== Removing temporary scripts ==="
rm -f cleanup.sh
rm -f remove_enable.sh
rm -f make_scripts_executable.sh

echo -e "\n=== Cleanup complete ==="
echo "All unnecessary components have been removed from the codebase."
echo "The project should now compile without any references to removed components."
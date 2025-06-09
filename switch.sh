#!/bin/bash
# Script to switch ImGui to docking branch

# Navigate to the imgui submodule directory
cd deps/imgui

# Fetch all branches
git fetch origin

# Switch to the docking branch
git checkout docking

# Go back to project root
cd ../..

# Update the submodule reference
git add deps/imgui
git commit -m "Switch ImGui to docking branch"

echo "ImGui switched to docking branch. You can now uncomment the docking features in UIManager.cpp"
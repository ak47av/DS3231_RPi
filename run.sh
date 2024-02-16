make clean
make

# Find the operating system
OS=$(uname -s)

# Check the operating system and take appropriate action
if [ "$OS" = "Darwin" ]; then
    echo "macOS detected"
    make upload
    # Add your macOS-specific commands here
elif [ "$OS" = "Linux" ]; then
    echo "Linux detected"
    # Add your Linux-specific commands here
else
    echo "Unsupported operating system: $OS"
    exit 1
fi
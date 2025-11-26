#!/bin/bash
# ============================================================================
# RegisLex - Linux Release Build Script
# ============================================================================
# This script builds a clean Linux release and packages it in dist/
#
# Prerequisites:
#   - CMake 3.16 or later
#   - GCC or Clang with C11/C++17 support
#   - Make or Ninja
#
# Usage:
#   ./build-linux-release.sh [clean|rebuild|build]
#     clean   - Remove build and dist directories
#     rebuild - Clean then build (default)
#     build   - Build without cleaning
# ============================================================================

set -e  # Exit on error

# Configuration
PROJECT_NAME="RegisLex"
BUILD_DIR="build-linux"
DIST_DIR="dist-linux"
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc 2>/dev/null || echo 4)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
print_header() {
    echo -e "${BLUE}"
    echo "============================================================================"
    echo "  $1"
    echo "============================================================================"
    echo -e "${NC}"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}[$1]${NC} $2"
}

# Parse command line argument
ACTION="${1:-rebuild}"

print_header "$PROJECT_NAME - Linux Release Build"

# ============================================================================
# Check prerequisites
# ============================================================================
print_step "CHECK" "Checking prerequisites..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake not found in PATH"
    echo "Please install CMake: sudo apt-get install cmake (Debian/Ubuntu)"
    echo "                      sudo dnf install cmake (Fedora)"
    echo "                      sudo pacman -S cmake (Arch)"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
print_info "CMake version: $CMAKE_VERSION"

# Check for C/C++ compiler
if command -v gcc &> /dev/null; then
    CC_COMPILER="gcc"
    CXX_COMPILER="g++"
    GCC_VERSION=$(gcc --version | head -n1 | awk '{print $3}')
    print_info "GCC version: $GCC_VERSION"
elif command -v clang &> /dev/null; then
    CC_COMPILER="clang"
    CXX_COMPILER="clang++"
    CLANG_VERSION=$(clang --version | head -n1 | awk '{print $3}')
    print_info "Clang version: $CLANG_VERSION"
else
    print_error "No C compiler found (gcc or clang required)"
    echo "Please install a compiler:"
    echo "  sudo apt-get install build-essential (Debian/Ubuntu)"
    echo "  sudo dnf groupinstall 'Development Tools' (Fedora)"
    echo "  sudo pacman -S base-devel (Arch)"
    exit 1
fi

# Check for Make or Ninja
if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    print_info "Using Ninja build system"
elif command -v make &> /dev/null; then
    GENERATOR="Unix Makefiles"
    print_info "Using Make build system"
else
    print_error "No build system found (make or ninja required)"
    exit 1
fi

# Check for git (optional)
if command -v git &> /dev/null; then
    GIT_AVAILABLE=1
else
    GIT_AVAILABLE=0
fi

echo

# ============================================================================
# Handle actions
# ============================================================================
do_clean() {
    print_step "CLEAN" "Removing build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_info "Removed $BUILD_DIR"
    fi

    print_step "CLEAN" "Removing dist directory..."
    if [ -d "$DIST_DIR" ]; then
        rm -rf "$DIST_DIR"
        print_info "Removed $DIST_DIR"
    fi

    print_step "DONE" "Clean completed"

    if [ "$ACTION" == "clean" ]; then
        exit 0
    fi
}

do_configure() {
    echo
    print_step "CONFIGURE" "Running CMake configuration..."
    echo

    mkdir -p "$BUILD_DIR"

    # Run CMake configuration
    cmake -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DREGISLEX_BUILD_TESTS=OFF \
        -DCMAKE_C_COMPILER="$CC_COMPILER" \
        -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
        -B "$BUILD_DIR" \
        -S .

    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        exit 1
    fi

    echo
    print_step "DONE" "Configuration completed"
}

do_compile() {
    echo
    print_step "BUILD" "Compiling $BUILD_TYPE build with $PARALLEL_JOBS jobs..."
    echo

    cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$PARALLEL_JOBS"

    if [ $? -ne 0 ]; then
        echo
        print_error "Build failed"
        exit 1
    fi

    echo
    print_step "DONE" "Build completed successfully"
}

do_package() {
    echo
    print_step "PACKAGE" "Creating distribution package..."
    echo

    # Create dist directory structure
    mkdir -p "$DIST_DIR"/{bin,lib,include,docs,resources}

    # Copy executables
    print_step "COPY" "Copying executables..."
    if [ -f "$BUILD_DIR/bin/regislex" ]; then
        cp "$BUILD_DIR/bin/regislex" "$DIST_DIR/bin/"
        chmod +x "$DIST_DIR/bin/regislex"
    fi

    if [ -f "$BUILD_DIR/bin/regislex-cli" ]; then
        cp "$BUILD_DIR/bin/regislex-cli" "$DIST_DIR/bin/"
        chmod +x "$DIST_DIR/bin/regislex-cli"
    fi

    # Copy libraries
    print_step "COPY" "Copying libraries..."
    if [ -f "$BUILD_DIR/lib/libregislex_core.a" ]; then
        cp "$BUILD_DIR/lib/libregislex_core.a" "$DIST_DIR/lib/"
    fi

    if [ -f "$BUILD_DIR/lib/libregislex_core.so" ]; then
        cp "$BUILD_DIR/lib/libregislex_core.so" "$DIST_DIR/lib/"
    fi

    # Copy third-party libraries if present
    if [ -f "$BUILD_DIR/lib/libsqlite3.a" ]; then
        cp "$BUILD_DIR/lib/libsqlite3.a" "$DIST_DIR/lib/"
    fi

    if [ -f "$BUILD_DIR/third_party/sqlite/libsqlite3.a" ]; then
        cp "$BUILD_DIR/third_party/sqlite/libsqlite3.a" "$DIST_DIR/lib/"
    fi

    # Copy headers
    print_step "COPY" "Copying headers..."
    if [ -d "include/regislex" ]; then
        cp -r include/regislex "$DIST_DIR/include/"
    fi

    if [ -d "include/platform" ]; then
        cp -r include/platform "$DIST_DIR/include/"
    fi

    if [ -d "include/database" ]; then
        cp -r include/database "$DIST_DIR/include/"
    fi

    # Copy documentation
    print_step "COPY" "Copying documentation..."
    [ -f "README.md" ] && cp "README.md" "$DIST_DIR/"
    [ -f "LICENSE" ] && cp "LICENSE" "$DIST_DIR/"
    [ -d "docs" ] && cp -r docs/* "$DIST_DIR/docs/" 2>/dev/null || true

    # Copy resources/configs
    print_step "COPY" "Copying resources..."
    [ -d "resources" ] && cp -r resources/* "$DIST_DIR/resources/" 2>/dev/null || true

    # Create version info file
    print_step "INFO" "Creating version info..."
    {
        echo "RegisLex Distribution Package"
        echo "=============================="
        echo ""
        echo "Version: 1.0.0"
        echo "Build Type: $BUILD_TYPE"
        echo "Platform: Linux $(uname -m)"
        echo "Build Date: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Kernel: $(uname -r)"
        echo ""
        echo "Compiler: $CC_COMPILER"
        echo "CMake Version: $CMAKE_VERSION"
    } > "$DIST_DIR/VERSION.txt"

    # Get git info if available
    if [ $GIT_AVAILABLE -eq 1 ] && [ -d .git ]; then
        GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
        GIT_TAG=$(git describe --tags --always 2>/dev/null || echo "unknown")
        echo "Git Commit: $GIT_HASH" >> "$DIST_DIR/VERSION.txt"
        echo "Git Tag: $GIT_TAG" >> "$DIST_DIR/VERSION.txt"
    fi

    # Create launcher scripts
    print_step "INFO" "Creating launcher scripts..."

    # Server launcher
    cat > "$DIST_DIR/start-server.sh" << 'EOF'
#!/bin/bash
# RegisLex Server Launcher
cd "$(dirname "$0")"
./bin/regislex "$@"
EOF
    chmod +x "$DIST_DIR/start-server.sh"

    # CLI launcher
    cat > "$DIST_DIR/regislex.sh" << 'EOF'
#!/bin/bash
# RegisLex CLI
cd "$(dirname "$0")"
./bin/regislex-cli "$@"
EOF
    chmod +x "$DIST_DIR/regislex.sh"

    # Create systemd service file
    print_step "INFO" "Creating systemd service template..."
    cat > "$DIST_DIR/regislex.service" << EOF
[Unit]
Description=RegisLex Enterprise Legal Software Suite
After=network.target

[Service]
Type=simple
User=regislex
Group=regislex
WorkingDirectory=/opt/regislex
ExecStart=/opt/regislex/bin/regislex -p 8080
Restart=on-failure
RestartSec=5s

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/regislex /var/log/regislex

[Install]
WantedBy=multi-user.target
EOF

    # List distribution contents
    echo
    echo "Distribution Contents:"
    echo "----------------------"
    ls -lh "$DIST_DIR/bin/" 2>/dev/null || true
    ls -lh "$DIST_DIR/lib/" 2>/dev/null || true
    echo

    # Create tarball
    print_step "PACKAGE" "Creating tarball..."
    TARBALL="regislex-$(uname -m)-linux-${BUILD_TYPE}.tar.gz"
    tar -czf "$TARBALL" -C "$DIST_DIR" .
    print_info "Created $TARBALL"

    # Run tests if available
    if [ -f "$BUILD_DIR/bin/regislex_tests" ]; then
        echo
        print_step "TEST" "Running unit tests..."
        "$BUILD_DIR/bin/regislex_tests"
        if [ $? -eq 0 ]; then
            print_step "DONE" "All tests passed"
        else
            print_warn "Some tests failed"
        fi
    fi
}

# Main execution flow
case "$ACTION" in
    clean)
        do_clean
        ;;
    build)
        if [ ! -d "$BUILD_DIR" ]; then
            do_configure
        else
            print_info "Build directory exists, skipping configure"
        fi
        do_compile
        do_package
        ;;
    rebuild)
        do_clean
        do_configure
        do_compile
        do_package
        ;;
    *)
        print_error "Unknown action '$ACTION'"
        echo "Usage: $0 [clean|rebuild|build]"
        exit 1
        ;;
esac

# Success message
echo
print_header "BUILD SUCCESSFUL"
echo
print_info "Distribution package created in: $(pwd)/$DIST_DIR/"
print_info "Tarball created: $(pwd)/$TARBALL"
echo
echo "To start the server:"
echo "  cd $DIST_DIR"
echo "  ./start-server.sh -p 8080"
echo
echo "To use the CLI:"
echo "  cd $DIST_DIR"
echo "  ./regislex.sh help"
echo
echo "To install system-wide:"
echo "  sudo ./install.sh"
echo

exit 0

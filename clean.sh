#!/bin/bash
# ============================================================================
# RegisLex - Clean Script
# ============================================================================
# This script removes build artifacts and temporary files
#
# Usage:
#   ./clean.sh [OPTIONS]
#
# Options:
#   --all         Remove all build artifacts and distributions
#   --build       Remove build directories only
#   --dist        Remove distribution directories only
#   --cache       Remove CMake cache and configuration
#   --help        Show this help message
#
# Examples:
#   ./clean.sh                # Remove all build artifacts (same as --all)
#   ./clean.sh --build        # Remove only build directories
#   ./clean.sh --dist         # Remove only distribution directories
#   ./clean.sh --cache        # Remove CMake cache files
# ============================================================================

set -e  # Exit on error

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

show_help() {
    cat << EOF
RegisLex Clean Script

Usage:
  ./clean.sh [OPTIONS]

Options:
  --all         Remove all build artifacts and distributions (default)
  --build       Remove build directories only
  --dist        Remove distribution directories only
  --cache       Remove CMake cache and configuration
  --help        Show this help message

Examples:
  ./clean.sh                # Remove all build artifacts
  ./clean.sh --build        # Remove only build directories
  ./clean.sh --dist         # Remove only distribution directories
  ./clean.sh --cache        # Remove CMake cache files

Directories that can be cleaned:
  build/                    # Generic build directory
  build-linux/              # Linux build directory
  build-release/            # Release build directory
  build-debug/              # Debug build directory
  dist/                     # Generic distribution directory
  dist-linux/               # Linux distribution directory
  *.tar.gz                  # Distribution tarballs

EOF
    exit 0
}

# Default options
CLEAN_BUILD=0
CLEAN_DIST=0
CLEAN_CACHE=0
CLEAN_ALL=0

# Parse command line arguments
if [ $# -eq 0 ]; then
    CLEAN_ALL=1
fi

while [[ $# -gt 0 ]]; do
    case $1 in
        --all)
            CLEAN_ALL=1
            shift
            ;;
        --build)
            CLEAN_BUILD=1
            shift
            ;;
        --dist)
            CLEAN_DIST=1
            shift
            ;;
        --cache)
            CLEAN_CACHE=1
            shift
            ;;
        --help|-h)
            show_help
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# If --all is specified, clean everything
if [ $CLEAN_ALL -eq 1 ]; then
    CLEAN_BUILD=1
    CLEAN_DIST=1
    CLEAN_CACHE=1
fi

print_header "RegisLex Clean"

# ============================================================================
# Clean build directories
# ============================================================================
if [ $CLEAN_BUILD -eq 1 ]; then
    print_step "CLEAN" "Removing build directories..."

    REMOVED_COUNT=0

    # List of build directories to clean
    BUILD_DIRS=(
        "build"
        "build-linux"
        "build-release"
        "build-debug"
        "build-win"
        "build-macos"
        "_build"
        "out"
    )

    for dir in "${BUILD_DIRS[@]}"; do
        if [ -d "$dir" ]; then
            SIZE=$(du -sh "$dir" 2>/dev/null | cut -f1 || echo "unknown")
            rm -rf "$dir"
            print_info "Removed $dir/ ($SIZE)"
            ((REMOVED_COUNT++))
        fi
    done

    if [ $REMOVED_COUNT -eq 0 ]; then
        print_info "No build directories found"
    else
        print_info "Removed $REMOVED_COUNT build directories"
    fi
    echo
fi

# ============================================================================
# Clean distribution directories and tarballs
# ============================================================================
if [ $CLEAN_DIST -eq 1 ]; then
    print_step "CLEAN" "Removing distribution directories..."

    REMOVED_COUNT=0

    # List of distribution directories to clean
    DIST_DIRS=(
        "dist"
        "dist-linux"
        "dist-win"
        "dist-macos"
        "package"
        "release"
    )

    for dir in "${DIST_DIRS[@]}"; do
        if [ -d "$dir" ]; then
            SIZE=$(du -sh "$dir" 2>/dev/null | cut -f1 || echo "unknown")
            rm -rf "$dir"
            print_info "Removed $dir/ ($SIZE)"
            ((REMOVED_COUNT++))
        fi
    done

    if [ $REMOVED_COUNT -eq 0 ]; then
        print_info "No distribution directories found"
    fi

    # Remove tarballs
    TARBALL_COUNT=0
    for tarball in regislex-*.tar.gz regislex-*.tar.bz2 regislex-*.tar.xz regislex-*.zip; do
        if [ -f "$tarball" ]; then
            SIZE=$(du -sh "$tarball" 2>/dev/null | cut -f1 || echo "unknown")
            rm -f "$tarball"
            print_info "Removed $tarball ($SIZE)"
            ((TARBALL_COUNT++))
        fi
    done

    if [ $TARBALL_COUNT -eq 0 ]; then
        print_info "No distribution tarballs found"
    fi

    echo
fi

# ============================================================================
# Clean CMake cache and configuration files
# ============================================================================
if [ $CLEAN_CACHE -eq 1 ]; then
    print_step "CLEAN" "Removing CMake cache and configuration..."

    REMOVED_COUNT=0

    # Remove CMakeCache.txt in all build directories
    find . -maxdepth 2 -name "CMakeCache.txt" -type f 2>/dev/null | while read -r file; do
        rm -f "$file"
        print_info "Removed $file"
        ((REMOVED_COUNT++))
    done

    # Remove CMakeFiles directories
    find . -maxdepth 2 -name "CMakeFiles" -type d 2>/dev/null | while read -r dir; do
        rm -rf "$dir"
        print_info "Removed $dir/"
        ((REMOVED_COUNT++))
    done

    # Remove cmake_install.cmake files
    find . -maxdepth 2 -name "cmake_install.cmake" -type f 2>/dev/null | while read -r file; do
        rm -f "$file"
        ((REMOVED_COUNT++))
    done

    # Remove Makefile generated by CMake (but not hand-written ones)
    for makefile in build*/Makefile; do
        if [ -f "$makefile" ]; then
            rm -f "$makefile"
            ((REMOVED_COUNT++))
        fi
    done

    if [ $REMOVED_COUNT -eq 0 ]; then
        print_info "No CMake cache files found"
    fi

    echo
fi

# ============================================================================
# Clean temporary and editor files (optional)
# ============================================================================
print_step "CLEAN" "Removing temporary files..."

TEMP_COUNT=0

# Remove editor backup files
for pattern in "*~" "*.swp" "*.swo" ".*.swp" ".*.swo" "*.orig"; do
    while IFS= read -r -d '' file; do
        rm -f "$file"
        ((TEMP_COUNT++))
    done < <(find . -name "$pattern" -type f -print0 2>/dev/null)
done

# Remove core dumps
while IFS= read -r -d '' file; do
    rm -f "$file"
    print_warn "Removed core dump: $file"
    ((TEMP_COUNT++))
done < <(find . -name "core" -o -name "core.*" -type f -print0 2>/dev/null)

if [ $TEMP_COUNT -gt 0 ]; then
    print_info "Removed $TEMP_COUNT temporary files"
else
    print_info "No temporary files found"
fi

echo

# ============================================================================
# Summary
# ============================================================================
print_header "CLEAN COMPLETE"

# Show disk space saved
if command -v du &> /dev/null; then
    CURRENT_SIZE=$(du -sh . 2>/dev/null | cut -f1 || echo "unknown")
    print_info "Current directory size: $CURRENT_SIZE"
fi

echo
print_info "Cleaned successfully!"
echo

# Show what would be cleaned with different options
if [ $CLEAN_ALL -eq 0 ]; then
    echo "Tip: To clean everything, run: ./clean.sh --all"
fi

exit 0

#!/bin/bash
# ============================================================================
# RegisLex - Linux Installation Script
# ============================================================================
# This script installs RegisLex system-wide on Linux
#
# Usage:
#   sudo ./install.sh [OPTIONS]
#
# Options:
#   --prefix PATH     Installation prefix (default: /usr/local)
#   --uninstall       Uninstall RegisLex
#   --user            Install for current user only (no sudo required)
#   --systemd         Install and enable systemd service
#   --help            Show this help message
#
# Examples:
#   sudo ./install.sh                    # Install to /usr/local
#   sudo ./install.sh --prefix /opt      # Install to /opt
#   ./install.sh --user                  # Install to ~/.local
#   sudo ./install.sh --systemd          # Install with systemd service
#   sudo ./install.sh --uninstall        # Uninstall
# ============================================================================

set -e  # Exit on error

# Default configuration
PREFIX="/usr/local"
DIST_DIR="dist-linux"
USER_INSTALL=0
INSTALL_SYSTEMD=0
UNINSTALL=0

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
RegisLex Installation Script

Usage:
  sudo ./install.sh [OPTIONS]

Options:
  --prefix PATH     Installation prefix (default: /usr/local)
  --uninstall       Uninstall RegisLex
  --user            Install for current user only (no sudo required)
  --systemd         Install and enable systemd service
  --help            Show this help message

Examples:
  sudo ./install.sh                    # Install to /usr/local
  sudo ./install.sh --prefix /opt      # Install to /opt
  ./install.sh --user                  # Install to ~/.local
  sudo ./install.sh --systemd          # Install with systemd service
  sudo ./install.sh --uninstall        # Uninstall

Installation locations:
  Binaries:       \$PREFIX/bin/
  Libraries:      \$PREFIX/lib/
  Headers:        \$PREFIX/include/
  Documentation:  \$PREFIX/share/doc/regislex/
  Resources:      \$PREFIX/share/regislex/
  Data:           /var/lib/regislex/ (system) or ~/.local/share/regislex/ (user)
  Logs:           /var/log/regislex/ (system) or ~/.local/share/regislex/logs/ (user)

EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix)
            PREFIX="$2"
            shift 2
            ;;
        --user)
            USER_INSTALL=1
            PREFIX="$HOME/.local"
            shift
            ;;
        --systemd)
            INSTALL_SYSTEMD=1
            shift
            ;;
        --uninstall)
            UNINSTALL=1
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

# ============================================================================
# Uninstall function
# ============================================================================
do_uninstall() {
    print_header "RegisLex Uninstallation"

    print_step "REMOVE" "Removing RegisLex files..."

    # Stop and disable systemd service if it exists
    if systemctl is-active --quiet regislex 2>/dev/null; then
        print_step "SYSTEMD" "Stopping RegisLex service..."
        systemctl stop regislex
    fi

    if systemctl is-enabled --quiet regislex 2>/dev/null; then
        print_step "SYSTEMD" "Disabling RegisLex service..."
        systemctl disable regislex
    fi

    if [ -f /etc/systemd/system/regislex.service ]; then
        rm -f /etc/systemd/system/regislex.service
        systemctl daemon-reload
        print_info "Removed systemd service"
    fi

    # Remove binaries
    rm -f "$PREFIX/bin/regislex"
    rm -f "$PREFIX/bin/regislex-cli"
    print_info "Removed binaries"

    # Remove libraries
    rm -f "$PREFIX/lib/libregislex_core.a"
    rm -f "$PREFIX/lib/libregislex_core.so"
    rm -f "$PREFIX/lib/libsqlite3.a"
    print_info "Removed libraries"

    # Remove headers
    rm -rf "$PREFIX/include/regislex"
    rm -rf "$PREFIX/include/platform"
    rm -rf "$PREFIX/include/database"
    print_info "Removed headers"

    # Remove documentation and resources
    rm -rf "$PREFIX/share/doc/regislex"
    rm -rf "$PREFIX/share/regislex"
    print_info "Removed documentation and resources"

    # Ask about data directories
    echo
    print_warn "Data directories were not removed:"
    if [ $USER_INSTALL -eq 1 ]; then
        echo "  - ~/.local/share/regislex/"
        echo "To remove: rm -rf ~/.local/share/regislex"
    else
        echo "  - /var/lib/regislex/"
        echo "  - /var/log/regislex/"
        echo "To remove: sudo rm -rf /var/lib/regislex /var/log/regislex"
    fi

    echo
    print_header "UNINSTALL COMPLETE"
    exit 0
}

# ============================================================================
# Installation function
# ============================================================================
do_install() {
    print_header "RegisLex Installation"

    # Check if running as root for system install
    if [ $USER_INSTALL -eq 0 ] && [ "$EUID" -ne 0 ]; then
        print_error "System-wide installation requires root privileges"
        echo "Please run: sudo $0"
        echo "Or use: $0 --user (for user installation)"
        exit 1
    fi

    # Check if distribution directory exists
    if [ ! -d "$DIST_DIR" ]; then
        print_error "Distribution directory '$DIST_DIR' not found"
        echo "Please run the build script first: ./build-linux-release.sh"
        exit 1
    fi

    # Check if binaries exist
    if [ ! -f "$DIST_DIR/bin/regislex" ] && [ ! -f "$DIST_DIR/bin/regislex-cli" ]; then
        print_error "No binaries found in $DIST_DIR/bin/"
        echo "Please run the build script first: ./build-linux-release.sh"
        exit 1
    fi

    print_info "Installing to: $PREFIX"
    echo

    # Create directories
    print_step "MKDIR" "Creating directories..."
    mkdir -p "$PREFIX/bin"
    mkdir -p "$PREFIX/lib"
    mkdir -p "$PREFIX/include"
    mkdir -p "$PREFIX/share/doc/regislex"
    mkdir -p "$PREFIX/share/regislex"

    # Install binaries
    print_step "INSTALL" "Installing binaries..."
    if [ -f "$DIST_DIR/bin/regislex" ]; then
        install -m 755 "$DIST_DIR/bin/regislex" "$PREFIX/bin/"
        print_info "Installed regislex"
    fi

    if [ -f "$DIST_DIR/bin/regislex-cli" ]; then
        install -m 755 "$DIST_DIR/bin/regislex-cli" "$PREFIX/bin/"
        print_info "Installed regislex-cli"
    fi

    # Install libraries
    print_step "INSTALL" "Installing libraries..."
    if [ -f "$DIST_DIR/lib/libregislex_core.a" ]; then
        install -m 644 "$DIST_DIR/lib/libregislex_core.a" "$PREFIX/lib/"
        print_info "Installed libregislex_core.a"
    fi

    if [ -f "$DIST_DIR/lib/libregislex_core.so" ]; then
        install -m 755 "$DIST_DIR/lib/libregislex_core.so" "$PREFIX/lib/"
        print_info "Installed libregislex_core.so"
    fi

    if [ -f "$DIST_DIR/lib/libsqlite3.a" ]; then
        install -m 644 "$DIST_DIR/lib/libsqlite3.a" "$PREFIX/lib/"
        print_info "Installed libsqlite3.a"
    fi

    # Update library cache (for system installs)
    if [ $USER_INSTALL -eq 0 ] && command -v ldconfig &> /dev/null; then
        ldconfig
    fi

    # Install headers
    print_step "INSTALL" "Installing headers..."
    if [ -d "$DIST_DIR/include/regislex" ]; then
        cp -r "$DIST_DIR/include/regislex" "$PREFIX/include/"
        print_info "Installed regislex headers"
    fi

    if [ -d "$DIST_DIR/include/platform" ]; then
        cp -r "$DIST_DIR/include/platform" "$PREFIX/include/"
        print_info "Installed platform headers"
    fi

    if [ -d "$DIST_DIR/include/database" ]; then
        cp -r "$DIST_DIR/include/database" "$PREFIX/include/"
        print_info "Installed database headers"
    fi

    # Install documentation
    print_step "INSTALL" "Installing documentation..."
    [ -f "$DIST_DIR/README.md" ] && cp "$DIST_DIR/README.md" "$PREFIX/share/doc/regislex/"
    [ -f "$DIST_DIR/LICENSE" ] && cp "$DIST_DIR/LICENSE" "$PREFIX/share/doc/regislex/"
    [ -f "$DIST_DIR/VERSION.txt" ] && cp "$DIST_DIR/VERSION.txt" "$PREFIX/share/doc/regislex/"
    [ -d "$DIST_DIR/docs" ] && cp -r "$DIST_DIR/docs"/* "$PREFIX/share/doc/regislex/" 2>/dev/null || true
    print_info "Installed documentation"

    # Install resources
    print_step "INSTALL" "Installing resources..."
    [ -d "$DIST_DIR/resources" ] && cp -r "$DIST_DIR/resources"/* "$PREFIX/share/regislex/" 2>/dev/null || true

    # Create data directories
    print_step "MKDIR" "Creating data directories..."
    if [ $USER_INSTALL -eq 1 ]; then
        mkdir -p "$HOME/.local/share/regislex"/{data,logs}
        print_info "Data directory: $HOME/.local/share/regislex/"
    else
        # Create system user if it doesn't exist
        if ! id -u regislex &>/dev/null; then
            print_step "USER" "Creating regislex user..."
            useradd -r -s /bin/false -d /var/lib/regislex regislex
        fi

        mkdir -p /var/lib/regislex
        mkdir -p /var/log/regislex
        chown -R regislex:regislex /var/lib/regislex
        chown -R regislex:regislex /var/log/regislex
        chmod 755 /var/lib/regislex
        chmod 755 /var/log/regislex
        print_info "Data directory: /var/lib/regislex/"
        print_info "Log directory: /var/log/regislex/"
    fi

    # Install systemd service
    if [ $INSTALL_SYSTEMD -eq 1 ] && [ $USER_INSTALL -eq 0 ]; then
        print_step "SYSTEMD" "Installing systemd service..."

        if [ -f "$DIST_DIR/regislex.service" ]; then
            # Update paths in service file
            sed -e "s|/opt/regislex|$PREFIX|g" \
                -e "s|ExecStart=.*|ExecStart=$PREFIX/bin/regislex -p 8080|g" \
                "$DIST_DIR/regislex.service" > /etc/systemd/system/regislex.service

            systemctl daemon-reload
            print_info "Installed systemd service"

            # Ask to enable and start service
            echo
            read -p "Enable and start RegisLex service now? [y/N] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                systemctl enable regislex
                systemctl start regislex
                print_info "Service enabled and started"
            else
                print_info "Service installed but not enabled"
                echo "To enable: sudo systemctl enable regislex"
                echo "To start:  sudo systemctl start regislex"
            fi
        fi
    fi

    echo
    print_header "INSTALLATION COMPLETE"
    echo

    # Show installation summary
    print_info "Installation summary:"
    echo "  Prefix:       $PREFIX"
    echo "  Binaries:     $PREFIX/bin/"
    echo "  Libraries:    $PREFIX/lib/"
    echo "  Headers:      $PREFIX/include/"
    echo "  Docs:         $PREFIX/share/doc/regislex/"

    if [ $USER_INSTALL -eq 1 ]; then
        echo "  Data:         $HOME/.local/share/regislex/"
        echo
        print_info "Add $PREFIX/bin to your PATH if not already included:"
        echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc"
    else
        echo "  Data:         /var/lib/regislex/"
        echo "  Logs:         /var/log/regislex/"
    fi

    echo
    print_info "Quick start:"
    echo "  regislex-cli init              # Initialize RegisLex"
    echo "  regislex-cli case-create ...   # Create a case"
    echo "  regislex -p 8080               # Start server"

    if [ $INSTALL_SYSTEMD -eq 1 ] && [ $USER_INSTALL -eq 0 ]; then
        echo
        print_info "Systemd service:"
        echo "  systemctl status regislex"
        echo "  systemctl stop regislex"
        echo "  systemctl restart regislex"
    fi

    echo
}

# ============================================================================
# Main execution
# ============================================================================
if [ $UNINSTALL -eq 1 ]; then
    do_uninstall
else
    do_install
fi

exit 0

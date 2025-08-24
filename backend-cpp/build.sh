#!/bin/bash

# Proyecto Izana - Build Script
# Builds the C++ backend with all dependencies

set -e

# Configuration
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
INSTALL_DIR="$PROJECT_DIR/install"

# Build type (Debug or Release)
BUILD_TYPE="${1:-Release}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install system dependencies
install_dependencies() {
    print_status "Installing system dependencies..."
    
    if command_exists apt-get; then
        # Ubuntu/Debian
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libserial-dev \
            nlohmann-json3-dev \
            libwebsocketpp-dev \
            libboost-system-dev \
            libboost-thread-dev \
            libboost-chrono-dev \
            libfftw3-dev
    elif command_exists yum; then
        # CentOS/RHEL
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y \
            cmake3 \
            pkgconfig \
            libserial-devel \
            nlohmann-json-devel \
            websocketpp-devel \
            boost-devel \
            fftw3-devel
    elif command_exists brew; then
        # macOS
        brew install \
            cmake \
            pkg-config \
            libserial \
            nlohmann-json \
            websocketpp \
            boost \
            fftw
    else
        print_error "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=0
    
    if ! command_exists cmake; then
        print_error "CMake not found"
        missing_deps=1
    fi
    
    if ! command_exists g++; then
        print_error "g++ not found"
        missing_deps=1
    fi
    
    if ! pkg-config --exists libserial; then
        print_warning "libserial not found (will try to build anyway)"
    fi
    
    if ! pkg-config --exists nlohmann_json; then
        print_warning "nlohmann_json not found (will try to build anyway)"
    fi
    
    if [ $missing_deps -eq 1 ]; then
        print_error "Missing critical dependencies. Run: $0 --install-deps"
        exit 1
    fi
    
    print_status "Dependencies check completed"
}

# Function to configure CMake
configure_cmake() {
    print_status "Configuring CMake build (${BUILD_TYPE})..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    cmake \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DBUILD_TESTS=ON \
        -DBUILD_EXAMPLES=ON \
        "$PROJECT_DIR"
    
    print_status "CMake configuration completed"
}

# Function to build the project
build_project() {
    print_status "Building project..."
    
    cd "$BUILD_DIR"
    
    # Build with make (use all available cores)
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    print_status "Build completed successfully"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$BUILD_DIR"
    
    if [ -f Makefile ]; then
        make test
        print_status "All tests passed"
    else
        print_warning "No tests found"
    fi
}

# Function to install the project
install_project() {
    print_status "Installing project to $INSTALL_DIR..."
    
    cd "$BUILD_DIR"
    make install
    
    print_status "Installation completed"
}

# Function to clean build directory
clean_build() {
    print_status "Cleaning build directory..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_status "Build directory cleaned"
    else
        print_status "Build directory already clean"
    fi
}

# Function to create package
create_package() {
    print_status "Creating installation package..."
    
    cd "$BUILD_DIR"
    
    if command_exists cpack; then
        cpack
        print_status "Package created successfully"
    else
        print_warning "CPack not available, skipping package creation"
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [options] [build-type]"
    echo ""
    echo "Build types:"
    echo "  Debug       Build with debug information (default if no type specified)"
    echo "  Release     Build optimized release version (default)"
    echo ""
    echo "Options:"
    echo "  --install-deps    Install system dependencies"
    echo "  --clean          Clean build directory before building"
    echo "  --test           Run tests after building"
    echo "  --install        Install after building"
    echo "  --package        Create installation package"
    echo "  --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build release version"
    echo "  $0 Debug             # Build debug version"
    echo "  $0 --clean Release   # Clean and build release"
    echo "  $0 --test --install  # Build, test, and install"
}

# Main build function
main() {
    local install_deps=0
    local clean=0
    local run_test=0
    local install=0
    local package=0
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --install-deps)
                install_deps=1
                shift
                ;;
            --clean)
                clean=1
                shift
                ;;
            --test)
                run_test=1
                shift
                ;;
            --install)
                install=1
                shift
                ;;
            --package)
                package=1
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            Debug|Release)
                BUILD_TYPE="$1"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    print_status "Starting Proyecto Izana build process..."
    print_status "Build type: $BUILD_TYPE"
    print_status "Project directory: $PROJECT_DIR"
    print_status "Build directory: $BUILD_DIR"
    
    # Install dependencies if requested
    if [ $install_deps -eq 1 ]; then
        install_dependencies
    fi
    
    # Check dependencies
    check_dependencies
    
    # Clean if requested
    if [ $clean -eq 1 ]; then
        clean_build
    fi
    
    # Configure and build
    configure_cmake
    build_project
    
    # Run tests if requested
    if [ $run_test -eq 1 ]; then
        run_tests
    fi
    
    # Install if requested
    if [ $install -eq 1 ]; then
        install_project
    fi
    
    # Create package if requested
    if [ $package -eq 1 ]; then
        create_package
    fi
    
    print_status "Build process completed successfully!"
    print_status "Executable location: $BUILD_DIR/izana-backend"
    
    if [ $install -eq 1 ]; then
        print_status "Installed to: $INSTALL_DIR/bin/izana-backend"
    fi
}

# Run main function with all arguments
main "$@"
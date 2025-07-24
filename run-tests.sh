#!/bin/bash

# Script to build and run PocketMage V3 tests in Docker
# Usage: ./run-tests.sh [command]

set -e  # Exit on any error

# Configuration
IMAGE_NAME="pocketmage-v3-tests"
CONTAINER_NAME="pocketmage-tests-runner"
WORKSPACE_DIR="$(pwd)"
PROJECT_DIR="Code/PocketMage_V3"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to clean up existing container
cleanup_container() {
    if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        log_info "Removing existing container: ${CONTAINER_NAME}"
        docker rm -f "${CONTAINER_NAME}" > /dev/null 2>&1 || true
    fi
}

# Function to build Docker image
build_image() {
    log_info "Building Docker image: ${IMAGE_NAME}"
    
    if ! docker build -t "${IMAGE_NAME}" .; then
        log_error "Failed to build Docker image"
        exit 1
    fi
    
    log_success "Docker image built successfully"
}

# Function to run tests in container
run_tests() {
    local cmd="${1:-platformio test -e native -v}"
    
    log_info "Running command in container: ${cmd}"
    log_info "Workspace: ${WORKSPACE_DIR}"
    
    # Clean up any existing container
    cleanup_container
    
    # Run the container with workspace mounted
    if ! docker run \
        --name "${CONTAINER_NAME}" \
        --rm \
        -v "${WORKSPACE_DIR}:/workspace" \
        -w "/workspace/${PROJECT_DIR}" \
        "${IMAGE_NAME}" \
        bash -c "${cmd}"; then
        log_error "Tests failed or container execution failed"
        exit 1
    fi
    
    log_success "Command completed successfully"
}

# Function to run interactive shell
run_shell() {
    log_info "Starting interactive shell in container"
    
    cleanup_container
    
    docker run \
        --name "${CONTAINER_NAME}" \
        --rm \
        -it \
        -v "${WORKSPACE_DIR}:/workspace" \
        -w "/workspace/${PROJECT_DIR}" \
        "${IMAGE_NAME}" \
        bash
}

# Function to show help
show_help() {
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  build              Build the Docker image only"
    echo "  test               Build image and run default tests (default)"
    echo "  test-specific      Build image and run specific test file"
    echo "  shell              Build image and start interactive shell"
    echo "  clean              Remove Docker image and any containers"
    echo "  help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                 # Run default tests"
    echo "  $0 test            # Run default tests"
    echo "  $0 test-specific   # Run specific test (will prompt for test name)"
    echo "  $0 shell           # Start interactive shell for debugging"
    echo "  $0 clean           # Clean up Docker artifacts"
}

# Function to clean up Docker artifacts
clean_docker() {
    log_info "Cleaning up Docker artifacts"
    
    cleanup_container
    
    if docker images --format '{{.Repository}}' | grep -q "^${IMAGE_NAME}$"; then
        log_info "Removing Docker image: ${IMAGE_NAME}"
        docker rmi "${IMAGE_NAME}" > /dev/null 2>&1 || true
    fi
    
    log_success "Cleanup completed"
}

# Function to run specific test
run_specific_test() {
    echo "Available test targets:"
    echo "  1. All native tests (default)"
    echo "  2. Specific test file"
    echo "  3. Custom PlatformIO command"
    echo ""
    read -p "Enter choice (1-3): " choice
    
    case $choice in
        1)
            run_tests "platformio test -e native -v"
            ;;
        2)
            read -p "Enter test file name (e.g., test_tasks): " test_file
            run_tests "platformio test -e native -f ${test_file} -v"
            ;;
        3)
            read -p "Enter custom command: " custom_cmd
            run_tests "${custom_cmd}"
            ;;
        *)
            log_error "Invalid choice"
            exit 1
            ;;
    esac
}

# Main script logic
main() {
    local command="${1:-test}"
    
    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    # Check if Dockerfile exists
    if [[ ! -f "Dockerfile" ]]; then
        log_error "Dockerfile not found in current directory"
        exit 1
    fi
    
    # Check if PlatformIO project directory exists
    if [[ ! -d "${PROJECT_DIR}" ]]; then
        log_error "PlatformIO project directory not found: ${PROJECT_DIR}"
        exit 1
    fi
    
    # Check if platformio.ini exists in project directory
    if [[ ! -f "${PROJECT_DIR}/platformio.ini" ]]; then
        log_error "platformio.ini not found in ${PROJECT_DIR}"
        exit 1
    fi
    
    case "${command}" in
        "build")
            build_image
            ;;
        "test")
            build_image
            run_tests
            ;;
        "test-specific")
            build_image
            run_specific_test
            ;;
        "shell")
            build_image
            run_shell
            ;;
        "clean")
            clean_docker
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            log_error "Unknown command: ${command}"
            show_help
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@" 
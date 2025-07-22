#!/bin/bash

set -e

echo "🔍 EinkPDA Development Environment Health Check"
echo "=============================================="

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $message"
    elif [ "$status" = "FAIL" ]; then
        echo -e "${RED}❌ FAIL${NC}: $message"
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠️  WARN${NC}: $message"
    elif [ "$status" = "INFO" ]; then
        echo -e "${BLUE}ℹ️  INFO${NC}: $message"
    fi
}

# Track overall status
OVERALL_STATUS="PASS"

echo ""
echo "📋 Checking Required Tools..."

# Check Docker
if command -v docker &> /dev/null; then
    docker_version=$(docker --version | cut -d' ' -f3 | cut -d',' -f1)
    print_status "PASS" "Docker is installed (version $docker_version)"
else
    print_status "FAIL" "Docker is not installed or not in PATH"
    OVERALL_STATUS="FAIL"
fi

# Check if Docker daemon is running
if docker info &> /dev/null; then
    print_status "PASS" "Docker daemon is running"
else
    print_status "FAIL" "Docker daemon is not running"
    OVERALL_STATUS="FAIL"
fi

# Check Git
if command -v git &> /dev/null; then
    git_version=$(git --version | cut -d' ' -f3)
    print_status "PASS" "Git is installed (version $git_version)"
else
    print_status "WARN" "Git is not installed (optional for running tests)"
fi

echo ""
echo "📁 Checking Project Structure..."

# Check critical files
critical_files=(
    "Dockerfile"
    "run_tests.sh"
    "Code/PocketMage_V3/platformio.ini"
    "Code/PocketMage_V3/src/TASKS.cpp"
    "Code/PocketMage_V3/test/test_tasks.cpp"
    "Code/PocketMage_V3/test/mocks/mock_hardware.h"
)

for file in "${critical_files[@]}"; do
    if [ -f "$file" ]; then
        print_status "PASS" "Found $file"
    else
        print_status "FAIL" "Missing $file"
        OVERALL_STATUS="FAIL"
    fi
done

# Check if run_tests.sh is executable
if [ -x "run_tests.sh" ]; then
    print_status "PASS" "run_tests.sh is executable"
else
    print_status "WARN" "run_tests.sh is not executable (run: chmod +x run_tests.sh)"
fi

echo ""
echo "🧪 Checking Test Environment..."

# Check if we can build the Docker image
echo "Building test Docker image (this may take a few minutes)..."
if docker build -t einkpda-health-check . &> /tmp/docker_build.log; then
    print_status "PASS" "Docker image builds successfully"
    
    # Try to run a quick test
    echo "Running quick test..."
    if timeout 60 docker run --rm einkpda-health-check &> /tmp/docker_run.log; then
        print_status "PASS" "Tests run successfully"
        
        # Extract test results
        test_count=$(grep -o '[0-9]\+ test cases' /tmp/docker_run.log | grep -o '[0-9]\+' || echo "unknown")
        if [ "$test_count" != "unknown" ]; then
            print_status "INFO" "Found $test_count test cases"
        fi
    else
        print_status "FAIL" "Tests failed to run (check /tmp/docker_run.log for details)"
        OVERALL_STATUS="FAIL"
    fi
    
    # Clean up test image
    docker rmi einkpda-health-check &> /dev/null || true
else
    print_status "FAIL" "Docker image failed to build (check /tmp/docker_build.log for details)"
    OVERALL_STATUS="FAIL"
fi

echo ""
echo "📊 Environment Summary..."

# Show system info
print_status "INFO" "Operating System: $(uname -s) $(uname -r)"
print_status "INFO" "Architecture: $(uname -m)"
print_status "INFO" "Available Memory: $(free -h | awk '/^Mem:/ {print $7}' 2>/dev/null || echo 'unknown')"
print_status "INFO" "Available Disk Space: $(df -h . | awk 'NR==2 {print $4}' 2>/dev/null || echo 'unknown')"

# Show Docker info if available
if command -v docker &> /dev/null && docker info &> /dev/null; then
    docker_mem=$(docker system df | awk '/^Images/ {print $4}' 2>/dev/null || echo 'unknown')
    print_status "INFO" "Docker images size: $docker_mem"
fi

echo ""
echo "🎯 Final Status..."

if [ "$OVERALL_STATUS" = "PASS" ]; then
    print_status "PASS" "Environment health check completed successfully!"
    echo ""
    echo "🚀 You can now run tests with: ./run_tests.sh"
    exit 0
else
    print_status "FAIL" "Environment health check failed!"
    echo ""
    echo "📝 Check the errors above and:"
    echo "   1. Install Docker if missing"
    echo "   2. Start Docker daemon if not running" 
    echo "   3. Fix any missing files"
    echo "   4. Check log files in /tmp/ for detailed error information"
    exit 1
fi 
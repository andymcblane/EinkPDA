#!/bin/bash

# Build and run tests for EinkPDA Tasks module
echo "Building Docker image for EinkPDA testing..."

# Build Docker image
docker build -t einkpda-tests .

if [ $? -ne 0 ]; then
    echo "❌ Docker build failed!"
    exit 1
fi

echo "✅ Docker image built successfully!"
echo ""
echo "Running tests..."
echo "=================="

# Run tests in container
docker run --rm einkpda-tests

echo "=================="
echo "Tests completed!" 
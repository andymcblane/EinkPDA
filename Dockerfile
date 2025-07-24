
FROM python:3.11-slim

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    git \
    curl \
    build-essential \
    gcc \
    g++ \
    make \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO
RUN pip install -U platformio

# Set working directory
WORKDIR /workspace

# Default command: run native tests
CMD ["platformio", "test", "-e", "native", "-v"] 
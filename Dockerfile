FROM python:3.11-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO (pinned version for reproducibility)
RUN pip install --upgrade platformio==6.1.17

# Set working directory
WORKDIR /workspace

# Copy only platformio.ini first to leverage Docker cache for dependencies
COPY Code/PocketMage_V3/platformio.ini /workspace/

# Install project dependencies (this layer will be cached unless platformio.ini changes)
RUN pio lib install

# Install test platform (cached unless PlatformIO version changes)
RUN pio platform install native

# Copy the rest of the project files (source code changes won't invalidate lib cache)
COPY Code/PocketMage_V3/ /workspace/
COPY scripts/ /workspace/scripts/

# Default command to run tests
CMD ["pio", "test", "-e", "native"] 

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

# Set workdir to project for platform and tool install
WORKDIR /workspace/Code/PocketMage_V3

# Copy only the project config for dependency install
COPY Code/PocketMage_V3/platformio.ini /workspace/Code/PocketMage_V3/platformio.ini
COPY Code/PocketMage_V3/platformio.ini /workspace/platformio.ini

# Install PlatformIO and required tools/libraries (after config is present)
RUN python3 -m pip install --upgrade pip && \
    pip install -U platformio && \
    platformio platform install native && \
    platformio platform install espressif32 && \
    platformio lib --global install "throwtheswitch/Unity@^2.6.0" && \
    platformio pkg install --tool "platformio/tool-scons@~4.40801.0"

# Copy the full project into the container
WORKDIR /workspace
COPY . /workspace
COPY Code/PocketMage_V3/test /workspace/test
COPY Code/PocketMage_V3/include /workspace/include
COPY Code/PocketMage_V3/src /workspace/src

# Default command: run native tests
CMD ["platformio", "test", "-e", "native", "-v"] 
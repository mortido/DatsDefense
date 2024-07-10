FROM ubuntu:22.04

# Update packages and install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake

RUN apt-get install -y libcurlpp-dev
RUN apt-get install -y pkg-config

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Moscow
RUN apt-get install -y tzdata

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Create a build directory
RUN mkdir build

# Build the project with CMake
RUN cmake -S . -B build -D CMAKE_BUILD_TYPE=Release && \
    cmake --build build --target mortido-bot -j$(nproc)

# Run the executable
CMD ["./build/mortido-bot"]
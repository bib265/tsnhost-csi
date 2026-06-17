# Use an Ubuntu base image (Ubuntu 24.04)
FROM ubuntu:24.04

# Set environment variables (optional)
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    libssl-dev \
    cmake gcc g++ git libpcre2-dev make build-essential pkg-config \
    libxml2-dev libprotobuf-c-dev libgcrypt-dev libboost-all-dev wget curl \
    libssh-dev libssh2-1-dev libcurl4-openssl-dev \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Clone and build libyang
WORKDIR /libyang
RUN git clone https://github.com/CESNET/libyang.git .
RUN mkdir build && cd build && cmake .. && make -j$(nproc) && make install

# Clone and build Sysrepo
WORKDIR /sysrepo
RUN git clone --recursive https://github.com/sysrepo/sysrepo.git .
RUN mkdir build && cd build && cmake .. && make -j$(nproc) && make install
RUN ldconfig

# Clone and build libnetconf2 (latest version)
WORKDIR /libnetconf2
RUN git clone https://github.com/CESNET/libnetconf2.git .
RUN rm -rf /usr/local/lib/libnetconf2.so*  # Remove old versions
RUN mkdir build && cd build && cmake .. && make -j$(nproc) && make install

# Clone and build Netopeer2
WORKDIR /netopeer2
RUN git clone https://github.com/CESNET/netopeer2.git .
RUN mkdir build && cd build && cmake .. && make -j$(nproc) && make install

# Add example event handler
WORKDIR /sysrepo/examples
COPY hello_world_event_handler.c /sysrepo/examples/hello_world_event_handler.c
RUN gcc hello_world_event_handler.c -o hello_world_event_handler -lsysrepo -pthread

COPY oven_event_handler.c /sysrepo/examples/oven_event_handler.c
RUN gcc oven_event_handler.c -o oven_event_handler -lsysrepo -pthread

# Install the YANG modules found inside the container
RUN sysrepoctl -i /sysrepo/tests/files/example-module.yang  \
    && sysrepoctl -i /sysrepo/examples/plugin/oven.yang 

# Ensure config directory exists and copy configuration files
COPY oven-config.xml /tmp/oven-config.xml
COPY example-config.xml /tmp/example-config.xml

# Update linker cache
RUN ldconfig



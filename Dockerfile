
FROM ubuntu:24.04


ENV DEBIAN_FRONTEND=noninteractive


WORKDIR /workspace

# Install required dependencies
RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libsystemd-dev \
    libssl-dev \
    libpcre3-dev \
    libpcre2-dev \
    libssh-dev \
    libprotobuf-c-dev \
    protobuf-c-compiler \
    libev-dev \
    libevent-dev \
    libcurl4-openssl-dev \
    libxml2-dev \
    python3 \
    python3-pip \
    uthash-dev \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# Clone and build required repositories
RUN git clone https://github.com/CESNET/libyang.git && \
    cd libyang && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DGEN_LANGUAGE_BINDINGS=ON .. && \
    make -j$(nproc) && make install && \
    ldconfig 

RUN git clone https://github.com/CESNET/libnetconf2.git && \
    cd libnetconf2 && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
    make -j$(nproc) && make install && \
    ldconfig 

RUN git clone https://github.com/sysrepo/sysrepo.git && \
    cd sysrepo && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
    make -j$(nproc) && make install && \
    ldconfig 

RUN git clone https://github.com/CESNET/netopeer2.git && \
    cd netopeer2 && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
    make -j$(nproc) && make install && \
    ldconfig 

RUN git clone https://github.com/onqtam/doctest.git && \
    cd doctest && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig

# Install dependencies inside /opt
WORKDIR /opt

# Clone and build libyang-cpp
RUN git clone https://github.com/CESNET/libyang-cpp.git && \
    cd libyang-cpp && \
    # Modify CMakeLists.txt to require libyang >= 3.7.8 instead of >= 3.7.11
    sed -i 's/pkg_check_modules(LIBYANG REQUIRED libyang>=3.7.11/pkg_check_modules(LIBYANG REQUIRED libyang>=3.7.8/' CMakeLists.txt && \
    sed -i 's/find_package(date)//g' CMakeLists.txt && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig

# Clone and build sdbus-cpp
RUN git clone https://github.com/Kistler-Group/sdbus-cpp.git && \
    cd sdbus-cpp && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig

# Clone trompeloeil
RUN git clone https://github.com/rollbear/trompeloeil.git && \
cd trompeloeil && \
mkdir build && cd build && \
cmake .. && \
make -j$(nproc) && make install && \
ldconfig

# Clone umgmt
RUN git clone https://github.com/sartura/umgmt.git && \
    cd umgmt && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig

# Clone and build sysrepo-cpp
RUN git clone https://github.com/sysrepo/sysrepo-cpp.git && \
    cd sysrepo-cpp && \
    sed -i 's/pkg_check_modules(SYSREPO REQUIRED sysrepo>=2.12.0 sysrepo<3 IMPORTED_TARGET)/pkg_check_modules(SYSREPO REQUIRED sysrepo>=2.12.0 IMPORTED_TARGET)/g' CMakeLists.txt && \  
    # Modify CMakeLists.txt
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig

# Clone sysrepo-plugins-common (cpp branch)
RUN git clone --branch cpp https://github.com/telekom/sysrepo-plugins-common.git && \
    cd sysrepo-plugins-common && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && make install && \
    ldconfig





# Clone sysrepo-plugins but do not install it
RUN git clone https://github.com/telekom/sysrepo-plugins.git /opt/sysrepo-plugins

# Create the cmake directory inside sysrepo-plugins/plugins/ietf-system-plugin/
RUN mkdir -p /opt/sysrepo-plugins/plugins/ietf-system-plugin/cmake && \
    cp -r /opt/sysrepo-plugins/CMakeModules/* /opt/sysrepo-plugins/plugins/ietf-system-plugin/cmake/ && \
    ldconfig
    


# Dynamically get SYSTEMD_IFINDEX and run cmake (but do NOT build)
RUN mkdir -p /opt/sysrepo-plugins/plugins/ietf-system-plugin/build && cd /opt/sysrepo-plugins/plugins/ietf-system-plugin/build && \
    SYSTEMD_IFINDEX=$(ip link | awk '/eth0/ {print $1}' | sed 's/://') && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DSYSTEMD_IFINDEX=$SYSTEMD_IFINDEX \
    -DCMAKE_MODULE_PATH=/opt/sysrepo-plugins/plugins/ietf-system-plugin/cmake
    

# Run ldconfig again at the end to finalize shared libraries
RUN ldconfig

# Set the working directory back to sysrepo
WORKDIR /opt/sysrepo-plugins/plugins/ietf-system-plugin

# Set the default command
CMD ["/bin/bash"]

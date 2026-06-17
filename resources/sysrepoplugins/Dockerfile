#
# Linux TSN Host with netopeer2 and sysrepo
#

FROM ubuntu:noble
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends --fix-missing \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    less \
    libcurl4-openssl-dev \
    libnl-3-dev\
    libnl-genl-3-dev \
    libnl-nf-3-dev \
    libnl-route-3-dev \
    libpcre2-dev \
    libssh-dev \
    libssl-dev \
    libsystemd-dev \
    nano \
    pkgconf \
    valgrind \
    vim


# libyang
##########
RUN git clone -b v3.4.2 --single-branch https://github.com/CESNET/libyang.git

RUN cmake -B /libyang/build -S /libyang && \
    make -C /libyang/build && \
    make -C /libyang/build install && \
    ldconfig


# libyangcpp
#############
RUN git clone -b v3 --single-branch https://github.com/CESNET/libyang-cpp.git 

RUN cmake -B /libyang-cpp/build -S /libyang-cpp -DBUILD_TESTING=OFF && \
    make -C /libyang-cpp/build && \
    make -C /libyang-cpp/build install && \
    ldconfig


# sdbus-cpp
############
RUN git clone -b v2.1.0 --single-branch https://github.com/Kistler-Group/sdbus-cpp.git

RUN cmake -B /sdbus-cpp/build -S /sdbus-cpp -D BUILD_TESTING=OFF && \
    make -C /sdbus-cpp/build && \
    make -C /sdbus-cpp/build install && \
    ldconfig


# sysrepo
##########
RUN git clone -b v2.11.7 --single-branch https://github.com/sysrepo/sysrepo.git

# Set custom repository path:
# -DREPO_PATH=/opt/sysrepo/my_repository
RUN cmake -B /sysrepo/build -S /sysrepo -DREPO_PATH=/srv/sysrepo/repo && \
    make -C /sysrepo/build && \
    make -C /sysrepo/build install && \
    ldconfig
# missing systemd support because libsystemd not found?
# Could NOT find LibSystemd (missing: SYSTEMD_UNIT_DIR)


# sysrepo-cpp
##############
RUN git clone -b v3 --single-branch https://github.com/sysrepo/sysrepo-cpp.git

RUN cmake -B /sysrepo-cpp/build -S /sysrepo-cpp -D BUILD_TESTING=OFF && \
    make -C /sysrepo-cpp/build && \
    make -C /sysrepo-cpp/build install && \
    ldconfig


# umgmt
########
RUN git clone https://github.com/sartura/umgmt.git && \
    cd umgmt && \
    git checkout dce4c30e4212f5bd8d17dff573851c4e5dc659e8 && \
    git submodule init && \
    git submodule update

RUN cmake -B /umgmt/build -S /umgmt && \
    make -C /umgmt/build && \
    make -C /umgmt/build install && \
    ldconfig


# sysrepo-plugins-common
#########################
RUN git clone https://github.com/telekom/sysrepo-plugins-common.git && \
    cd sysrepo-plugins-common && \
    git checkout 20885de0d3bb95a05610fdb3a0f83d8f7c370fad && \
    git submodule init && \
    git submodule update

RUN cmake -B /sysrepo-plugins-common/build -S /sysrepo-plugins-common && \
    make -C /sysrepo-plugins-common/build && \
    make -C /sysrepo-plugins-common/build install && \
    ldconfig


# sysrepo-plugins
##################
#RUN git clone -b v0.1.3 --single-branch https://github.com/telekom/sysrepo-plugins.git
RUN git clone https://github.com/telekom/sysrepo-plugins.git

RUN cmake -B /sysrepo-plugins/build -S /sysrepo-plugins -D SYSTEMD_IFINDEX=1 && \
    make -C /sysrepo-plugins/build 


# RUN git clone -b v1.7.3 --single-branch https://github.com/sysrepo/sysrepo-python.git
# RUN pip install --break-system-packages /sysrepo-python
# sysrepo-python downloads libyang 3.0.1 again and uses it...


# libnetconf2
##############
RUN git clone -b v3.5.1 --single-branch https://github.com/CESNET/libnetconf2.git

RUN cmake -B /libnetconf2/build -S /libnetconf2 && \
    make -C /libnetconf2/build && \
    make -C /libnetconf2/build install && \
    ldconfig


# netopeer2
############
RUN git clone -b v2.2.31 --single-branch https://github.com/CESNET/netopeer2.git

RUN cmake -B /netopeer2/build -S /netopeer2 && \
    make -C /netopeer2/build && \
    make -C /netopeer2/build install


# useradd -b /home -m -p netconf netconf
# openssh-client
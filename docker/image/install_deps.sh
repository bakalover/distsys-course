#!/bin/sh

set -e -x

apt-get update

DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata

apt-get install -y \
	wget \
	gpg \
	software-properties-common

# Kitware APT repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
apt-get update

apt-get install -y \
	cmake \
	ninja-build \
	ssh \
	git \
	clang-12 \
	clang-format-12 \
	clang-tidy-12 \
	lldb-12 \
	python3 \
	python3-pip \
	python3-venv \
	ca-certificates \
	openssh-server \
	rsync \
	vim \
	gdb \
	autoconf \
	iputils-ping \
	binutils-dev

pip3 install \
	click \
	gitpython \
	python-gitlab \
	termcolor \
	virtualenv \
	argcomplete

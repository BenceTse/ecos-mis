#!/bin/bash
if test $(id -u)!=0
then
    SUDO=sudo
fi
function version_lt {
    test $1 != $(echo -e "$1\n$2" | sort -rV | head -n 1)
}

function ensure_decent_gcc_on_rh {
    local old=$(gcc -dumpversion)
    local expected=5.0
    #local dts_ver=$1
    if version_lt $old $expected; then
	if test -t 1; then
        #将GCC版本提升为gcc7
        $SUDO yum install -y centos-release-scl
        $SUDO yum install -y devtoolset-7
	    # interactive shell
	    cat << EOF
*************************************************************************************
Your GCC is too old. Please run following command to add DTS to your environment:

scl enable devtoolset-7 bash

Or add following line to the end of ~/.bashrc to add it permanently:

source scl_source enable devtoolset-7

see https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/ for more details.
*************************************************************************************
EOF
    echo -n "Enter any key to continue:"
    read anykey
	#else
	    # non-interactive shell
	#    source /opt/rh/devtoolset-$dts_ver/enable
	fi
    fi
}

source /etc/os-release
case $ID in
    ubuntu)
        echo "Using apt-get to install dependencies"
        $SUDO apt-get install -y cmake g++ \
        libgflags-dev;;
        #libcurl4-openssl-dev libgflags-dev uuid-dev libev-dev libevent-dev libssl-dev;;
	   
    centos)
        echo "Using yum to install dependencies"
        $SUDO yum install -y epel-release 
        #将CMak提升为Cmake3
        $SUDO yum install -y gcc-c++ cmake3 \
        gflags-devel 
        #libcurl-devel gflags-devel libuuid-devel libevent-devel libev-devel openssl-devel
        ensure_decent_gcc_on_rh;;
    *)
        echo "NOT SUPPORT THE OS";;
esac

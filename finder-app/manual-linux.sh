#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
ROOTDIR=${OUTDIR}/rootfs
HOMEDIR=${ROOTDIR}/home

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

    cp arch/${ARCH}/boot/Image ${OUTDIR}
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${ROOTDIR}" ]
then
	echo "Deleting rootfs directory at ${ROOTDIR} and starting over"
    sudo rm  -rf ${ROOTDIR}
else
    echo "Creating rootfs directory at ${ROOTDIR}"
    mkdir -p ${ROOTDIR}
fi

# TODO: Create necessary base directories
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

# TODO: Make and install busybox
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
interpretor=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk '{print $NF}' | tr -d '[]')
libs=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk '{print $NF}' | tr -d '[]')

# TODO: Add library dependencies to rootfs
$files="$libs $interpretor"
DEST_DIR="${ROOTDIR}/lib"
mkdir -p $DEST_DIR

for file in $files do;
    if [ -f $file ]; then
        cp $file $DEST_DIR
    fi
done

# TODO: Make device nodes
sudo mknode -m 666 ${ROOTDIR}/dev/null c 1 3
sudo mknode -m 666 ${ROOTDIR}/dev/null c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp "./writer" ${HOMEDIR}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp "./finder.sh ./conf/username.txt ./conf/assignment.txt ./finder-test.sh autorun-qemu.sh" ${HOMEDIR}

# TODO: Chown the root directory
chown -R root:root ${ROOTDIR}

# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}
find . | cpio -H newc -ov --owner root:root > ./initramfs.cpio
gzip -f ./initramfs.cpio
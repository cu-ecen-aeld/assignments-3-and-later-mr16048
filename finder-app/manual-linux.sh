#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE="$FINDER_APP_DIR/../arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-"

ARM_DIR="$FINDER_APP_DIR/../arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc"
ARM_LIB_DIR=${ARM_DIR}/lib64/
ARM_INTERPRETER=${ARM_DIR}/lib

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
    git checkout "${KERNEL_VERSION}" || { echo "Failed to checkout version ${KERNEL_VERSION}"; exit 1; }
    # if [ ! -e ${GIT_PATCH_PATH} ]; then
        # echo "Downloading the patch"
        # GIT_PATCH_PATH="${OUTDIR}/git_patch.patch"
        # curl -o "${GIT_PATCH_PATH}" https://github.com/torvalds/linux/commit/e33a814e772cdc36436c8c188d8c42d019fda639.patch    

        # echo "Applying the patch"
        # git apply "${GIT_PATCH_PATH}" || { echo "Failed to apply patch"; exit 1; }
    # fi

    echo "Building the kernel"

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

ROOTDIR=${OUTDIR}/rootfs
HOMEDIR=${ROOTDIR}/home

if [ -d "${ROOTDIR}" ]
then
	echo "Deleting rootfs directory at ${ROOTDIR} and starting over"
    sudo rm  -rf ${ROOTDIR}
else
    echo "Creating rootfs directory at ${ROOTDIR}"
    # mkdir -p ${ROOTDIR}
fi

# TODO: Create necessary base directories
mkdir -p ${ROOTDIR}
cd ${ROOTDIR}
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
make CONFIG_PREFIX=${ROOTDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ${ROOTDIR}

echo "Library dependencies"
interpretor=$(${CROSS_COMPILE}readelf -a ${ROOTDIR}/bin/busybox | grep "program interpreter" | awk '{print $NF}' | tr -d '[]')
libs=$(${CROSS_COMPILE}readelf -a ${ROOTDIR}/bin/busybox | grep "Shared library" | awk '{print $NF}' | tr -d '[]')

# TODO: Add library dependencies to rootfs
DEST_DIR_I="${ROOTDIR}/lib"
mkdir -p $DEST_DIR_I
cp "${ARM_DIR}${interpretor}" $DEST_DIR_I

DEST_DIR_L="${ROOTDIR}/lib64"
mkdir -p $DEST_DIR_L
for file in $libs; do
    # if [ -f $file ]; then
        cp "${ARM_LIB_DIR}${file}" $DEST_DIR_L
    # fi
done

# TODO: Make device nodes
rm -rf ${ROOTDIR}/dev/*
sudo mknod -m 666 ${ROOTDIR}/dev/null c 1 3
sudo mknod -m 666 ${ROOTDIR}/dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp "./writer" ${HOMEDIR}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
ls
cp ./finder.sh ./conf/username.txt ./conf/assignment.txt ./finder-test.sh ./autorun-qemu.sh ${HOMEDIR}

# TODO: Chown the root directory
chown -R root:root ${ROOTDIR}

# TODO: Create initramfs.cpio.gz
cd ${ROOTDIR}
find . | cpio -H newc -ov --owner root:root > ./initramfs.cpio
gzip -f ./initramfs.cpio
mv ./initramfs.cpio.gz ${OUTDIR}

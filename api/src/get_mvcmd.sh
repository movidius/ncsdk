#! /bin/bash
#
# Attempt to download tar file to get firmware.
# If download fails, restores saved firmware if the NCSDK installed version in /opt/movidius matches NCSDK version of this distribution.
# If download fails and versions mismatch, then existing firmware isn't restored.
#

# read in functions shared with installer 
if [ -f ../../install-utilities.sh ] ; then
    source ../../install-utilities.sh
    # function initialize_constants() from install-utilities.sh
    initialize_constants
else
    echo "Warning cannot find file install-utilities.sh.  Will exit."
    exit 0
fi


mvcmd_dir=./mvnc
mvcmd_old_dir=./mvnc.old

VERSION_FILE="../../version.txt"
if [ ! -f ${VERSION_FILE} ] ; then
    # see if local copy of version.txt
    if [ -f ./version.txt ] ; then
        VERSION_FILE="./version.txt"
    else
        echo -e "${RED}Error cannot find version file version.txt."
        echo -e "Error on line $LINENO.  Will exit ${NC}."
        exit 1
    fi
fi
NCSDK_API_VERSION=$(cat ${VERSION_FILE})
download_filename="NCSDK-${NCSDK_API_VERSION}.tar.gz"


function restore_old_fw()
{
    # remove the tar file if downloaded
    [ -f ./${download_filename} ] && rm -rf ./${download_filename}

    # Require old FW to exist
    if [ ! -d ${mvcmd_old_dir} ] ; then
        echo "Error restoring old firmware - directory ${mvcmd_old_dir} doesn't exist.  Will exit"
        exit 1
    fi
    
    # Remove FW directory if it exists
    [ -d ${mvcmd_dir} ] && rm -rf ${mvcmd_dir}

    # check version of installed NCSDK vs. version of API
    NCSDK_installed_version_file="/opt/movidius/version.txt"
    if [ -f ${NCSDK_installed_version_file} ] ; then
        NCSDK_INSTALLED_VERSION=$(cat ${NCSDK_installed_version_file} )

        # compare installed version with API
        if [ "${NCSDK_API_VERSION}" = "${NCSDK_INSTALLED_VERSION}" ] ; then
            mv -f ${mvcmd_old_dir} ${mvcmd_dir}
            echo "Warning: Restored old firmware"
        else
            echo "Error: Old firmware version=${NCSDK_API_VERSION} doesn't match installed NCSDK version ${NCSDK_INSTALLED_VERSION}."
            echo "       Not restoring old firmware directory, ${mvcmd_old_dir}.  Will exit"
            exit 1
        fi
        
    else
        echo "Warning: Cannot find NCSDK installed version file ${NCSDK_installed_version_file}."
        mv -f ${mvcmd_old_dir} ${mvcmd_dir}
        echo "Warning: Restored old firmware and exiting"
    fi

    exit 0
}


# If exists, rename existing FW 
[ -d ${mvcmd_old_dir} ] && rm -rf ${mvcmd_old_dir}
[ -d ${mvcmd_dir} ] && mv -f ${mvcmd_dir} ${mvcmd_old_dir}


# download file if not already available
if [ -f ../../${download_filename} ] ; then
    [ -f ${download_filename} ] && rm -f ${download_filename}
    ln -s ../../${download_filename} .
else
    # download, ncsdk_download_link set in install-utilities.sh function initialize_constants()
    wget -q --no-cache -O ${download_filename} ${ncsdk_download_link}
    if [ $? -ne 0 ]; then
        echo "Error downloading $ncsdk_download_link.  Will try to restore existing firmware"
        restore_old_fw
    fi
fi

# ncsdk_pkg is the filename without the .tar.gz extension
ncsdk_pkg=${download_filename%%.tar.gz}

# create mvnc directory 
mkdir -p ${mvcmd_dir}

# set owner to the person that called sudo
if [ "${SUDO_USER}" == "" ]
then
   echo ""
else
   chown ${SUDO_USER} ${mvcmd_dir}
   chgrp ${SUDO_USER} ${mvcmd_dir}
fi


# search for FW in tarball
ARCH=$(uname -m)
tar -tf ${download_filename} | grep -q "ncsdk-${ARCH}/fw/$"
if [ $? -ne 0 ] ; then
    echo "Error firmware, ncsdk-${ARCH}/fw/, not found in tar file ${download_filename}.  Will try to restore existing firmware"
    restore_old_fw
fi


tar -xf ${download_filename} --strip-components=3 -C ${mvcmd_dir} NCSDK-${NCSDK_API_VERSION}/ncsdk-${ARCH}/fw/
if [ $? -ne 0 ]; then
   echo "Error extracting tar file ${download_filename}.  Will try to restore existing firmware"
   restore_old_fw
fi

# check for FW
ls mvnc/*.mvcmd >& /dev/null
if [ $? -eq 0 ]; then
    echo "NCSDK FW successfully installed"

    # set owner to the person that called sudo
    if [ "${SUDO_USER}" == "" ]
    then
        echo ""
    else
        chown ${SUDO_USER} mvnc/*.mvcmd
        chgrp ${SUDO_USER} mvnc/*.mvcmd
    fi

    # remove old FW dir if it exists
    [ -d ${mvcmd_old_dir} ] && rm -rf ${mvcmd_old_dir}
else
    echo "ERROR NCSDK FW not found after downloading.  Will try to restore existing firmware"
    restore_old_fw
fi

# remove the tar file
rm -rf ${download_filename}

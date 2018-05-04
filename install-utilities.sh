#!/bin/bash
#
# Movidius Neural Compute Toolkit install utilities
# This file contains functions only sourced by install.sh and uninstall.sh.
# This script cannot be executed directly.
# 
# Please provide feedback in our support forum if you encountered difficulties.
################################################################################
# require this script to be sourced and not executed directly - requires executing script name, $0,
# isn't the script name which it would if executed (i.e. run), not sourced
RC=0
echo $0  | grep install-utilities.sh > /dev/null || RC=$?
if [ ${RC} -eq 0 ] ; then
    echo "USAGE:"
    echo "This script shouldn't be executed/called directly, as it only contains functions to be called from install.sh and uninstall.sh"
    echo "If you want to install, run 'make install' and if you want to uninstall, run 'make uninstall'"
    echo "This script must be sourced.  Will exit 1"
    exit 1
fi


# error_report - function that gets called if an error is detected
function error_report()
{
    echo -e "${RED}Installation failed. Error on line $1${NC}"
    exit 1
}


# set_error_handling - enable catching errors 
function set_error_handling()
{
    ### set error handling
    # -e Exit immediately if a command exits with a non-zero status.
    # -u  Treat unset variables as an error when substituting.
    set -e
    set -u
    # Enable ERR trap handling
    trap 'error_report $LINENO' ERR
    # If bash version >=3, use enhanced error trapping with pipefail and errtrace
    [ "${BASH_VERSINFO:-0}" -ge 3 ] && set -o pipefail && set -o errtrace
}


# initialize_constants - initialize constants
function initialize_constants()
{
    # avoid conflicts with PYTHONPATH during install
    export PYTHONPATH=""
    
    # File used to find install location in case install dir doesn't match INSTALL_DIR
    INSTALL_INFO_FILENAME=.ncsdk_install.info
    # set colours 
    RED='\e[31m'
    GREEN='\e[32m'
    YELLOW='\e[33m'
    NC='\e[m'

    # Directory NCSDK 1.x release will be moved to
    NCSDK1_ARCHIVE_DIR=/opt/movidius/ncsdk1
}


# ask_sudo_permissions - 
# Sets global variables: SUDO_PREFIX, PIP_PREFIX
function ask_sudo_permissions()
{
    ### If the executing user is not root, ask for sudo priviledges
    SUDO_PREFIX=""
    PIP_PREFIX=""
    if [ $EUID != 0 ]; then
        SUDO_PREFIX="sudo"
        sudo -v
        if [ "${PIP_SYSTEM_INSTALL}" = "yes" ]; then
            PIP_PREFIX="$SUDO_PREFIX -H -E"
        fi
        # Keep-alive: update existing sudo time stamp if set, otherwise do nothing.
        while true; do sudo -n true; sleep 60; kill -0 "$$" || exit; done 2>/dev/null &
    fi    
}


# read_ncsdk_config - reads user configuration file, ncsdk.conf
# Sets global variables: INSTALL_DIR, INSTALL_CAFFE, CAFFE_FLAVOR, CAFFE_USE_CUDA, 
#                        INSTALL_TENSORFLOW, INSTALL_TOOLKIT, PIP_SYSTEM_INSTALL, VERBOSE
function read_ncsdk_config()
{
    # check if file exist
    CONF_FILE=./ncsdk.conf
    if [ ! -f ${CONF_FILE} ] ; then
        echo -e "${RED}Couldn't find file CONF_FILE=${CONF_FILE}.  Will exit${NC}"
        exit 1
    fi

    ### Set default values in case not they are not set in ncsdk.conf 
    #  Any or all of these can be overridden by adding to ncsdk.conf
    INSTALL_DIR=/opt/movidius
    INSTALL_CAFFE=yes
    CAFFE_FLAVOR=bvlc
    CAFFE_USE_CUDA=no
    INSTALL_TENSORFLOW=yes
    INSTALL_TOOLKIT=yes
    PIP_SYSTEM_INSTALL=yes
    USE_VIRTUALENV=no
    VERBOSE=no
    # check if nproc is available on the system via 'command'
    RC=0
    command -v nproc > /dev/null || RC=$?
    if [ $RC -eq 0 ] ; then
        #  default to using all processors to invoke make with (make -j $MAKE_NJOBS)
        MAKE_NJOBS=$(nproc)
    else
        # Can't find nproc, default to 1 make process. Can change in ncsdk.conf
        MAKE_NJOBS=1
    fi
    
    ### check configuration file, and discard any malformed lines
    count=0
    mal_count=0
    while IFS='' read -r line || [[ -n "$line" ]]; do
        count=$((count+1))
        if ! [[ $line =~ ^[^=]*+=[^=]*+$ ]]; then
            mal_count=$((mal_count+1))
            echo "Malformed line at line no. $count.  Problem line=$line"
        fi
    done < ${CONF_FILE}

    if [ "$mal_count" -gt "0" ]; then
        echo -e "${RED}Found $mal_count errors in ${CONF_FILE}. Please fix before running install.${NC}"
        exit 1
    fi

    ### Config file error check done, source it
    echo "Reading installer configuration variables from $CONF_FILE"
    source $CONF_FILE
}


# find_previous_install - Attempt to find previous installation, looks first in
#                               ${INSTALL_DIR}, then searches all of /opt.
# sets globals PREV_INSTALL_INFO, PREV_INSTALL_OWNER, PREV_INSTALL_DIR, PREV_NCSDK_VER
#              PREV_NCSDK_PATH, PREV_NCS_BIN_PATH, PREV_NCS_LIB_PATH, PREV_NCS_INC_PATH
function find_previous_install()
{
    echo ""
    PREV_NCSDK_PATH="unknown"
    PREV_NCS_BIN_PATH="unknown"
    PREV_NCS_LIB_PATH="unknown"
    PREV_NCS_INC_PATH="unknown"
    # first look for prev install in INSTALL_DIR
    PREV_INSTALL_INFO=${INSTALL_DIR}/${INSTALL_INFO_FILENAME}
    if [ ! -f "${PREV_INSTALL_INFO}" ] ; then
        # file INSTALL_INFO_FILENAME not found in ${INSTALL_DIR}, so search for it in /opt via find
        PREV_INSTALL_INFO="unknown"
        if [ -d /opt ] ; then
            PREV_INSTALL_INFO=`$SUDO_PREFIX find /opt -name $INSTALL_INFO_FILENAME -print 2> /dev/null`
            if [ ! -f "${PREV_INSTALL_INFO}" ] ; then
                PREV_INSTALL_INFO="unknown"
            fi
        fi
    fi
    if [ -f "${PREV_INSTALL_INFO}" ] ; then
        PREV_INSTALL_OWNER=$(ls -l $PREV_INSTALL_INFO | awk '{print $3}')
        CURRENT_USER=$(id -u -n)
        if [ "$PREV_INSTALL_OWNER" != "${CURRENT_USER}" ]; then
            echo "Warning: Previous installation not owned by current user"
        fi
        PREV_INSTALL_DIR=${PREV_INSTALL_INFO%/*}
        if [ -f $PREV_INSTALL_DIR/version.txt ] ; then
            PREV_NCSDK_VER=`cat $PREV_INSTALL_DIR/version.txt`
            echo "Found NCSDK version $PREV_NCSDK_VER previously installed at $PREV_INSTALL_DIR"
        else
            echo "Warning: Couldn't find previous NCSDK version number"
        fi

        # verify ncsdk_path is in $PREV_INSTALL_INFO & it's a valid directory, else set unknown
        RC=0
        grep 'ncsdk_path' $PREV_INSTALL_INFO 2>/dev/null || RC=$?
        if [ $RC -eq 0 ] ; then
            PREV_NCSDK_PATH=$(grep 'ncsdk_path' $PREV_INSTALL_INFO | cut -d '=' -f 2)
            [ ! -d "${PREV_NCSDK_PATH}" ] && PREV_NCSDK_PATH="unknown"
        else
            PREV_NCSDK_PATH="unknown"
        fi      
        # verify ncs_bin_path is in $PREV_INSTALL_INFO & it's a valid directory, else set unknown
        RC=0
        grep 'ncs_bin_path' $PREV_INSTALL_INFO 2>/dev/null || RC=$?
        if [ $RC -eq 0 ] ; then
            PREV_NCS_BIN_PATH=$(grep 'ncs_bin_path' $PREV_INSTALL_INFO | cut -d '=' -f 2)
            [ ! -d "${PREV_NCS_BIN_PATH}" ] && PREV_NCS_BIN_PATH="unknown"
        else
            PREV_NCS_BIN_PATH="unknown"
        fi
        # verify ncs_lib_path is in $PREV_INSTALL_INFO & it's a valid directory, else set unknown
        RC=0
        grep 'ncs_lib_path' $PREV_INSTALL_INFO 2>/dev/null || RC=$?
        if [ $RC -eq 0 ] ; then
            PREV_NCS_LIB_PATH=$(grep 'ncs_lib_path' $PREV_INSTALL_INFO | cut -d '=' -f 2)
            [ ! -d "${PREV_NCS_LIB_PATH}" ] && PREV_NCS_LIB_PATH="unknown"
        else
            PREV_NCS_LIB_PATH="unknown"
        fi
        # verify ncs_inc_path is in $PREV_INSTALL_INFO & it's a valid directory, else set unknown
        RC=0
        grep 'ncs_inc_path' $PREV_INSTALL_INFO 2>/dev/null || RC=$?
        if [ $RC -eq 0 ] ; then
            PREV_NCS_INC_PATH=$(grep 'ncs_inc_path' $PREV_INSTALL_INFO | cut -d '=' -f 2)
            [ ! -d "${PREV_NCS_INC_PATH}" ] && PREV_NCS_INC_PATH="unknown"
        else
            PREV_NCS_INC_PATH="unknown"
        fi
        
        if [ "${VERBOSE}" = "yes" ] ; then
            echo "Previously installed NCSDK files are at: $PREV_NCSDK_PATH"
            echo "Previously installed NCSDK binaries are at: $PREV_NCS_BIN_PATH"
            echo "Previously installed NCSDK libraries are at: $PREV_NCS_LIB_PATH"
            echo "Previously installed NCSDK include files are at: $PREV_NCS_INC_PATH"
        fi
    else
        [ "${VERBOSE}" = "yes" ] && echo "Didn't find a valid NCSDK installation"
        return 0  # required to prevent function returning 1 if $VERBOSE != yes, which would cause error trap & install to exit
    fi
}


# check_and_remove_tk_file - remove Toolkit (TK) files
function check_and_remove_tk_file()
{
    if [ -e "$PREV_NCS_BIN_PATH/$1" ]; then
        RECURSIVE=""
        if [ -d "$PREV_NCS_BIN_PATH/$1" ]; then
            RECURSIVE="-r"
        fi
        if [ "${VERBOSE}" = "yes" ] ; then
            printf "Removing NCSDK toolkit file..."
            echo "$PREV_NCS_BIN_PATH/$1"
        fi
        $SUDO_PREFIX rm $RECURSIVE "$PREV_NCS_BIN_PATH/$1"
    fi
}


# check_and_remove_file - checks if a file exists, then rm it
function check_and_remove_file()
{
    if [ -e "$1" ]; then
        $SUDO_PREFIX rm "$1"
    else
        # remove symbolic links
        RC=0
        [ -L "$1" ] || RC=$?
        if [ ${RC} -eq 0 ] ; then
            $SUDO_PREFIX rm "$1"
        fi
    fi
}

# check_and_remove_files - checks if a file or director exists, then rm -r it
function check_and_remove_files()
{
    if [ -e "$1" ]; then
        $SUDO_PREFIX rm -r "$1"
    fi
}


# check_and_remove_pip_pip3_pkg - removes pip2/pip3 package if installed
#   argument 1 = pip package to check and remove
#  
function check_and_remove_pip_pkg()
{
    pip_pkg=$1
    for PIP in pip3 pip2; do
        # check if ${PIP} is available on the system via 'command'
        RC=0
        command -v ${PIP} > /dev/null || RC=$?
        if [ $RC -ne 0 ] ; then
            echo -e "${RED} Command PIP=${PIP} not found.  Will exit${NC}"
            exit 1
        fi    
        RC=0
        $PIP_PREFIX ${PIP} show ${pip_pkg} 1> /dev/null || RC=$?
        if [ $RC -eq 0 ]; then
            $PIP_PREFIX ${PIP} uninstall -y ${pip_pkg}
        fi
    done
}


# exec_and_search_errors - execute 1st argument and 1) check for error message
#      related to network connectivity issues, then 2) check all other errors
function exec_and_search_errors()
{
    # Execute the incoming command in $1
    RC_INITIAL=0
    TMP_FILE=/tmp/tmp_cmd_ouput.txt
    $1 |& tee ${TMP_FILE} || RC_INITIAL=$?
    fileout=$(<${TMP_FILE})
    RC=0
    echo "$fileout" | grep -q -e "Temporary failure" || RC=$?
    # First check if command failed due to network issues
    if [ $RC -eq 0 ]; then
        echo -e "${RED}Installation failed: Unable to reach remote servers, check your network and/or proxy settings and try again. Error on line $LINENO. ${NC}"
        exit 128
    fi
    # Next check if command failed for all other reasons
    if [ $RC_INITIAL -ne 0 ]; then
        echo -e "${RED}Installation failed: Command '$1' return code=${RC_INITIAL}. Error on line $LINENO in ${BASH_SOURCE[0]}.  Will exit ${NC}"
        exit 131
    fi
    if [ -f ${TMP_FILE} ] ; then
        rm ${TMP_FILE}
    fi
}


# Find if previous install was a NCSDK 1.x release and move it to ${NCSDK1_ARCHIVE_DIR}
function detect_and_move_ncsdk1()
{
    MOVED_NCAPI1="no"
    # check if existing install in NCSDK 1.x release or not
    [ -f ${INSTALL_DIR}/version.txt ] && egrep ^1 ${INSTALL_DIR}/version.txt >& /dev/null && FOUND_NCSDK1="Yes" || FOUND_NCSDK1="No"
    if [ "${FOUND_NCSDK1}" = "Yes" ] ; then
        echo -e "${YELLOW}"
        echo "Warning: NCSDK 1.x was detected on this system."
        echo "If you continue with this installation of NCSDK 2.x your existing "
        echo "projects using NCAPI v1 will no longer work unless you do one of the following:"
        echo "  - Manually update projects to use NCAPI v2."
        echo "  - Manually configure your projects to use the old NCAPI v1 files which will be"
        echo "    moved to ${NCSDK1_ARCHIVE_DIR}:" 
        echo ""
        read -p "Do you wish to continue installing Movidius Neural Compute Toolkit Installation (y/n) ? " CONTINUE
        echo -e "${NC}"
        if [[ "$CONTINUE" != "y" && "$CONTINUE" != "Y" ]]; then
            echo -e "${RED}Based on user input, not continuing with installation${NC}"
            exit 1            
        fi

        # Prompt user if they want to remove old NCSDK API 1 directory 
        if [ -d ${NCSDK1_ARCHIVE_DIR} ] ; then
            echo -e "${YELLOW}Warning: Detected an existing NCSDK 1.x directory at ${NCSDK1_ARCHIVE_DIR}"
            echo -e "You can exit and see if you want to delete this directory and re-run make install"
            read -p "Delete this directory and continue installing (y/n) ? " CONTINUE
            echo -e "${NC}"
            if [[ "$CONTINUE" != "y" && "$CONTINUE" != "Y" ]]; then
                echo -e "${RED}Based on user input, not continuing with installation${NC}"
                exit 1
            else
                echo "Removing ${NCSDK1_ARCHIVE_DIR}"
                ${SUDO_PREFIX} rm -rf ${NCSDK1_ARCHIVE_DIR}
            fi
        fi
        # Create NCSDK API 1 system directories
        ${SUDO_PREFIX} mkdir -p ${NCSDK1_ARCHIVE_DIR}/bin
        ${SUDO_PREFIX} mkdir -p ${NCSDK1_ARCHIVE_DIR}/include
        ${SUDO_PREFIX} mkdir -p ${NCSDK1_ARCHIVE_DIR}/lib/mvnc
        ${SUDO_PREFIX} mkdir -p ${NCSDK1_ARCHIVE_DIR}/api

        ## Move NCSDK 1.x Toolkit binaries
        [ -d ${SYS_INSTALL_DIR}/bin/ncsdk ] && ${SUDO_PREFIX} mv ${SYS_INSTALL_DIR}/bin/ncsdk ${NCSDK1_ARCHIVE_DIR}/bin
        # remove toolkit binaries old soft links
        check_and_remove_file ${SYS_INSTALL_DIR}/bin/mvNCCheck
        check_and_remove_file ${SYS_INSTALL_DIR}/bin/mvNCCompile 
        check_and_remove_file ${SYS_INSTALL_DIR}/bin/mvNCProfile
        # Create new toolkit binaries soft links
        check_and_remove_file ${NCSDK1_ARCHIVE_DIR}/bin/mvNCCheck
        check_and_remove_file ${NCSDK1_ARCHIVE_DIR}/bin/mvNCCompile
        check_and_remove_file ${NCSDK1_ARCHIVE_DIR}/bin/mvNCProfile
        [ -f ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCCheck.py ]   && $SUDO_PREFIX ln -s ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCCheck.py   ${NCSDK1_ARCHIVE_DIR}/bin/mvNCCheck
        [ -f ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCCompile.py ] && $SUDO_PREFIX ln -s ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCCompile.py ${NCSDK1_ARCHIVE_DIR}/bin/mvNCCompile
        [ -f ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCProfile.py ] && $SUDO_PREFIX ln -s ${NCSDK1_ARCHIVE_DIR}/bin/ncsdk/mvNCProfile.py ${NCSDK1_ARCHIVE_DIR}/bin/mvNCProfile

        ## Move NCSDK 1.x C/C++ API header file 
        [ -f ${SYS_INSTALL_DIR}/include/mvnc.h ] && $SUDO_PREFIX mv ${SYS_INSTALL_DIR}/include/mvnc.h ${NCSDK1_ARCHIVE_DIR}/include

        ## Move NCSDK 1.x C/C++ API lib - remove old soft links, move lib, create new soft link
        check_and_remove_file $SYS_INSTALL_DIR/lib/libmvnc.so
        check_and_remove_file $SYS_INSTALL_DIR/lib/libmvnc.so.0
        [ -f ${SYS_INSTALL_DIR}/lib/mvnc/libmvnc.so.0 ] && $SUDO_PREFIX mv ${SYS_INSTALL_DIR}/lib/mvnc/libmvnc.so.0 ${NCSDK1_ARCHIVE_DIR}/lib
        check_and_remove_file ${NCSDK1_ARCHIVE_DIR}/lib/libmvnc.so
        [ -f ${NCSDK1_ARCHIVE_DIR}/lib/libmvnc.so.0 ] && $SUDO_PREFIX ln -s ${NCSDK1_ARCHIVE_DIR}/lib/libmvnc.so.0 ${NCSDK1_ARCHIVE_DIR}/lib/libmvnc.so

        ## Move NCSDK 1.x firmware
        [ -f ${SYS_INSTALL_DIR}/lib/mvnc/MvNCAPI.mvcmd ] && $SUDO_PREFIX mv ${SYS_INSTALL_DIR}/lib/mvnc/MvNCAPI.mvcmd ${NCSDK1_ARCHIVE_DIR}/lib/mvnc

        ## Move NCSDK 1.x Python API
        NCSDK_DIR=${INSTALL_DIR}/NCSDK
        SDK_DIR=${NCSDK_DIR}/ncsdk-$(eval uname -m)
        [ -d ${SDK_DIR}/api ] && $SUDO_PREFIX mv ${SDK_DIR}/api ${NCSDK1_ARCHIVE_DIR}/

        MOVED_NCAPI1="yes"
        echo -e "${YELLOW}Moved existing NCSDK 1.x files to ${NCSDK1_ARCHIVE_DIR} ${NC}"
        echo ""
    fi
}


# remove_previous_install - remove previous installed files if they exist
function remove_previous_install()
{
    echo "Searching and removing previous NCSDK installation from the system"
    if [ "${PREV_NCS_BIN_PATH}" != "unknown" ] ; then
        # If installed, remove toolkit binaries
        check_and_remove_tk_file mvNCCheck
        check_and_remove_tk_file mvNCProfile
        check_and_remove_tk_file mvNCCompile
        check_and_remove_tk_file ncsdk
    fi
    
    # Remove libraries
    if [ "${PREV_NCS_LIB_PATH}" != "unknown" ] ; then
        [ "${VERBOSE}" = "yes" ] && printf "Searching and removing NCSDK libraries..."
        check_and_remove_file  $PREV_NCS_LIB_PATH/libmvnc.so.0
        check_and_remove_file  $PREV_NCS_LIB_PATH/libmvnc.so
        check_and_remove_file  $PREV_NCS_LIB_PATH/libmvnc_highclass.so
        check_and_remove_files $PREV_NCS_LIB_PATH/mvnc
        [ "${VERBOSE}" = "yes" ] && printf "done\n"
    fi
    
    # Remove include files
    if [ "${PREV_NCS_INC_PATH}" != "unknown" ] ; then
        [ "${VERBOSE}" = "yes" ] && printf "Searching and removing NCSDK include files..."
        check_and_remove_file $PREV_NCS_INC_PATH/mvnc.h
        check_and_remove_file $PREV_NCS_INC_PATH/mvnc_deprecated.h
        check_and_remove_files $PREV_NCS_INC_PATH/mvnc2
        [ "${VERBOSE}" = "yes" ] && printf "done\n"
    fi
    
    # Remove API, don't prompt user for a response
    [ "${VERBOSE}" = "yes" ] && printf "Searching and removing NCS python API..."
    # if pip2/3 are installed, attempt to remove mvnc.  If pip not installed then mvnc couldn't have been installed.
    RC_PIP2=0
    RC_PIP3=0
    command -v pip2 > /dev/null || RC_PIP2=$?
    command -v pip3 > /dev/null || RC_PIP3=$?
    [ ${RC_PIP2} -eq 0 ] && [ ${RC_PIP3} -eq 0 ] && check_and_remove_pip_pkg mvnc
    [ "${VERBOSE}" = "yes" ] && printf "done\n"
    
    # Remove udev rules files
    [ "${VERBOSE}" = "yes" ] && printf "Searching and removing udev rules..."
    check_and_remove_file /etc/udev/rules.d/97-usbboot.rules
    [ "${VERBOSE}" = "yes" ] && printf "done\n"

    # Remove NCSDK files
    if [ "${PREV_NCSDK_PATH}" != "unknown" ] ; then
        [ "${VERBOSE}" = "yes" ] && printf "Searching and removing NCSDK files..."
        check_and_remove_files ${PREV_NCSDK_PATH}/NCSDK
        [ "${VERBOSE}" = "yes" ] && printf "done\n"
    fi

    # Remove script files
    if [ "${PREV_NCSDK_PATH}" != "unknown" ] ; then
        [ "${VERBOSE}" = "yes" ] && printf "Searching and removing ncsdk.conf & uninstall script files..."
        check_and_remove_file ${PREV_NCSDK_PATH}/ncsdk.conf
        check_and_remove_file ${PREV_NCSDK_PATH}/install-utilities.sh
        check_and_remove_file ${PREV_NCSDK_PATH}/uninstall.sh
        check_and_remove_file ${PREV_NCSDK_PATH}/uninstall-ncsdk.sh
        [ "${VERBOSE}" = "yes" ] && printf "done\n"
    fi

    if [ "${USE_VIRTUALENV}" == 'yes' ]; then
        check_and_remove_files ${INSTALL_DIR}/virtualenv-python
    fi
    
    [ "${VERBOSE}" = "yes" ] && printf "Running ldconfig..."
    $SUDO_PREFIX ldconfig
    [ "${VERBOSE}" = "yes" ] && printf "done\n"

    [ "${VERBOSE}" = "yes" ] && printf "Updating udev rules..."
    RC=0
    $SUDO_PREFIX udevadm control --reload-rules || RC=$?
    if [ $RC -ne 0 ] ; then
        echo "Warning udevadm control --reload-rules return code = ${RC}"
    fi
    RC=0 
    $SUDO_PREFIX udevadm trigger || RC=$?
    if [ $RC -ne 0 ] ; then
        echo "Warning udevadm trigger reported an error return code = ${RC}"
    fi
    [ "${VERBOSE}" = "yes" ] && printf "done\n"

    # remove NCSDK dir
    check_and_remove_files ${INSTALL_DIR}/NCSDK

    # remove install info file
    check_and_remove_files ${PREV_INSTALL_INFO}
    
    ## remove version.txt and LICENSE
    check_and_remove_files ${INSTALL_DIR}/version.txt
    check_and_remove_files ${INSTALL_DIR}/LICENSE
    
    echo "Successfully uninstalled NCSDK from the system"
}

#!/bin/bash
#
# Movidius Neural Compute Toolkit uninstall script.
#
# Function main at the bottom calls all of the other functions required to uninstall.
# 
# Please provide feedback in our support forum if you encountered difficulties.
################################################################################
# read in functions shared by installer and uninstaller
source $(dirname "$0")/install-utilities.sh

# remove_NCSDK_from_PYTHONPATH - removes PYTHONPATH from $HOME/.bashrc
function remove_NCSDK_from_PYTHONPATH()
{
    [ "${VERBOSE}" = "yes" ] && printf "Remove NCSDK references from PYTHONPATH..."
    #sed -i "\#export PYTHONPATH=\"\${PYTHONPATH}:"${PREV_NCSDK_PATH}"/mvnc/python\"#d" $HOME/.bashrc
    sed -i "\#export PYTHONPATH=\"\${PYTHONPATH}:"${PREV_NCSDK_PATH}"/caffe/python\"#d" $HOME/.bashrc
    # Remove some of the older versions of this as well
    sed -i "\#export PYTHONPATH=\$env:\""${PREV_NCSDK_PATH}"/mvnc/python\":\$PYTHONPATH#d" $HOME/.bashrc
    sed -i "\#export PYTHONPATH=\$env:\""${PREV_NCSDK_PATH}"/caffe/python\":\$PYTHONPATH#d" $HOME/.bashrc
    [ "${VERBOSE}" = "yes" ] && printf "done\n"    
}


# remove_caffe - remove version of caffe built during the install
function remove_caffe()
{
    [ "${VERBOSE}" = "yes" ] && printf "Removing ${INSTALL_DIR}/${CAFFE_FLAVOR}-caffe..."
    # remove caffe directory
    check_and_remove_files ${INSTALL_DIR}/${CAFFE_FLAVOR}-caffe
    # remove caffe soft link
    check_and_remove_file ${INSTALL_DIR}/caffe
    [ "${VERBOSE}" = "yes" ] && printf "done\n"         
}


# remove_install_dir - remove ${INSTALL_DIR} if ${NCSDK1_ARCHIVE_DIR} doesn't exist, the dir NCSDK 1.x files were moved to
function remove_install_dir()
{
    if [ -d ${NCSDK1_ARCHIVE_DIR} ] ; then
        echo ""
        echo -e "${YELLOW}Warning: Detected an NCSDK 1.x backup directory at ${NCSDK1_ARCHIVE_DIR}"        
        read -p "Delete this directory (y/n) ? " CONTINUE
        echo -e "${NC}"        
        if [[ "$CONTINUE" = "y" || "$CONTINUE" = "Y" ]]; then
            echo "Based on user input, will remove ${NCSDK1_ARCHIVE_DIR}"
            ${SUDO_PREFIX} rm -rf ${NCSDK1_ARCHIVE_DIR}
        else
            # if user wants to keep ${NCSDK1_ARCHIVE_DIR}, don't rm ${INSTALL_DIR}
            echo "Not removing ${INSTALL_DIR} because ${NCSDK1_ARCHIVE_DIR} exists, the directory where NCSDK 1.x files were moved to"
            return
        fi
    fi
    [ "${VERBOSE}" = "yes" ] && printf "Removing ${INSTALL_DIR}/..."
    check_and_remove_files ${INSTALL_DIR}
    [ "${VERBOSE}" = "yes" ] && printf "done\n"
}


function initialize_virtualenv()
{
	# if virtualenv dir exists, try to activate it
    if [ -d ${VIRTUALENV_DIR} ] ; then
        RC=0
        # disable trapping for unset variables due to activate script 
        set +u
        source ${VIRTUALENV_DIR}/bin/activate || RC=$?
        set -u
        if [ ${RC} -ne 0 ] ; then
            echo "source ${VIRTUALENV_DIR}/bin/activate gave an error=${RC}"
            echo "You need to investigate: 1) Either figure out why virtualenv is not activating correctly or"
            echo "                         2) edit ncsdk.conf to set USE_VIRTUALENV=no to disable virtualenv.  Will exit"
            exit 1
            echo ""
        else
            echo "virtualenv ${VIRTUALENV_DIR} exists, and successfully activated it"
        fi
	else
		echo "Supplied virtualenv dir does not exist."
	fi
}


# main - this is the main function that runs the uninstall
function main()
{
    echo "Movidius Neural Compute Toolkit uninstall"

    ### get constants, function is in install-utilities.sh
    initialize_constants

    ### ask for sudo priviledges, function is in install-utilities.sh
    ask_sudo_permissions

    ### function is in install-utilities.sh
    read_ncsdk_config

    # override value in ncsdk.config for uninstall
    VERBOSE=yes
    
    ### function is in install-utilities.sh
    find_previous_install

    # Optionally use python virtualenv, USE_VIRTUALENV set in ncsdk.conf
    [ "${USE_VIRTUALENV}" == 'yes' ] && initialize_virtualenv
    
    ### remove prev installation. Function is in install-utilities.sh
    remove_previous_install

    ### edit $HOME/.bashrc
    remove_NCSDK_from_PYTHONPATH

    ### remove version of caffe that installer installed
    remove_caffe

    remove_install_dir
    echo "Movidius Neural Compute Toolkit uninstall finished"
}
main

#!/bin/bash
#
# Movidius Neural Compute Toolkit uninstall-opencv.sh script.
#
# Function main at the bottom calls all of the other functions required to uninstall.
# 
# Please provide feedback in our support forum if you encountered difficulties.
################################################################################
# read in functions shared by installer and uninstaller
source $(dirname "$0")/install-utilities.sh


# initialization() - sets up variables, error handling, reads config file and asks for sudo if needed
function initialization()
{
    # set OpenCV version
    OPENCV_VER="3.3.0"
    # enable trapping for error (function is in install-utilities.sh)
    set_error_handling
    ### get constants (function is in install-utilities.sh)
    initialize_constants
    ### read config file (function is in install-utilities.sh) 
    read_ncsdk_config
    # Ask for sudo priviledges (function is in install-utilities.sh)
    ask_sudo_permissions
}


# check_opencv_python - check if OpenCV python packages installed via pip
function check_opencv_python()
{
    ## uninstall pip OpenCV packages if installed
    # if pip3 is installed, attempt to remove OpenCV python packages.  If pip3 not installed then OpenCV packages couldn't have been installed.
    RC=0
    command -v pip3 > /dev/null || RC=$?

    if [ ${RC} -eq 0 ] ; then
        # Is pip pkg opencv-python installed?  
        RC=0
        $PIP_PREFIX pip3 show opencv-python 1> /dev/null || RC=$?
        if [ $RC -eq 0 ] ; then
            PROMPT_UNINSTALL_OPENCV="Yes"
        fi
    fi
    return 0
}


# check_opencv_from_sources - check if OpenCV was installed from sources
function check_opencv_from_sources()
{
    # check if OpenCV Makefile exists
    [ -e "$HOME/opencv-${OPENCV_VER}/build/Makefile" ]  && PROMPT_UNINSTALL_OPENCV="Yes"
    return 0
}


# uninstall_opencv - uninstalls OpenCV python packages via pip and if installed from sources
function uninstall_opencv()
{
    if [ "${PROMPT_UNINSTALL_OPENCV}" = "Yes" ] ; then
        echo ""
        echo "************************ Please confirm *******************************"
        echo " As of NCSDK 1.11, installation requires that previous installations of"
        echo " openCV be uninstalled before proceeding with NCSDK installation."
        echo " Note that if you installed opencv via pip3 or from source into the"
        echo " home directory, it will be uninstalled."
        read -p " Continue uninstalling OpenCV (y/n) ? " CONTINUE
        if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then

            # remove OpenCV for python
            check_and_remove_pip_pkg opencv-contrib-python
            check_and_remove_pip_pkg opencv-python

            # remove OpenCV if installed from source in $HOME/opencv-${OPENCV_VER}
            echo "Looking for opencv source installation";
            if [ -d "$HOME/opencv-${OPENCV_VER}" ]; then
                echo "opencv-${OPENCV_VER} directory exists"
                if [ -e "$HOME/opencv-${OPENCV_VER}/build/Makefile" ]; then
                    echo "opencv-${OPENCV_VER} Makefile exists, attempting to uninstall opencv-${OPENCV_VER}"
                    cd "$HOME/opencv-${OPENCV_VER}/build"
                    $SUDO_PREFIX  make uninstall &> /dev/null
                    echo "done."
                fi
            fi
        else
            echo "Based on user input, not removing OpenCV"
            exit 1
        fi
    fi    
}


# main - this is the main function that runs the uninstall
function main()
{
    echo "OpenCV uninstall starting"
    initialization

    # default to OpenCV not installed
    PROMPT_UNINSTALL_OPENCV="No"

    # check if OpenCV installed via pip or from sources
    check_opencv_python
    check_opencv_from_sources

    # uninstall OpenCV
    uninstall_opencv
    
    echo "OpenCV uninstall finished"
}
main

#! /bin/bash


# test if OpenCV already installed for python
function test_opencv_installed()
{
    RC=0
    python3 -c "import cv2" > /dev/null 2>&1 || RC=$?
    if [ $RC -eq 0 ] ;
    then
        echo "";
        echo "OpenCV already setup for python3";
        echo "";
        exit 0
    fi;
}


# install_opencv - installs OpenCV
function install_opencv()
{
    # install package lsb-release if application lsb_release isn't installed
    APT_QUIET="-qq"
    [ "${VERBOSE}" = "yes" ] && APT_QUIET=""
    RC=0
    command -v lsb_release > /dev/null || RC=$?
    if [ $RC -ne 0 ] ; then
        exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET update -y"
        exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y lsb-release"
    fi
    
    if [[ `lsb_release -d` =~ .*Raspbian.* ]] 
    then 
        echo ""
        echo "************************ Please confirm *******************************"
        echo " Installing OpenCV on Raspberry Pi may take a long time."
        echo " You may skip this part of the installation in which case some examples "
        echo " may not work without modifications but the rest of the SDK will still "
        echo " be functional. Select n to skip OpenCV installation or y to install it." 
        read -p " Continue installing OpenCV (y/n) ? " CONTINUE
        if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then
            echo ""; 
            echo "Installing OpenCV"; 
            echo "";
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET update -y"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y build-essential cmake pkg-config"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y libavcodec-dev libavformat-dev libswscale-dev libv4l-dev"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y libxvidcore-dev libx264-dev"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y libgtk2.0-dev libgtk-3-dev"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y libatlas-base-dev gfortran"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y python2.7-dev python3-dev wget python3-pip"

            cd $HOME
            VERSION="3.3.0"
            exec_and_search_errors "wget -O opencv.zip https://github.com/Itseez/opencv/archive/${VERSION}.zip"
            ZIP_QUIET="-q"
            [ "${VERBOSE}" = "yes" ] && ZIP_QUIET=""
            unzip ${ZIP_QUIET} opencv.zip
            exec_and_search_errors "wget -O opencv_contrib.zip https://github.com/Itseez/opencv_contrib/archive/${VERSION}.zip"
            unzip ${ZIP_QUIET} opencv_contrib.zip
            cd ${HOME}/opencv-${VERSION}/
            mkdir -p build
            cd build
            cmake -D CMAKE_BUILD_TYPE=RELEASE \
                  -D CMAKE_INSTALL_PREFIX=/usr/local \
                  -D INSTALL_PYTHON_EXAMPLES=OFF \
                  -D OPENCV_EXTRA_MODULES_PATH=${HOME}/opencv_contrib-${VERSION}/modules \
                  -D BUILD_DOCS=OFF \
                  -D BUILD_PERF_TESTS=OFF \
                  -D BUILD_TESTS=OFF \
                  -D BUILD_EXAMPLES=OFF ..

            # build and trap for errors in case we ran out of memory running make -j ${MAKE_NJOBS}
            RC=0
            make -j ${MAKE_NJOBS} || RC=$?
            if [ $RC -ne 0 ] ; then
                echo -e "${RED}  Running make -j ${MAKE_NJOBS} failed."
                if [ ${MAKE_NJOBS} -gt 2 ] ; then
                    echo "MAKE_NJOBS=${MAKE_NJOBS}, suggestion is to increase swap space and edit ncsdk.conf to uncomment #MAKE_NJOBS=1 and change to MAKE_NJOBS=2 or MAKE_NJOBS=1 and try again"
                else
                    if [ ${MAKE_NJOBS} -gt 1 ] ; then
                        echo "MAKE_NJOBS=${MAKE_NJOBS}, suggestion is increase swap space and edit ncsdk.conf to uncomment #MAKE_NJOBS=1 and try again"
                    else
                        echo "MAKE_NJOBS=${MAKE_NJOBS}, suggestion is to increase swap space and try again"  
                    fi
                fi
                echo -e "Error on line $LINENO.  Will exit${NC}"
                exit 1
            fi            

            $SUDO_PREFIX make install
            $SUDO_PREFIX ldconfig
        else
            echo "";
            echo "Skipping OpenCV installation based on user input";
            echo "";
        fi
    else  
        echo "Installing opencv python for non-Raspbian";
        # check if pip2 & pip3 are available on the system via 'command'
        RC=0
        command -v pip3 > /dev/null || RC=$?
        if [ $RC -ne 0 ] ; then
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET update -y"
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y python3-pip"
        fi
        command -v pip2 > /dev/null || RC=$?
        if [ $RC -ne 0 ] ; then
            exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y python-pip"
        fi

        PIP_QUIET=--quiet
        [ "${VERBOSE}" = "yes" ] && PIP_QUIET=""
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET opencv-python>=3.4.0.12"
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET opencv-contrib-python>=3.4.0.12"
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET opencv-python>=3.4.0.12"
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET opencv-contrib-python>=3.4.0.12"
    fi
}



# main - this is the main function that runs the OpenCV install
function main()
{
    echo "OpenCV Installation Starting"
    
    # Test if OpenCV is installed.  If OpenCV is already installed for python, script will exit 
    test_opencv_installed

    
    ### initialization 
    # read in functions shared by installer and uninstaller
    source $(dirname "$0")/install-utilities.sh
    # enable trapping for error (function is in install-utilities.sh)
    set_error_handling
    ### get constants (function is in install-utilities.sh)
    initialize_constants
    ### read config file (function is in install-utilities.sh) 
    read_ncsdk_config
    # (function is in install-utilities.sh)    
    ask_sudo_permissions

    
    ### install opencv
    install_opencv

    echo "OpenCV Installation Finished"    
}
main

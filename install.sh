#!/bin/bash
#
# Movidius Neural Compute Toolkit installation script.
# 
# The file ncsdk.conf contains the user configuration options and these
# override the defaults set in this script.  The function print_ncsdk_config()
# is called if ${VERBOSE} = "yes" to print user config variables and documents what they do.
#
# Function main at the bottom calls all of the other functions required to install.
#
# Function check_prerequisites lists the prerequisites required to install
#  
# Please provide feedback in our support forum if you encountered difficulties.
################################################################################
# read in functions shared by installer and uninstaller
source $(dirname "$0")/install-utilities.sh


# check_supported_os - require install to be running on a supported OS
function check_supported_os()
{
    ### Checking OS and version...
    # install package lsb-release if application lsb_release isn't installed 
    RC=0
    command -v lsb_release > /dev/null || RC=$?
    [ $RC -ne 0 ] && exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y lsb-release"
    
    DISTRO="$(lsb_release -i 2>/dev/null | cut -f 2)"
    VERSION="$(lsb_release -r 2>/dev/null | awk '{ print $2 }' | sed 's/[.]//')"
    OS_DISTRO="${DISTRO:-INVALID}"
    OS_VERSION="${VERSION:-255}"
    if [ "${OS_DISTRO,,}" = "ubuntu" ] && [ ${OS_VERSION} = 1604 ]; then
        [ "${VERBOSE}" = "yes" ] && echo "Installing on Ubuntu 16.04"
    elif [ "${OS_DISTRO,,}" = "raspbian" ] && [ ${OS_VERSION} -ge 91 ]; then
        [ "${VERBOSE}" = "yes" ] && echo "Installing on Raspbian Stretch"
    elif [ "${OS_DISTRO,,}" = "raspbian" ] && [ ${OS_VERSION} -ge 80 ] && [ ${OS_VERSION} -lt 90 ]; then
        echo -e "${RED} You are running Raspbian Jessie, which is not supported by NCSDK."
        echo -e "Please upgrade to Raspbian Stretch and then install NCSDK."
        echo -e "Error on line $LINENO${NC}"
        exit 1
    else
        echo "Your current combination of Linux distribution and distribution version is not officially supported! Error on line $LINENO.  Will exit"
        exit 1
    fi
    return 0
}


# check_prerequisites - tests the prerequisites required to install
function check_prerequisites()
{
    prerequisites="sudo sed udevadm tar"
    for i in $prerequisites; do
        RC=0
        # command is a bash builtin to run a command ($i in this case)
        command -v $i > /dev/null || RC=$?
        if [ $RC -ne 0 ] ; then
            echo -e "${RED}Installation prerequisite $i not available on this system.  Error on line $LINENO.  Will exit${NC}"
            exit 1
        fi
    done
    
    check_supported_os
}


# create_install_logfile - Save a log file of the install in $DIR/setup-logs
function create_install_logfile()
{
    # Print stdout and stderr to screen and in logfile
    d=$(date +%y-%m-%d-%H-%M)
    LOGFILE="setup_"$d".log"
    ${SUDO_PREFIX} mkdir -p $DIR/setup-logs
    ${SUDO_PREFIX} chown $(id -un):$(id -gn) $DIR/setup-logs
    echo "Saving installation log file in $DIR/setup-logs/$LOGFILE"
    exec &> >(tee -a "$DIR/setup-logs/$LOGFILE")    
}


# print_ncsdk_config - prints out the definition and values of user configurable options
function print_ncsdk_config()
{
    echo ""
    echo "print_ncsdk_config()"
    echo "INSTALL_DIR - Main installation directory"
    echo "INSTALL_CAFFE - Flag to install TensorFlow"
    echo "CAFFE_FLAVOR - Specific 'flavor' of caffe to install"
    echo "CAFFE_USE_CUDA - Use CUDA enabled version of caffe"
    echo "INSTALL_TENSORFLOW - Flag to install TensorFlow"
    echo "INSTALL_TOOLKIT - Flag to install Neural Compute Toolkit"
    echo "PIP_SYSTEM_INSTALL - Globally install pip packages via sudo -H"
    echo "VERBOSE - Flag to enable more verbose installation"
    echo "USE_VIRTUALENV - Flag to enable python virtualenv"
    echo "MAKE_NJOBS - Number of processes to use for parallel build (i.e. make -j MAKE_NJOBS)"
    echo ""
    echo "INSTALL_DIR=${INSTALL_DIR}"
    echo "INSTALL_CAFFE=${INSTALL_CAFFE}"
    echo "CAFFE_FLAVOR=${CAFFE_FLAVOR}"
    echo "CAFFE_USE_CUDA=${CAFFE_USE_CUDA}"
    echo "INSTALL_TENSORFLOW=${INSTALL_TENSORFLOW}"
    echo "INSTALL_TOOLKIT=${INSTALL_TOOLKIT}"    
    echo "PIP_SYSTEM_INSTALL=${PIP_SYSTEM_INSTALL}"
    echo "VERBOSE=${VERBOSE}"
    echo "USE_VIRTUALENV=${USE_VIRTUALENV}"
    echo "MAKE_NJOBS=${MAKE_NJOBS}"
    echo ""
}


# init_installer - 
#   sets up installer including error handling, verbosity level, sudo privileges,
#   reads ncsdk.conf via read_ncsdk_config function, creates dirs for installation,
#   sets global variables CONF_FILE, DIR and NCSDK_VERSION.
function init_installer()
{
    # trap errors (function is in install-utilities.sh) 
    set_error_handling

    ### get constants (function is in install-utilities.sh) 
    initialize_constants
    
    ### check if file exist 
    VERSION_FILE=version.txt
    if [ ! -f ${VERSION_FILE} ] ; then
        echo -e "${RED}Couldn't find file ${VERSION_FILE}. Error on line $LINENO  Will exit${NC}"
        exit 1
    fi
    NCSDK_VERSION=`cat ${VERSION_FILE}`
    echo "Installer NCSDK version: $NCSDK_VERSION"
    echo ""
    
    ### read config file (function is in install-utilities.sh) 
    read_ncsdk_config

    ### Ask for sudo priviledges (function is in install-utilities.sh)
    ask_sudo_permissions

    ### Set installer verbosity level
    APT_QUIET=-qq
    PIP_QUIET=--quiet
    GIT_QUIET=-q
    STDOUT_QUIET='>/dev/null'
    if [ "${VERBOSE}" = "yes" ]; then
        APT_QUIET=
        PIP_QUIET=
        GIT_QUIET=
        STDOUT_QUIET=
    fi

    # Install location for sdk and API 
    DIR=${INSTALL_DIR}/NCSDK
    SDK_DIR=$DIR/ncsdk-$(eval uname -m)
    if [ -d /usr/local/lib ]; then
        SYS_INSTALL_DIR=/usr/local
    else
        SYS_INSTALL_DIR=/usr
    fi

    ### make sure system has required prerequisites
    check_prerequisites

    if [ "${VERBOSE}" = "yes" ] ; then
        print_ncsdk_config
    fi
}


# make_installer_dirs - creates directories that the install uses
function make_installer_dirs()
{
    ### Create Required installation dirs.  
    ${SUDO_PREFIX} mkdir -p ${INSTALL_DIR}
    ${SUDO_PREFIX} chown $(id -un):$(id -gn) "$INSTALL_DIR"
    # Get absolute dir
    INSTALL_DIR="$( cd ${INSTALL_DIR} && pwd )"

    
    # Create directories if needed
    $SUDO_PREFIX mkdir -p $SYS_INSTALL_DIR/include/mvnc2
    $SUDO_PREFIX mkdir -p $SYS_INSTALL_DIR/lib/mvnc
}


# download_and_copy_files - download tarball and copy to install dir
function download_and_copy_files()
{
    download_filename="NCSDK-${NCSDK_VERSION}.tar.gz"
    if [ ! -f ${download_filename} ] ; then
        echo "Downloading ${download_filename}"
        ncsdk_link="https://downloadmirror.intel.com/27839/eng/NCSDK-2.05.00.02.tar.gz"
        exec_and_search_errors "wget --no-cache -O ${download_filename} $ncsdk_link"
    fi
    # ncsdk_pkg is the filename without the .tar.gz extension
    ncsdk_pkg=${download_filename%%.tar.gz}

    # copy files
    ${SUDO_PREFIX} cp ./$download_filename ${INSTALL_DIR}/ncsdk.tar.gz
    ${SUDO_PREFIX} cp ./uninstall.sh ${INSTALL_DIR}/
    ${SUDO_PREFIX} cp ./install-utilities.sh ${INSTALL_DIR}/
    ${SUDO_PREFIX} cp ./ncsdk.conf ${INSTALL_DIR}/

    # save current dir
    FROM_DIR=$PWD

    # untar in INSTALL_DIR 
    cd ${INSTALL_DIR}
    if [ "${VERBOSE}" = "yes" ] ; then  
        TAR_OPTIONS="--no-same-owner -vzxf"
    else
        TAR_OPTIONS="--no-same-owner -zxf"
    fi
    ${SUDO_PREFIX} tar ${TAR_OPTIONS} ./ncsdk.tar.gz
    ${SUDO_PREFIX} rm -rf NCSDK
    ${SUDO_PREFIX} mv $ncsdk_pkg* NCSDK
    cd ${INSTALL_DIR}/NCSDK

    ${SUDO_PREFIX} cp ${FROM_DIR}/version.txt ${INSTALL_DIR}
    ${SUDO_PREFIX} cp ${FROM_DIR}/ncsdk.conf ${INSTALL_DIR}/NCSDK 
    ${SUDO_PREFIX} cp ${FROM_DIR}/uninstall.sh ${INSTALL_DIR}/NCSDK 
    ${SUDO_PREFIX} cp ${FROM_DIR}/requirements.txt ${INSTALL_DIR}/NCSDK 
    ${SUDO_PREFIX} cp ${FROM_DIR}/requirements_apt.txt ${INSTALL_DIR}/NCSDK 
    ${SUDO_PREFIX} cp ${FROM_DIR}/requirements_apt_raspbian.txt ${INSTALL_DIR}/NCSDK 
}


# compare_versions - sets global VERCOMP_RETVAL 
function compare_versions()
{
    VERCOMP_RETVAL=-1
    if [ $1 = $2 ]; then
        VERCOMP_RETVAL=0
    else
        if [[ $1 = `echo -e "$1\n$2" | sort -V | head -n1` ]]; then
            VERCOMP_RETVAL=1
        else
            VERCOMP_RETVAL=2
        fi
    fi
}


# print_previous_ncsdk_install_info - if previous valid install found, print version number
function print_previous_ncsdk_install_info()
{
    if [ "${PREV_INSTALL_INFO}" != "unknown" ]; then

        # compare_versions sets VERCOMP_RETVAL to 0, 1 or 2
        compare_versions ${PREV_NCSDK_VER} ${NCSDK_VERSION}
        
        if [ ${VERCOMP_RETVAL} -eq 0 ]; then
            echo "Previously installed version is the same as installer version, overwriting..."
        elif [ ${VERCOMP_RETVAL} -eq 1 ]; then
            echo "Previously installed version is older than installer version, upgrading..."
        else
            echo "Previously installed version is more recent than installer version, downgrading..."
        fi
    fi
}


# install_apt_dependencies - installs dependencies using apt 
function install_apt_dependencies()
{
    exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET update"
    exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y $(cat "$DIR/requirements_apt.txt")"
    exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y --no-install-recommends libboost-all-dev"

    # Make sure pip2 and pip3 installed correctly
    RC=0
    command -v pip2 > /dev/null || RC=$?
    if [ $RC -ne 0 ] ; then
        echo -e "${RED} pip2 command not found.  Will exit${NC}"
        exit 1
    fi    
    RC=0
    command -v pip3 > /dev/null || RC=$?
    if [ $RC -ne 0 ] ; then
        echo -e "${RED} pip3 command not found.  Will exit${NC}"
        exit 1
    fi

    $SUDO_PREFIX ldconfig
}


# setup_virtualenv - Use python virtualenv 
function setup_virtualenv()
{
    echo ""
    echo "USE_VIRTUALENV=${USE_VIRTUALENV} - will use python virtualenv"
    [ "${VERBOSE}" = "yes" ] && echo "Installing prerequisites for virtualenv"
    ${SUDO_PREFIX} apt-get $APT_QUIET install python-virtualenv -y
    echo ""
    
    VIRTUALENV_DIR=${INSTALL_DIR}/virtualenv-python
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
            echo "virtualenv ${INSTALL_DIR}/virtualenv-python exists, and successfully activated it"
        fi
    else
        # Create new virtualenv and activate it
        echo "Creating new virtualenv in ${VIRTUALENV_DIR}"
        ${SUDO_PREFIX} mkdir -p ${VIRTUALENV_DIR}
        ${SUDO_PREFIX} virtualenv --system-site-packages -p python3 ${VIRTUALENV_DIR}
        # disable trapping for unset variables due to activate script
        set +u
        RC=0        
        source ${VIRTUALENV_DIR}/bin/activate || RC=$?
        set -u        
        if [ ${RC} -ne 0 ] ; then
            echo "source ${VIRTUALENV_DIR}/bin/activate gave an error=${RC}"
            echo "This is unexpected and needs to be investigated."
            echo "virtualenv can be disabled via edit ncsdk.conf to set USE_VIRTUALENV=no.  Will exit"
            exit 1
        else
            echo "Created virtualenv in ${VIRTUALENV_DIR}, and successfully activated it"
        fi
    fi
    echo ""
    echo "   To activate this virtualenv in the future enter"
    echo "   source ${VIRTUALENV_DIR}/bin/activate "
    echo ""


}


# install_python_dependencies - install dependencies using pip2/pip3 
function install_python_dependencies()
{
    # Note: If sudo is used and PIP_SYSTEM_INSTALL=yes (set in ncsdk.conf), pip packages
    #       will be installed in the systems directory, otherwise installed per user
    echo "Installing python dependencies"

    if [ "${OS_DISTRO,,}" = "ubuntu" ] ; then
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --trusted-host files.pythonhosted.org -r $DIR/requirements.txt"
        # Install packages for python 2.x, required for NCSDK python API
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET --trusted-host files.pythonhosted.org Enum34>=1.1.6"
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET --trusted-host files.pythonhosted.org --upgrade numpy>=1.13.0,<=1.13.3"

        # verify python3 import scipy._lib.decorator working, a potential problem on Ubuntu only.  First check python3 import scipy.  
        RC=0
        python3 -c "import scipy" >& /dev/null || RC=$?
        if [ ${RC} -ne 0 ] ; then
            echo -e "${RED}Error, cannot import scipy into python3.  Error on line $LINENO.  Will exit${NC}"
            exit 1
        fi
        RC=0
        python3 -c "import scipy._lib.decorator" >& /dev/null || RC=$?
        if [ ${RC} -ne 0 ] ; then
            echo -e "${YELLOW}Problem importing scipy._lib.decorator into python3.  Attempting to fix${NC}"
            # Get the location of scipy to get the location of decorator.py 
            RC=0
            SCIPY_FILE=$(python3 -c "import scipy; print(scipy.__file__)") || RC=$?
            if [ ${RC} -eq 0 ] ; then
                # Get directory decorator.py is in from SCIPY_FILE. If decorator.py isn't a readable file, i.e. from a broken softlink, reinstall via apt
                [ ! -f $(dirname $SCIPY_FILE)/decorator.py ] && $SUDO_PREFIX apt install --reinstall python*-decorator
                RC=0
                python3 -c "import scipy._lib.decorator" >& /dev/null || RC=$?
                if [ ${RC} -ne 0 ] ; then
                    echo -e "${RED}Error, cannot import scipy._lib.decorator even after trying to fix this problem.  Error on line $LINENO.  Will exit${NC}"
                    exit 1
                else
                    echo "Resolved problem importing scipy._lib.decorator into python3."
                    echo ""
                fi
            else
                echo -e "${RED}Error in python3 importing scipy / printing scipy.__file__.  Error on line $LINENO.  Will exit${NC}"
                exit 1
            fi  
        fi
        
    elif [ "${OS_DISTRO,,}" = "raspbian" ] ; then
        # for Raspian, use apt with python3-* if available
        exec_and_search_errors "$SUDO_PREFIX apt-get $APT_QUIET install -y $(cat "$DIR/requirements_apt_raspbian.txt")"
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --trusted-host files.pythonhosted.org Cython graphviz scikit-image"
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --trusted-host files.pythonhosted.org --upgrade numpy==1.13.1"
        exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --trusted-host files.pythonhosted.org pygraphviz Enum34>=1.1.6 networkx>=2.1,<=2.1"
        # Install packages for python 2.x, required for NCSDK python API
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET --trusted-host files.pythonhosted.org Enum34>=1.1.6"
        exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET --trusted-host files.pythonhosted.org --upgrade numpy==1.13.1"
    fi
}


# find_tensorflow - Test if tensorflow is installed and if it's the supported version
#                   function arg1 - package name - tensorflow or tensorflow-gpu
#                   sets FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=0 when supported version of TensorFlow is installed
#                   sets FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=1 when an unsupported version of TensorFlow is installed
#                   sets FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=2 when TensorFlow isn't installed
function find_tensorflow()
{
    SUPPORTED_TENSORFLOW_VERSION=1.7.0
    RC=0
    $PIP_PREFIX pip3 show $1 1> /dev/null || RC=$?
    if [ $RC -eq 0 ]; then
        tf_ver_string=$($PIP_PREFIX pip3 show $1 | grep "^Version:")
        tf_ver=${tf_ver_string##* }
        if echo "$1" | grep -q "gpu"; then
            hw="GPU"
        else
            hw="CPU"
        fi
        echo "Found tensorflow $hw version $tf_ver"
        if [ $tf_ver = "$SUPPORTED_TENSORFLOW_VERSION" ]; then
            FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=0
        else
            FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=1
        fi
    else
        FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION=2
    fi
}


# install_tensorflow - install supported version of tensorflow on system
function install_tensorflow()
{
    if [ "${OS_DISTRO,,}" = "raspbian" ] && [ "${INSTALL_TENSORFLOW}" = "yes" ]; then
        echo -e "${YELLOW}NOTE: TensorFlow is not officially supported on Raspbian Stretch."
        echo -e "      We are installing a specific nightly TensorFlow build from http://ci.tensorflow.org/view/Nightly/job/nightly-pi-python3/"
        echo -e "      for code development and testing. ${NC}"

        echo "Checking if tensorflow is installed..."
        # find_tensorflow sets FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION to 0, 1 or 2 depending if correct version installed, incorrect version installed or not installed, respectively
        find_tensorflow "tensorflow" 
        tf=${FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION}
        INSTALL_TF="no"
        [ $tf -eq 0 ] && echo "Installed TensorFlow is the correct version, not re-installing"
        if [ $tf -eq 1 ] ; then
            exec_and_search_errors "$PIP_PREFIX pip3 uninstall --quiet -y tensorflow"
            INSTALL_TF="yes"
        fi
        [ $tf -eq 2 ] && INSTALL_TF="yes"

        # install TensorFlow if needed
        if [ "${INSTALL_TF}" = "yes" ] ; then
            echo "Couldn't find a supported tensorflow version, downloading TensorFlow $SUPPORTED_TENSORFLOW_VERSION"
            # rename wheel for python 3.5
            WHEEL_DOWNLOAD=tensorflow-1.7.0-cp34-none-any.whl
            WHEEL=tensorflow-1.7.0-cp35-none-any.whl
            [ -f "${WHEEL_DOWNLOAD}" ] && sudo mv -f ${WHEEL_DOWNLOAD} ${WHEEL_DOWNLOAD}.save
            $SUDO_PREFIX wget https://storage.googleapis.com/download.tensorflow.org/deps/pi/2018_05_21/${WHEEL_DOWNLOAD}
            [ -f "${WHEEL}" ] && $SUDO_PREFIX mv -f ${WHEEL} ${WHEEL}.save
            $SUDO_PREFIX mv ${WHEEL_DOWNLOAD} ${WHEEL}
            echo "Installing TensorFlow"
            exec_and_search_errors "$PIP_PREFIX pip3 install --quiet ${WHEEL}"
            echo "Finished installing TensorFlow"
        fi              
        
    elif [ "${OS_DISTRO,,}" = "ubuntu" ] ; then
        echo "Checking whether tensorflow CPU version is installed..."
        # find_tensorflow sets FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION to 0, 1 or 2 depending if correct version installed, incorrect version installed or not installed, respectively
        find_tensorflow "tensorflow" 
        tf=${FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION}
        if [ $tf -ne 0 ]; then
            echo "Checking whether tensorflow GPU version is installed..."
            find_tensorflow tensorflow-gpu 
            tf_gpu=${FIND_TENSORFLOW__FOUND_SUPPORTED_VERSION}
        fi      
        if [[ $tf -ne 0 && $tf_gpu -ne 0 ]]; then
            echo "Couldn't find a supported tensorflow version, installing tensorflow $SUPPORTED_TENSORFLOW_VERSION"
            exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --trusted-host files.pythonhosted.org tensorflow==$SUPPORTED_TENSORFLOW_VERSION"
        else
            echo "tensorflow already at latest supported version...skipping."
        fi
    fi
}


# download_caffe - download caffe sources via git
function download_caffe()
{
    if [ -h "caffe" ]; then
        $SUDO_PREFIX rm caffe
    fi
    if [ ! -d $CAFFE_DIR ]; then
        echo "Downloading Caffe..."
        eval git clone $GIT_QUIET $CAFFE_SRC $STDOUT_QUIET $CAFFE_DIR
    fi
    ln -sf $CAFFE_DIR caffe
}


# configure_caffe_options - customize caffe options
function configure_caffe_options()
{
    # TODO: Make this configurable. Supports only python3 for now.
    sed -i 's/python_version "2"/python_version "3"/' CMakeLists.txt
    # Use/don't use CUDA
    if [ "$CAFFE_USE_CUDA" = "no" ]; then
        sed -i 's/CPU_ONLY  "Build Caffe without CUDA support" OFF/CPU_ONLY  "Build Caffe without CUDA support" ON/' CMakeLists.txt
    fi
}


# find_and_try_adjusting_caffe_symlinks - Create caffe soft link to point to specific caffe flavor that was installed
function find_and_try_adjusting_caffe_symlinks()
{
    cd ${INSTALL_DIR}
    if [[ -h "caffe" && -d "$CAFFE_DIR" ]]; then
        readlink -f caffe | grep -q "$CAFFE_DIR"
        if [ $? -eq 0 ]; then
            echo "$CAFFE_DIR present and we're currently pointing to it"
            FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND=0
        else
            if [ -d "${CAFFE_DIR}" ]; then
                echo "$CAFFE_DIR present, but we're not currently using it. Use it."
            fi
            $SUDO_PREFIX rm -f caffe
            $SUDO_PREFIX ln -sf $CAFFE_DIR caffe
            FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND=0
        fi
    else
        echo "Possibly stale caffe install, starting afresh"
        FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND=1
    fi
}

# check_opencv_caffe_conflict - Avoid known problem with OpenCV library linking with libprotobuf-lite when using ssd-caffe
function check_opencv_caffe_conflict()
{
    # Problem seen with ssd-caffe if built with OpenCV that links with GTK libraries which uses libprotobuf-lite.so.9
    # Building OpenCV from source on Ubuntu by default has this problem.  OpenCV defaults to linking with GTK libs which use libprotobuf-lite.so
    # and this causes a conflict as caffe links against libprotobuf.so, and using libprotobuf-lite.so and libprotobuf.so in the same app can cause problems.
    # Building OpenCV with QT vs. GTK avoids this problems.
    # Running make install runs ./uninstall-opencv.sh which attempts to remove OpenCV built from source for this reason.
    # Note: The OpenCV version apt-get installs in /usr/lib doesn't link against libprotobuf-lite.so, so doesn't have this problem.
    if [ "${CAFFE_FLAVOR}" = "ssd" ]; then
	LIBOPENCV_HIGHGUI=/usr/local/lib/libopencv_highgui.so.?.?.?
	RC=0
	[ -f  ${LIBOPENCV_HIGHGUI} ] && ldd ${LIBOPENCV_HIGHGUI} | grep libprotobuf-lite >& /dev/null || RC=$?
	if [ $RC -eq 0 ]; then
            echo ""
            echo ""
	    echo "**********************************************************************"
            echo "          ERROR - Compatibility Issue Detected with OpenCV"
            echo ""
            echo "The OpenCV library" ${LIBOPENCV_HIGHGUI} "links against libprotobuf-lite"
            echo "This causes an error with ssd-caffe as it links against libprotobuf (non-lite version)"
            echo "and these libraries cannot be used together and can cause a crash when deallocating memory."
            echo "Note building OpenCV with QT vs. the default GTK avoids this issue"
            echo ""
            echo "To avoid this, uninstall OpenCV libraries built from sources."
            echo "The script, uninstall-opencv.sh, attempts to do this and is called by make install"
            echo "If you have run make install or uninstall-opencv.sh and see this, try uninstalling OpenCV installed in /usr/local/ manually"
            echo "Will exit"
            exit 1
	fi
    fi
}


# install_caffe - test if caffe installed, if not download, configure and build
function install_caffe()
{
    # look for potential conflict with OpenCV library
    check_opencv_caffe_conflict

    CAFFE_SRC="https://github.com/BVLC/caffe.git"
    CAFFE_VER="1.0"
    CAFFE_DIR=bvlc-caffe
    CAFFE_BRANCH=master
    if [ "${CAFFE_FLAVOR}" = "intel" ] && [ "${OS_DISTRO,,}" = "ubuntu" ]; then
        CAFFE_SRC="https://github.com/intel/caffe.git"
        CAFFE_VER="1.0.3"
        CAFFE_DIR=intel-caffe
    elif [ "${CAFFE_FLAVOR}" = "ssd" ]; then
        CAFFE_SRC="https://github.com/weiliu89/caffe.git"
        CAFFE_VER=""
        CAFFE_DIR=ssd-caffe
        CAFFE_BRANCH=ssd
    fi

    # check if caffe is already installed
    RC=0
    python3 -c "import caffe" 2> /dev/null || RC=$?
    if [ $RC -eq 1 ]; then
        echo "Caffe not found, installing caffe..."
    else
        if [ "${CAFFE_FLAVOR}" = "intel" ] && [ "${OS_DISTRO,,}" = "ubuntu" ]; then
            # find_and_try_adjusting_caffe_symlinks sets FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND to 0 or 1
            find_and_try_adjusting_caffe_symlinks
            RC=$FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND
            # Look for intel caffe specific operation to determine the version of caffe currently running
            if [[ $RC -eq 0 && -d "caffe" ]]; then
                cd caffe
                ./build/tools/caffe time -model models/bvlc_googlenet/deploy.prototxt -engine "MKLDNN" -iterations 1 &> /dev/null
                if [ $? -eq 1 ]; then
                    echo "Intel caffe requested but not found, installing Intel caffe..."
                else
                    echo "Intel caffe already installed, skipping..."
                    return
                fi
            fi
        else
            # find_and_try_adjusting_caffe_symlinks sets FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND to 0 or 1
            find_and_try_adjusting_caffe_symlinks
            RC=$FIND_AND_TRY_ADJUSTING_CAFFE_SYMLINKS_FOUND
            if [ $RC -eq 0 ]; then
                echo "Caffe already installed, skipping..."
                return
            fi
        fi
    fi

    ### Install caffe
    cd ${INSTALL_DIR}
    if [[ -h "caffe" && -d `readlink -f caffe` ]]; then
        echo `readlink -f caffe`
        cd `readlink -f caffe`
        # grep returns 1 if no lines are matched, causing the script to
        # think that installation failed, so append "true"
        is_caffe_dir=`git remote show origin 2>&1 | grep -c -i $CAFFE_SRC` || true
        if [ "$is_caffe_dir" -eq 0 ]; then
            cd ..
            echo -e "${YELLOW}The repo $INSTALL_DIR/caffe does not match the caffe project specified"
            echo -e "in this installation. Installing caffe from $CAFFE_SRC.\n${NC}"
            download_caffe
            cd caffe
        fi

        eval git reset --hard HEAD $STDOUT_QUIET
        eval git checkout $GIT_QUIET $CAFFE_BRANCH $STDOUT_QUIET
        eval git branch -D fathom_branch &>/dev/null || true
        eval git pull $STDOUT_QUIET
    elif [ -d "caffe" ]; then
        echo -e "${YELLOW}Found an old version of caffe, removing it to install the version"
        echo -e "specified in this installer from $CAFFE_SRC.\n${NC}"
        $SUDO_PREFIX rm -r caffe
        download_caffe
    else
        download_caffe
    fi

    cd ${INSTALL_DIR}
    # disable trapping for unset variables in case PYTHONPATH not set
    set +u
    if [ -z "${PYTHONPATH}" ] ; then
        export PYTHONPATH="$INSTALL_DIR/caffe/python"
    else
        export PYTHONPATH="$INSTALL_DIR/caffe/python":$PYTHONPATH
    fi
    set -u

    # At this point, caffe *must* be a symlink
    cd `readlink -f caffe`

    if [ "$CAFFE_BRANCH" != "master" ]; then
        eval git checkout $GIT_QUIET $CAFFE_BRANCH $STDOUT_QUIET
    fi

    if [ "$CAFFE_VER" != "" ]; then
        eval git checkout $GIT_QUIET $CAFFE_VER -b fathom_branch $STDOUT_QUIET
    fi

    configure_caffe_options

    # If you get an error compiling caffe, one possible potential issue is if you
    # previously compiled an older version of opencv from sources and it is installed into /usr/local.
    # In that case the compiler will pick up the older version from /usr/local, not the version
    # this installer installed.  Please provide feedback in our support forum if you encountered difficulties. 
    echo "Compiling Caffe..."
    mkdir -p build
    cd build
    eval cmake .. $STDOUT_QUIET
    eval make -j $MAKE_NJOBS all $STDOUT_QUIET

    echo "Installing caffe..."
    eval make install $STDOUT_QUIET
    # You can use 'make runtest' to test this stage manually :)
    
    # Add PYTHONPATH if not already there
    printf "Removing previous references to previous caffe installation..."
    # Remove older references
    sed -i "\#export PYTHONPATH=\""${INSTALL_DIR}"/caffe/python\":\$PYTHONPATH#d" $HOME/.bashrc
    printf "done\n"
    echo "Adding caffe to PYTHONPATH"
    grep "^export PYTHONPATH=\"\${PYTHONPATH}:$INSTALL_DIR\/caffe\/python\"" $HOME/.bashrc || echo "export PYTHONPATH=\"\${PYTHONPATH}:$INSTALL_DIR/caffe/python\"" >> $HOME/.bashrc
}


# install_sdk - installs SDK to $SYS_INSTALL_DIR/bin
function install_sdk()
{
    # copy toolkit 
    $SUDO_PREFIX cp -r $SDK_DIR/tk $SYS_INSTALL_DIR/bin/ncsdk

    check_and_remove_file $SYS_INSTALL_DIR/bin/mvNCCompile
    check_and_remove_file $SYS_INSTALL_DIR/bin/mvNCCheck
    check_and_remove_file $SYS_INSTALL_DIR/bin/mvNCProfile
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/bin/ncsdk/mvNCCompile.py $SYS_INSTALL_DIR/bin/mvNCCompile
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/bin/ncsdk/mvNCCheck.py $SYS_INSTALL_DIR/bin/mvNCCheck
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/bin/ncsdk/mvNCProfile.py $SYS_INSTALL_DIR/bin/mvNCProfile
    echo "NCS Toolkit binaries have been installed in $SYS_INSTALL_DIR/bin"
}


# install_api - installs firmware & API 
function install_api()
{
    # Copy firmware(FW) to destination
    $SUDO_PREFIX cp $SDK_DIR/fw/MvNCAPI-*.mvcmd $SYS_INSTALL_DIR/lib/mvnc/

    # Copy C API to destination
    $SUDO_PREFIX cp $SDK_DIR/api/c/mvnc.h $SYS_INSTALL_DIR/include/mvnc2
    $SUDO_PREFIX cp $SDK_DIR/api/c/libmvnc.so.0 $SYS_INSTALL_DIR/lib/mvnc/

    if [ -f $SDK_DIR/api/c/libmvnc_highclass.so.0 ] ; then
        $SUDO_PREFIX cp $SDK_DIR/api/c/ncHighClass.h $SYS_INSTALL_DIR/include/mvnc2
        $SUDO_PREFIX cp $SDK_DIR/api/c/libmvnc_highclass.so.0 $SYS_INSTALL_DIR/lib/mvnc/
    fi
    echo "NCS Include files have been installed in $SYS_INSTALL_DIR/include"

    check_and_remove_file $SYS_INSTALL_DIR/include/mvnc.h
    check_and_remove_file $SYS_INSTALL_DIR/include/ncHighClass.h
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/include/mvnc2/mvnc.h $SYS_INSTALL_DIR/include/mvnc.h
    
    check_and_remove_file $SYS_INSTALL_DIR/lib/libmvnc.so.0
    check_and_remove_file $SYS_INSTALL_DIR/lib/libmvnc.so
    check_and_remove_file $SYS_INSTALL_DIR/lib/libmvnc_highclass.so 
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/lib/mvnc/libmvnc.so.0 $SYS_INSTALL_DIR/lib/libmvnc.so.0
    $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/lib/mvnc/libmvnc.so.0 $SYS_INSTALL_DIR/lib/libmvnc.so
    if [ -f $SYS_INSTALL_DIR/lib/mvnc/libmvnc_highclass.so.0 ] ; then
        $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/include/mvnc2/ncHighClass.h $SYS_INSTALL_DIR/include/ncHighClass.h
        $SUDO_PREFIX ln -s $SYS_INSTALL_DIR/lib/mvnc/libmvnc_highclass.so.0 $SYS_INSTALL_DIR/lib/libmvnc_highclass.so
    fi

    $SUDO_PREFIX ldconfig

    # Copy other collaterals to destination
    $SUDO_PREFIX cp -r $DIR/version.txt $INSTALL_DIR/
    $SUDO_PREFIX cp -r $SDK_DIR/LICENSE $INSTALL_DIR/

    # Install python API
    exec_and_search_errors "$PIP_PREFIX pip3 install $PIP_QUIET --upgrade --force-reinstall $SDK_DIR/api"
    exec_and_search_errors "$PIP_PREFIX pip2 install $PIP_QUIET --upgrade --force-reinstall $SDK_DIR/api"
    echo "NCS Python API has been installed in $INSTALL_DIR, and PYTHONPATH environment variable updated"
}


# finalize_installer - complete install
#    create $INSTALL_INFO_FILE file with info on installed files locations
#    Update udev rules, add user to 'users' group and remove tarball
function finalize_installer()
{
    echo "NCS Libraries have been installed in $SYS_INSTALL_DIR/lib"

    INSTALL_INFO_FILE=$INSTALL_DIR/$INSTALL_INFO_FILENAME
    touch $INSTALL_INFO_FILE
    echo "ncsdk_path=$INSTALL_DIR" > $INSTALL_INFO_FILE
    echo "ncs_lib_path=$SYS_INSTALL_DIR/lib" >> $INSTALL_INFO_FILE
    echo "ncs_inc_path=$SYS_INSTALL_DIR/include" >> $INSTALL_INFO_FILE
    if [ "${INSTALL_TOOLKIT}" = 'yes' ]; then
        echo "ncs_bin_path=$SYS_INSTALL_DIR/bin" >> $INSTALL_INFO_FILE
    fi

    # Update udev rules
    echo "Updating udev rules..."
    $SUDO_PREFIX cp $SDK_DIR/udev/97-usbboot.rules /etc/udev/rules.d/
    RC=0 
    $SUDO_PREFIX udevadm control --reload-rules || RC=$?
    if [ $RC -ne 0 ] ; then
        echo "Warning udevadm control --reload-rules reported an return code = ${RC}"
    fi
    RC=0 
    $SUDO_PREFIX udevadm trigger || RC=$?
    if [ $RC -ne 0 ] ; then
        echo "Warning udevadm trigger return code = ${RC}"
    fi
    
    # Final touch up
    CURRENT_USER=$(id -u -n)    
    echo "Adding user '$CURRENT_USER' to 'users' group"
    $SUDO_PREFIX usermod -a -G users ${CURRENT_USER}

    # cleanup
    if [ -f ${INSTALL_DIR}/ncsdk.tar.gz ] ; then
        $SUDO_PREFIX rm ${INSTALL_DIR}/ncsdk.tar.gz
    fi

    if [ "${INSTALL_CAFFE}" = "yes" ] ; then
        RC=0
        grep "export PYTHONPATH=" $HOME/.bashrc > /dev/null  2>&1 || RC=$?
        if [ $RC -eq 0 ] ; then
            UPDATED_PYTHONPATH_MSG=$(grep "export PYTHONPATH=" $HOME/.bashrc || RC=$?)
        else
            echo -e "${RED}Error, cannot find PYTHONPATH= in $HOME/.bashrc.  Error on line $LINENO.  Will exit${NC}"
            exit 1
        fi
    fi
    
    echo ""
    echo -e "${GREEN}Installation is complete.${NC}"
    echo "Please provide feedback in our support forum if you encountered difficulties."

    if [ "${MOVED_NCAPI1}" == 'yes' ]; then
        echo ""
        echo -e "${YELLOW}Installed NCSDK version: $NCSDK_VERSION"
        echo -e "Moved existing NCSDK 1.x files to ${NCSDK1_ARCHIVE_DIR}${NC}"
        echo " You need to either"
        echo "  - Manually update projects to use NCAPI v2."
        echo "  - Manually configure your projects to use the old NCAPI v1 files which were "
        echo "    moved to ${NCSDK1_ARCHIVE_DIR}" 
        echo ""        
    fi

    if [ "${INSTALL_CAFFE}" = "yes" ] ; then
        echo ""
        echo "The PYTHONPATH environment variable was added to your .bashrc as described in the Caffe documentation."
        echo -e "${YELLOW} New bash processes including new terminals will use the updated value of PYTHONPATH."
        echo ""
        echo "To use the NCSDK in this shell, set PYTHONPATH now:"
        echo ${UPDATED_PYTHONPATH_MSG}
        echo -e ${NC}
    fi

    if [ "${USE_VIRTUALENV}" == 'yes' ]; then
        echo ""
        echo "USE_VIRTUALENV=${USE_VIRTUALENV} - used virtualenv in ${VIRTUALENV_DIR}"
        echo "You must activate this virtualenv to use the NCSDK with python"
        echo -e "${YELLOW} Activate this virtualenv by entering this command now:"
        echo "   source ${VIRTUALENV_DIR}/bin/activate "
        echo -e ${NC}
    fi
}


# main - this is the main function that runs the install
function main()
{
    echo "Movidius Neural Compute Toolkit Installation"
    ### initialization phase
    init_installer

    # If previous install was from NCSDK 1.x release, move them
    detect_and_move_ncsdk1
    
    # Find old installs, if found, print old version and remove it
    # find_previous_install and remove_previous_install are in install-utilities.sh
    find_previous_install
    print_previous_ncsdk_install_info
    remove_previous_install
    
    ### installation phase
    make_installer_dirs
    download_and_copy_files
    install_apt_dependencies
    create_install_logfile
    # Optionally use python virtualenv, USE_VIRTUALENV set in ncsdk.conf
    [ "${USE_VIRTUALENV}" == 'yes' ] && setup_virtualenv
    install_python_dependencies

    ### Install tensorflow and caffe based on settings in ncsdk.conf
    [ "${INSTALL_TENSORFLOW}" = "yes" ] && install_tensorflow
    [ "${INSTALL_CAFFE}" = "yes" ] && install_caffe

    # install SDK based on settings in ncsdk.conf and C/C++ and Python API
    [ "${INSTALL_TOOLKIT}" = "yes" ] && install_sdk
    install_api

    finalize_installer
}
main
exit 0

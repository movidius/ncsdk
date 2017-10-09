#! /bin/bash

#test for python already installed for opencv
python3 -c "import cv2" > /dev/null 2>&1
if [ $? -eq 0 ] ;
then
	echo "";
	echo "OpenCV already setup for python";
	echo "";
	exit 0
fi;

## Install OpenCV
if [[ `lsb_release -d` =~ .*Raspbian.* ]] 
then 
	echo ""
	echo "************************ Please confirm *******************************"
	echo " Installing OpenCV on Raspberry Pi may take a long time. "
	echo " You may skip this part of the installation in which case some examples "
	echo " may not work without modifications but the rest of the SDK will still "
	echo " be functional. Select n to skip OpenCV installation or y to install it." 
	read -p " Continue installing OpenCV (y/n) ? " CONTINUE
	if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then
		echo ""; 
		echo "Installing OpenCV"; 
		echo "";
		sudo apt-get update -y && sudo apt-get upgrade -y
		sudo apt-get install -y build-essential cmake pkg-config
		sudo apt-get install -y libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev
		sudo apt-get install -y libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
		sudo apt-get install -y libxvidcore-dev libx264-dev
		sudo apt-get install -y libgtk2.0-dev libgtk-3-dev
		sudo apt-get install -y libatlas-base-dev gfortran
		sudo apt-get install -y python2.7-dev python3-dev

		cd ~
		wget -O opencv.zip https://github.com/Itseez/opencv/archive/3.3.0.zip
		unzip opencv.zip
		wget -O opencv_contrib.zip https://github.com/Itseez/opencv_contrib/archive/3.3.0.zip
		unzip opencv_contrib.zip
		cd ~/opencv-3.3.0/
		mkdir build
		cd build
		cmake -D CMAKE_BUILD_TYPE=RELEASE \
		      -D CMAKE_INSTALL_PREFIX=/usr/local \
		      -D INSTALL_PYTHON_EXAMPLES=OFF \
		      -D OPENCV_EXTRA_MODULES_PATH=~/opencv_contrib-3.3.0/modules \
		      -D BUILD_EXAMPLES=OFF ..
		make
		sudo make install
		sudo ldconfig
	else
		echo "";
		echo "Skipping OpenCV installation";
		echo "";
	fi
else  
	echo "Installing opencv python for non-Raspbian";
	sudo -H pip3 install opencv-python
	sudo -H pip3 install opencv-contrib-python
fi

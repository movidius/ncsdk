#! /bin/bash

#test for python already installed for opencv
python3 -c "import cv2" > /dev/null 2>&1
if [ $? -eq 0 ] ;
then
	echo ""
	echo "************************ Please confirm *******************************"
	echo " NCSDK 1.11 requires that previous installations of openCV"
	echo " be uninstalled before proceeding with NCSDK installation."
	echo " Note that if you installed opencv via pip3 or from source into the"
	echo " home directory, it will be uninstalled."
	read -p " Continue uninstalling OpenCV (y/n) ? " CONTINUE
	if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then
		echo "";
		echo "OpenCV already setup for python";
		echo "";
		echo "Uninstalling opencv pip installation";
		sudo pip3 uninstall opencv-contrib-python
		sudo pip3 uninstall opencv-python

		echo "Looking for opencv source installation";
		if [ -d "$HOME/opencv-3.3.0" ]; then
			echo "opencv-3.3.0 directory exists"
			if [ -e "$HOME/opencv-3.3.0/build/Makefile" ]; then
				echo "opencv-3.3.0 Makefile exists, attempting to uninstall opencv-3.3.0"
				cd "$HOME/opencv-3.3.0/build"
				sudo make uninstall &> /dev/null
				echo "done."
			fi
		fi
	else
		echo " Not removing opencv, quitting."
		exit 1
	fi
fi

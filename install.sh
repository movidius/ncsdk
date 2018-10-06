#! /bin/bash

# download the file
cp ncsdk.conf /tmp

download_filename=NCSDK-1.12.01.01.tar.gz

# ncsdk_link is the url (set in both install.sh & get_mvcmd.sh)
ncsdk_link=https://downloadmirror.intel.com/28192/eng/NCSDK-1.12.01.01.tar.gz

# download the payload from the redirector link
# and save it the download_filename no matter what the original filename was
wget --no-cache -O ${download_filename} $ncsdk_link

# Get the filenames, ncsdk_archive is just the download filename
# and ncsdk_pkg is the filename without the .tar.gz
#ncsdk_archive=$(echo "$ncsdk_link" | grep -o '[^/]*$')
ncsdk_archive=${download_filename}
ncsdk_pkg=${ncsdk_archive%%.tar.gz}

# now install-ncsdk.sh is doing the uninstallation if necessary.
# Remove older installation dirs
#if [ -f /opt/movidius/uninstall-ncsdk.sh ] 
#then
#	echo "Removing previous install..."
#	/opt/movidius/uninstall-ncsdk.sh
#fi

DIR=$(pwd)

# Create Required installation dirs
sudo mkdir -p /opt/movidius
sudo cp $ncsdk_archive /opt/movidius/ncsdk.tar.gz
cd /opt/movidius/

# untar the new install and run the install script
sudo tar zxvf ./ncsdk.tar.gz
sudo rm -rf NCSDK
sudo mv $ncsdk_pkg* NCSDK
cd /opt/movidius/NCSDK
sudo cp /tmp/ncsdk.conf .

sudo cp ${DIR}/api/include/mvnc.h .
sudo cp ${DIR}/api/include/mvnc_deprecated.h .
sudo cp ${DIR}/api/python/mvnc/mvncapi.py .
sudo cp ${DIR}/api/python/mvnc/__init__.py .
./install-ncsdk.sh

# leave the uninstall script on the target
#sudo cp ./uninstall-ncsdk.sh ../ncsdk/

# cleanup
cd ..
sudo rm ncsdk.tar.gz
#sudo rm -r NCSDK
sudo rm -f /tmp/${ncsdk_archive}





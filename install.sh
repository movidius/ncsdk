#! /bin/bash

# download the file
cp ncsdk.conf /tmp

rm -f /tmp/ncsdk_redirector.txt

if [ -f ncsdk_redirector.txt ]
then
	cp ncsdk_redirector.txt /tmp
	cd /tmp
else
	cd /tmp
	wget --no-cache http://ncs-forum-uploads.s3.amazonaws.com/ncsdk/ncsdk_01_12/ncsdk_redirector.txt
fi

download_filename=NCSDK-1.12.tar.gz

# redirector is the url from redirector text file
redirector=$(<ncsdk_redirector.txt)

# ncsdk_link is the url with '\r' deleted if it exists
# would probably be from editing in windows/dos
ncsdk_link=$(echo "$redirector" | tr -d '\r')

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

# Create Required installation dirs
sudo mkdir -p /opt/movidius
sudo cp $ncsdk_archive /opt/movidius/ncsdk.tar.gz
cd /opt/movidius/

# untar the new install and run the install script
sudo tar zxvf ./ncsdk.tar.gz
sudo rm -rf NCSDK
sudo mv $ncsdk_pkg* NCSDK
cd /opt/movidius/NCSDK
cp /tmp/ncsdk.conf .
./install-ncsdk.sh

# leave the uninstall script on the target
#sudo cp ./uninstall-ncsdk.sh ../ncsdk/

# cleanup
cd ..
sudo rm ncsdk.tar.gz
#sudo rm -r NCSDK
sudo rm -f /tmp/${ncsdk_archive}





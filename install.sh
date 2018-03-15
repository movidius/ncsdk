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
	#wget --no-cache http://ncs-forum-uploads.s3.amazonaws.com/ncsdk/ncsdk_01_12/ncsdk_redirector.txt
	wget --no-cache -O ncsdk_redirector.txt "https://ncs-forum-uploads.s3-us-west-1.amazonaws.com/ncsdk/ncsdk_01_012_B/ncsdk_redirector.txt?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAJPV7SAH5YZ2U3ARA/20180314/us-west-1/s3/aws4_request&X-Amz-Date=20180314T233615Z&X-Amz-Expires=604800&X-Amz-SignedHeaders=host&X-Amz-Signature=8886c9b49d0017ef736c9d226ff6763552f46fa501163fcbe769035c63b84456"
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





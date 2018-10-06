#!/bin/bash
all_devices=$(lsusb -d 03e7:f63b  | cut -d" " -f2,4 | cut -d":" -f1 |  sed 's/ /\//' | sed 's/^/\/dev\/bus\/usb\//')
devices_number=$(lsusb -d 03e7:f63b  | wc -l)
if [ ${devices_number} -eq 0 ] ; then
    echo "No devices connected!"
    exit 0
fi
docker_cmd="docker run --net=host"
for dev in $all_devices
do
    docker_cmd="$docker_cmd --device=$dev"
done

docker_cmd="$docker_cmd  --name ncsdk -i -t ncsdk /bin/bash"
echo $docker_cmd
eval $docker_cmd

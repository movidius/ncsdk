#! /bin/bash

if [ -f /opt/movidius/uninstall-ncsdk.sh ]; then
    echo "running uninstall-ncsdk.sh"
    /opt/movidius/uninstall-ncsdk.sh
else
   echo "could not find uninstall-ncsdk.sh - nothing to uninstall"
fi

sudo rm -rf /opt/movidius/ncsdk


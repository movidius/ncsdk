#!/usr/bin/python3
#
# Copyright 2017 Intel Corporation. 
# The source code, information and material ("Material") contained herein is  
# owned by Intel Corporation or its suppliers or licensors, and title to such  
# Material remains with Intel Corporation or its suppliers or licensors.  
# The Material contains proprietary information of Intel or its suppliers and  
# licensors. The Material is protected by worldwide copyright laws and treaty  
# provisions.  
# No part of the Material may be used, copied, reproduced, modified, published,  
# uploaded, posted, transmitted, distributed or disclosed in any way without  
# Intel's prior express written permission. No license under any patent,  
# copyright or other intellectual property rights in the Material is granted to  
# or conferred upon you, either expressly, by implication, inducement, estoppel  
# or otherwise.  
# Any license under such intellectual property rights must be express and  
# approved by Intel in writing. 

# Python script to open and close a single NCS device

import mvnc.mvncapi as fx

# main entry point for the program
if __name__=="__main__":

     # set the logging level for the NC API
    fx.SetGlobalOption(fx.GlobalOption.LOG_LEVEL, 0)

    # get a list of names for all the devices plugged into the system
    ncs_names = fx.EnumerateDevices()
    if (len(ncs_names) < 1):
        print("Error - no NCS devices detected, verify an NCS device is connected.")
        quit() 


    # get the first NCS device by its name.  For this program we will always open the first NCS device.
    dev = fx.Device(ncs_names[0])

    
    # try to open the device.  this will throw an exception if someone else has it open already
    try:
        dev.OpenDevice()
    except:
        print("Error - Could not open NCS device.")
        quit()


    print("Hello NCS! Device opened normally.")
    

    try:
        dev.CloseDevice()
    except:
        print("Error - could not close NCS device.")
        quit()

    print("Goodbye NCS! Device closed normally.")
    print("NCS device working.")
    

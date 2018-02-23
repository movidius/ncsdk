############################################################################################
# Title: TASS Movidius Helpers
# Description: Helper functions for TASS Movidius.
# Last Modified: 2018/02/21
############################################################################################

import os, json, cv2
from datetime import datetime

class TassMovidiusHelpers():
    
    def __init__(self):

        pass

    def loadConfigs(self):
        
        with open("data/confs.json") as configs:
            
            _configs = json.loads(configs.read())

        return _configs
        
    def printMode(self,mode):
        
        startStream = 0
        
        if mode == "InceptionLive" or mode == "YoloLive":
            
            print("-- YOU ARE IN LIVE MODE, EDIT data/confs.json TO CHANGE MODE TO TEST --")
            print("")
            
            startStream = 1
            
        else:
            
            print("-- YOU ARE IN TEST MODE, EDIT data/confs.json TO CHANGE MODE TO LIVE --")
            print("")
            
        return startStream
        
    def saveImage(self,networkPath,frame):
        
        timeDirectory =  networkPath + "data/captures/"+datetime.now().strftime('%Y-%m-%d')+'/'+datetime.now().strftime('%H')
        
        if not os.path.exists(timeDirectory):
            os.makedirs(timeDirectory)

        currentImage=timeDirectory+'/'+datetime.now().strftime('%M-%S')+'.jpg'
        print(currentImage)
        print("")
        
        cv2.imwrite(currentImage, frame)

        return currentImage
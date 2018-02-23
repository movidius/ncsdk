############################################################################################
# Title: TASS Movidius Core
# Description: Test classification of a local image or classifies live webcam stream.
# Acknowledgement: Uses code from Intel movidius/ncsdk (https://github.com/movidius/ncsdk)
# Acknowledgement: Uses code from gudovskiy/yoloNCS (https://github.com/gudovskiy/yoloNCS)
# Last Modified: 2018/02/22
############################################################################################
        
print("")
print("Welcome to TASS Movidius, please wait while the program initiates...")
print("")

from mvnc import mvncapi as mvnc
import sys,os,time,csv,getopt,json,time 
import cv2
import numpy as np
import techbubbleiotjumpwaymqtt.device as iotJumpway 
from yolo import TassMovidiusYolo
from datetime import datetime
from skimage.transform import resize

print("- Imported Required Modules")

class TassMovidius():
    
    def __init__(self):
        
        """
            CLASSIFIER MODE:
            
            Classifier configuration can be found in data/confs.json
            
                - InceptionTest: This mode sets the program to classify testing images using Inception V3
                - InceptionLive: TODO
                - YoloTest: This mode sets the program to classify testing images using Yolo
                - YoloLive: This mode sets the program to classify from the live webcam feed using Yolo
        """
        
        self._configs = {}
        self.movidius = None
        self.jumpwayClient = None
        self.cameraStream = None
        self.imagePath = None
        
        self.mean = 128
        self.std = 1/128
        
        self.categories = []
        self.graphfile = None
        self.graph = None
        self.reqsize = None

        self.TassMovidiusYolo = None
        self.iterations = None

        self.extensions = [
            ".jpg"
        ]

        with open("data/confs.json") as configs:

            self._configs = json.loads(configs.read())
        
        if self._configs["ClassifierSettings"]["MODE"] == "InceptionLive" or self._configs["ClassifierSettings"]["MODE"] == "YoloLive":
            
            print("-- YOU ARE IN LIVE MODE, EDIT data/confs.json TO CHANGE MODE TO TEST --")
            print("")
            
            self.startStream()
        
        else:
            
            print("-- YOU ARE IN TEST MODE, EDIT data/confs.json TO CHANGE MODE TO LIVE --")
            print("")
        
        self.CheckDevices()
        self.startMQTT()

        if self._configs["ClassifierSettings"]["MODE"] == "InceptionLive":
            
            pass
            
        elif self._configs["ClassifierSettings"]["MODE"] == "YoloLive":
            
            self.TassMovidiusYolo = TassMovidiusYolo()
            self.loadYoloRequirements()
        
        print("")
        print("-- TassMovidius Initiated")
        print("")
            
    def CheckDevices(self):
        
        #mvnc.SetGlobalOption(mvnc.GlobalOption.LOGLEVEL, 2)
        devices = mvnc.EnumerateDevices()
        if len(devices) == 0:
            print('WARNING! No Movidius Devices Found')
            quit()

        self.movidius = mvnc.Device(devices[0])
        self.movidius.OpenDevice()
        
        print("- Movidius Connected")
            
    def AllocateGraph(self,graphfile):

        self.graph = self.movidius.AllocateGraph(graphfile)
        
        print("- Allocated Graph OK")
        
    def loadInceptionRequirements(self):
            
        self.imagePath = self._configs["ClassifierSettings"]["InceptionImagePath"]+self._configs["ClassifierSettings"]["Image"]
        
        with open(self._configs["ClassifierSettings"]["NetworkPath"] + self._configs["ClassifierSettings"]["Graph"], mode='rb') as f:
            
            self.graphfile = f.read()

        self.AllocateGraph(self.graphfile)
        
        print("- Graph Loaded OK")
            
        with open(self._configs["ClassifierSettings"]["NetworkPath"] + 'categories.txt', 'r') as f:
            
            for line in f:
                
                cat = line.split('\n')[0]
                
                if cat != 'classes':
                    
                    self.categories.append(cat)
                    
            f.close()
        
            print("- Categories Loaded OK:", len(self.categories))
        
        with open(self._configs["ClassifierSettings"]["NetworkPath"] + 'inputsize.txt', 'r') as f:
            
            self.reqsize = int(f.readline().split('\n')[0])
        
        print("- Image Size Loaded OK", self.reqsize)
                
    def inceptionTest(self):
                
        testingStart = time.time()
        print("- TESTING INCEPTION V3 STARTED: ",testingStart)
        print("")
        
        self.loadInceptionRequirements()
            
        rootdir=self._configs["ClassifierSettings"]["NetworkPath"] + self._configs["ClassifierSettings"]["InceptionImagePath"]
            
        files = 0
        identified = 0
        for file in os.listdir(rootdir):
            
            if file.endswith(".jpg"):
                
                files = files + 1
                fileName = rootdir+file
        
                print("")
                print("- Loaded Test Image", fileName)
                img = cv2.imread(fileName).astype(np.float32)
                print("")

                detectionStart = time.time()
                print("- DETECTION STARTED: ",detectionStart)
                
                dx,dy,dz= img.shape
                delta=float(abs(dy-dx))
                
                if dx > dy: 
                    
                    img=img[int(0.5*delta):dx-int(0.5*delta),0:dy]
                    
                else:
                    
                    img=img[0:dx,int(0.5*delta):dy-int(0.5*delta)]
                    
                img = cv2.resize(img, (self.reqsize, self.reqsize))
                img=cv2.cvtColor(img,cv2.COLOR_BGR2RGB)

                for i in range(3):
                    
                    img[:,:,i] = (img[:,:,i] - self.mean) * self.std

                self.graph.LoadTensor(img.astype(np.float16), 'user object')
                print('- Loaded Tensor')
                output, userobj = self.graph.GetResult()

                top_inds = output.argsort()[::-1][:5]
    
                now = time.time()
                print("- DETECTION ENDED: {0}".format(now - detectionStart))
                print("")
                print("TASS Detected Image ID", top_inds[0], self.categories[top_inds[0]], "With A Confidence Of", output[top_inds[0]])
                print("")
                
                if output[top_inds[0]] > self._configs["ClassifierSettings"]["InceptionThreshold"]:
                    
                    identified = identified + 1

                    self.jumpwayClient.publishToDeviceChannel(
                            "Sensors",
                            {
                                "Sensor":"CCTV",
                                "SensorID": self._configs["Cameras"][0]["ID"],
                                "SensorValue":"OBJECT: " + self.categories[top_inds[0]] + " (Confidence: " + str(output[top_inds[0]]) + ")"
                            }
                        )
                    
                    print('Published To IoT JumpWay')
                    print("")

                print("".join(['*' for i in range(79)]))
                print('inception-v3 on NCS')
                print("".join(['*' for i in range(79)]))
                
                for i in range(5):
                    
                    print(top_inds[i], self.categories[top_inds[i]], output[top_inds[i]])

                print("".join(['*' for i in range(79)]))
    
        now = time.time()
        print("")
        print("TESTING INCEPTION V3 ENDED")
        print("TESTED:",files)
        print("IDENTIFIED:",identified)
        print("TESTING TIME: {0}".format(now - testingStart))
        print("")
        
        self.graph.DeallocateGraph()
        self.movidius.CloseDevice()
        sys.exit()
        
    def loadYoloRequirements(self):

        network_blob='graph'
        opt = self.movidius.GetDeviceOption(mvnc.DeviceOption.OPTIMISATION_LIST)

        # load blob
        with open(network_blob, mode='rb') as f:

            blob = f.read()

        self.AllocateGraph(blob)
        self.graph.SetGraphOption(mvnc.GraphOption.ITERATIONS, 1)
        self.iterations = self.graph.GetGraphOption(mvnc.GraphOption.ITERATIONS)
        
    def yoloTest(self):
        
        self.TassMovidiusYolo = TassMovidiusYolo()
                
        testingStart = time.time()
        print("- TESTING YOLO STARTED: ",testingStart)
        print("")

        self.loadYoloRequirements()
            
        rootdir=self._configs["ClassifierSettings"]["NetworkPath"] + self._configs["ClassifierSettings"]["YoloImagePath"]
            
        files = 0
        identified = 0
        for file in os.listdir(rootdir):
            
            if file.endswith(".jpg"):
                
                files = files + 1
                fileName = rootdir+file
        
                print("")
                print("- Loaded Test Image", fileName)
                print("")

                detectionStartHuman = datetime.now()
                detectionStart = time.time()
                print("- DETECTION STARTED: ",detectionStartHuman)
                
                dim=(448,448)
                img = cv2.imread(fileName)
                im = resize(img.copy()/255.0,dim,1)
                #im = cv2.cvtColor(im, cv2.COLOR_RGB2BGR)
                im = im[:,:,(2,1,0)]

                #print('NEW shape:',im.shape)
                #print(img[0,0,:],im[0,0,:])
                
                self.graph.LoadTensor(im.astype(np.float16), 'user object')
                print('- Loaded Tensor')
                out, userobj = self.graph.GetResult()
                
                results = self.TassMovidiusYolo.processImage(
                    out.astype(np.float32), 
                    img.shape[1], 
                    img.shape[0]) # fc27 instead of fc12 for yolo_small
                #print (results)
                #cv2.imshow('YOLO detection',img_cv)
    
                now = time.time()
                print("- DETECTION ENDED: {0}".format(now - detectionStart))
                print("")

                savedFrames = self.TassMovidiusYolo.saveFrame(
                    img, 
                    results, 
                    img.shape[1], 
                    img.shape[0])
                    
                for i in range(len(savedFrames)):
                    
                    print("")
                    print("TASS Detected ", savedFrames[i]["class"], "With A Confidence Of", str(savedFrames[i]["Confidence"]))
                    print("")
                    
                    if savedFrames[i]["Confidence"] > self._configs["ClassifierSettings"]["YoloThreshold"]:
                        
                        identified = identified + 1

                        self.jumpwayClient.publishToDeviceChannel(
                                "Sensors",
                                {
                                    "Sensor":"CCTV",
                                    "SensorID": self._configs["Cameras"][0]["ID"],
                                    "SensorValue":"OBJECT: " + savedFrames[i]["class"] + " (Confidence: " + str(savedFrames[i]["Confidence"]) + ")"
                                }
                            )
                        
                        print('Published To IoT JumpWay')
                        print("")

        now = time.time()
        print("")
        print("TESTING YOLO ENDED")
        print("TESTED:",files)
        print("IDENTIFIED:",identified)
        print("TESTING TIME: {0}".format(now - testingStart))
        print("")
        
        self.graph.DeallocateGraph()
        self.movidius.CloseDevice()
        sys.exit()
            
    def startMQTT(self):

        try:

            self.jumpwayClient = iotJumpway.JumpWayPythonMQTTDeviceConnection({
                "locationID": self._configs["IoTJumpWay"]["Location"],
                "zoneID": self._configs["IoTJumpWay"]["Zone"],
                "deviceId": self._configs["IoTJumpWay"]["Device"],
                "deviceName": self._configs["IoTJumpWayDevice"]["Name"],
                "username": self._configs["IoTJumpWayMQTT"]["Username"],
                "password": self._configs["IoTJumpWayMQTT"]["Password"]
            })

        except Exception as e:
            print(str(e))
            sys.exit()

        self.jumpwayClient.connectToDevice()
        
        print("- IoT JumpWay Initiated")
            
    def startStream(self):
        
        self.cameraStream = cv2.VideoCapture(self._configs["Cameras"][0]["URL"])
        
        print("- Camera Stream Initiated")
            
    def saveImage(self,frame):
        
        timeDirectory =  self._configs["ClassifierSettings"]["NetworkPath"] + "data/captures/"+datetime.now().strftime('%Y-%m-%d')+'/'+datetime.now().strftime('%H')
        
        if not os.path.exists(timeDirectory):
            os.makedirs(timeDirectory)

        currentImage=timeDirectory+'/'+datetime.now().strftime('%M-%S')+'.jpg'
        print(currentImage)
        print("")
        
        cv2.imwrite(currentImage, frame)

        return currentImage
        
TassMovidius = TassMovidius()

while True:
    
    if TassMovidius._configs["ClassifierSettings"]["MODE"] == "InceptionTest":

        print("TEST MODE")
        print("")
        
        TassMovidius.inceptionTest()
        
    elif TassMovidius._configs["ClassifierSettings"]["MODE"] == "InceptionLive":
    
        print("INCEPTION LIVE MODE")
        print("")
        
        pass
        
    elif TassMovidius._configs["ClassifierSettings"]["MODE"] == "YoloTest":
    
        print("YOLO TEST MODE")
        print("")
        
        TassMovidius.yoloTest()
        
        pass
        
    elif TassMovidius._configs["ClassifierSettings"]["MODE"] == "YoloLive":
    
        print("- YOLO LIVE MODE")
        print("")
                
        yoloStart = time.time()
        identified = 0
        files = 0
        print("- YOLO STARTED: ",yoloStart)
        print("")

        
        while True:
        
            try:
                
                ret, img = TassMovidius.cameraStream.read()
                if not ret: continue

                files = files + 1

                detectionStartHuman = datetime.now()
                detectionStart = time.time()
                print("- DETECTION STARTED: ",detectionStartHuman)
                
                dim=(448,448)
                im = resize(img.copy()/255.0,dim,1)
                #im = cv2.cvtColor(im, cv2.COLOR_RGB2BGR)
                im = im[:,:,(2,1,0)]

                #print('NEW shape:',im.shape)
                #print(img[0,0,:],im[0,0,:])
                
                TassMovidius.graph.LoadTensor(im.astype(np.float16), 'user object')
                print('- Loaded Tensor')
                out, userobj = TassMovidius.graph.GetResult()
                
                results = TassMovidius.TassMovidiusYolo.processImage(
                    out.astype(np.float32), 
                    img.shape[1], 
                    img.shape[0]) # fc27 instead of fc12 for yolo_small
                #print (results)
                #cv2.imshow('YOLO detection',img_cv)

                now = time.time()
                print("- DETECTION ENDED: {0}".format(now - detectionStart))
                print("")

                savedFrames = TassMovidius.TassMovidiusYolo.saveFrame(
                    img, 
                    results, 
                    img.shape[1], 
                    img.shape[0])

                if len(savedFrames) == 0:
                    
                    print("- Nothing Detected")
                    print("")

                    continue
                    
                for i in range(len(savedFrames)):
                    
                    print("")
                    print("TASS Detected ", savedFrames[i]["class"], "With A Confidence Of", str(savedFrames[i]["Confidence"]))
                    print("")
                    
                    if savedFrames[i]["Confidence"] > TassMovidius._configs["ClassifierSettings"]["YoloThreshold"]:
                        
                        identified = identified + 1

                        TassMovidius.jumpwayClient.publishToDeviceChannel(
                                "Sensors",
                                {
                                    "Sensor":"CCTV",
                                    "SensorID": TassMovidius._configs["Cameras"][0]["ID"],
                                    "SensorValue":"OBJECT: " + savedFrames[i]["class"] + " (Confidence: " + str(savedFrames[i]["Confidence"]) + ")"
                                }
                            )
                        
                        print('Published To IoT JumpWay')
                        print("")
                    
            except cv2.error as e:
                print(e)
                    
        now = time.time()
        print("")
        print("YOLO ENDED")
        print("PROCESSED:",files)
        print("IDENTIFIED:",identified)
        print("TIME: {0}".format(now - testingStart))
        print("")
        
        TassMovidius.graph.DeallocateGraph()
        TassMovidius.movidius.CloseDevice()
        sys.exit()
                
print("SHUTTING DOWN")
print("")

TassMovidius.graph.DeallocateGraph()
TassMovidius.movidius.CloseDevice()
TassMovidius.jumpwayClient.disconnectFromDevice()
        
if TassMovidius._configs["ClassifierSettings"]["MODE"] == "InceptionLive" or TassMovidius._configs["ClassifierSettings"]["MODE"] == "YoloLive":
    
    TassMovidius.cameraStream.release()
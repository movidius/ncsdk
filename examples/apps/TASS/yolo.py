############################################################################################
# Title: TASS Movidius Yolo
# Description: Uses Yolo for object recognition using the Intel Movidius.
# Acknowledgement: Uses code from gudovskiy/yoloNCS (https://github.com/gudovskiy/yoloNCS)
# Last Modified: 2018/02/22
############################################################################################

import os,cv2,json
import numpy as np
from datetime import datetime

class TassMovidiusYolo():
    	
	def __init__(self):
    		
		self._configs = {}
		self.classes = [
			"aeroplane", 
			"bicycle", 
			"bird", 
			"boat", 
			"bottle", 
			"bus", 
			"car", 
			"cat", 
			"chair", 
			"cow", 
			"diningtable", 
			"dog", 
			"horse", 
			"motorbike", 
			"person", 
			"pottedplant", 
			"sheep", 
			"sofa", 
			"train",
			"tvmonitor"
		]
		
		with open("data/confs.json") as configs:
			
			self._configs = json.loads(configs.read())
		
	def processImage(self, output, img_width, img_height):
    		
		w_img = img_width
		h_img = img_height
		#print ((w_img, h_img))
		threshold = 0.2
		iou_threshold = 0.5
		num_class = 20
		num_box = 2
		grid_size = 7
		probs = np.zeros((7,7,2,20))
		class_probs = (np.reshape(output[0:980],(7,7,20)))#.copy()
		#print(class_probs)
		scales = (np.reshape(output[980:1078],(7,7,2)))#.copy()
		#print(scales)
		boxes = (np.reshape(output[1078:],(7,7,2,4)))#.copy()
		offset = np.transpose(np.reshape(np.array([np.arange(7)]*14),(2,7,7)),(1,2,0))
		#boxes.setflags(write=1)
		boxes[:,:,:,0] += offset
		boxes[:,:,:,1] += np.transpose(offset,(1,0,2))
		boxes[:,:,:,0:2] = boxes[:,:,:,0:2] / 7.0
		boxes[:,:,:,2] = np.multiply(boxes[:,:,:,2],boxes[:,:,:,2])
		boxes[:,:,:,3] = np.multiply(boxes[:,:,:,3],boxes[:,:,:,3])

		boxes[:,:,:,0] *= w_img
		boxes[:,:,:,1] *= h_img
		boxes[:,:,:,2] *= w_img
		boxes[:,:,:,3] *= h_img

		for i in range(2):
			for j in range(20):
				probs[:,:,i,j] = np.multiply(class_probs[:,:,j],scales[:,:,i])
		#print (probs)
		filter_mat_probs = np.array(probs>=threshold,dtype='bool')
		filter_mat_boxes = np.nonzero(filter_mat_probs)
		boxes_filtered = boxes[filter_mat_boxes[0],filter_mat_boxes[1],filter_mat_boxes[2]]
		probs_filtered = probs[filter_mat_probs]
		classes_num_filtered = np.argmax(probs,axis=3)[filter_mat_boxes[0],filter_mat_boxes[1],filter_mat_boxes[2]]

		argsort = np.array(np.argsort(probs_filtered))[::-1]
		boxes_filtered = boxes_filtered[argsort]
		probs_filtered = probs_filtered[argsort]
		classes_num_filtered = classes_num_filtered[argsort]

		for i in range(len(boxes_filtered)):
			if probs_filtered[i] == 0 : continue
			for j in range(i+1,len(boxes_filtered)):
				if self.iou(boxes_filtered[i],boxes_filtered[j]) > iou_threshold :
					probs_filtered[j] = 0.0

		filter_iou = np.array(probs_filtered>0.0,dtype='bool')
		boxes_filtered = boxes_filtered[filter_iou]
		probs_filtered = probs_filtered[filter_iou]
		classes_num_filtered = classes_num_filtered[filter_iou]

		result = []
		for i in range(len(boxes_filtered)):
			result.append([self.classes[classes_num_filtered[i]],boxes_filtered[i][0],boxes_filtered[i][1],boxes_filtered[i][2],boxes_filtered[i][3],probs_filtered[i]])

		return result

	def iou(self, box1,box2):
		tb = min(box1[0]+0.5*box1[2],box2[0]+0.5*box2[2])-max(box1[0]-0.5*box1[2],box2[0]-0.5*box2[2])
		lr = min(box1[1]+0.5*box1[3],box2[1]+0.5*box2[3])-max(box1[1]-0.5*box1[3],box2[1]-0.5*box2[3])
		if tb < 0 or lr < 0 : intersection = 0
		else : intersection =  tb*lr
		return intersection / (box1[2]*box1[3] + box2[2]*box2[3] - intersection)

	def saveFrame(self, img, results, img_width, img_height):
		img_cp = img.copy()
		disp_console = True
		imshow = False
	#	if self.filewrite_txt :
	#		ftxt = open(self.tofile_txt,'w')
		dicts = {}
		for i in range(len(results)):
    			
			x = int(results[i][1])
			y = int(results[i][2])
			w = int(results[i][3])//2
			h = int(results[i][4])//2

			dicts[i] = {"class": results[i][0],"Confidence": results[i][5]}
			
			xmin = x-w
			xmax = x+w
			ymin = y-h
			ymax = y+h
			if xmin<0:
				xmin = 0
			if ymin<0:
				ymin = 0
			if xmax>img_width:
				xmax = img_width
			if ymax>img_height:
				ymax = img_height
				
			cv2.rectangle(img_cp,(xmin,ymin),(xmax,ymax),(0,255,0),2)
			#print ((xmin, ymin, xmax, ymax))
			cv2.rectangle(img_cp,(xmin,ymin-20),(xmax,ymin),(125,125,125),-1)
			cv2.putText(img_cp,results[i][0] + ' : %.2f' % results[i][5],(xmin+5,ymin-7),cv2.FONT_HERSHEY_SIMPLEX,0.5,(0,0,0),1)

		if len(results):
			
			timeDirectory =  self._configs["ClassifierSettings"]["NetworkPath"] + "data/captures/"+datetime.now().strftime('%Y-%m-%d')+'/'+datetime.now().strftime('%H')
			
			if not os.path.exists(timeDirectory):
					
				os.makedirs(timeDirectory)

			currentImage=timeDirectory+'/'+datetime.now().strftime('%M-%S')+'.jpg'
			cv2.imwrite(currentImage, img_cp)
			print("- SAVED IMAGE/FRAME")

		#print(dicts)
			
		if imshow :
			cv2.imshow('YOLO detection',img_cp)
			cv2.waitKey(1000)

		return dicts
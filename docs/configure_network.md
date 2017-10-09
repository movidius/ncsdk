# Configuring your network for NCS
This guide will help you get all of the configuration information correct when creating your network for the Movidius Neural Compute Stick.  All of these parameters are critical, if you don't get them right, your network won't give you the accuracy that was achieved by the team that trained the model.  The configuration parameters include:
* mean subtraction
* scale
* color channel configuration
* class prediction
* input image size

**let's go through these one at a time**

## Mean Subtraction
mean substraction on the input data to a CNN is a common technique.  The mean is calculated on the data set.  For example on Imagenet the mean is calculated on a per channel basis to be:
```
104, 117, 123
these numbers are in BGR orientation
```
### Caffe Specific Examples
this mean calculation can be calculated with a tool that comes with caffe: 
[compute_image_mean.cpp](https://github.com/BVLC/caffe/blob/master/tools/compute_image_mean.cpp), 
and Caffe provides a script to do it as well: 
[make_imagenet_mean.sh](https://github.com/BVLC/caffe/blob/master/examples/imagenet/make_imagenet_mean.sh)

---
this will create an output file often called mean_binary.proto.  You can see an example of this in the training prototxt file for AlexNet

[train_val.prototxt](https://github.com/BVLC/caffe/blob/master/models/bvlc_alexnet/train_val.prototxt)
```
 transform_param {
    mirror: true
    crop_size: 227
    mean_file: "data/ilsvrc12/imagenet_mean.binaryproto"
  }
```
---
in the GoogLeNet prototxt file they have just put the values directly:

[train_val.prototxt](https://github.com/BVLC/caffe/blob/master/models/bvlc_googlenet/train_val.prototxt)
```

  transform_param {
    mirror: true
    crop_size: 224
    mean_value: 104
    mean_value: 117
    mean_value: 123
  }
```
---
some models don't use mean subtraction, see below for LeNet as an example.  There is no mean in the transform_param, but there is a scale which we'll get to later

[lenet_train_test.prototxt](https://github.com/BVLC/caffe/blob/master/examples/mnist/lenet_train_test.prototxt)
```
  transform_param {
    scale: 0.00390625
  }
  ```
### TensorFlow specific examples
TensorFlow documentation of mean is not as straight forward as Caffe.  The TensorFlow Slim models for image classification are a great place to get high quality pre-trained models:

[slim models](https://github.com/tensorflow/models/tree/master/research/slim#pre-trained-models)

I could find this the following file the mean (and scale) for both Inception V3 and MobileNet V1

[retrain script](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/image_retraining/retrain.py#L872)
```
input_mean = 128
```
in the case of the InceptionV3 model there is not a per color channel mean, the same mean is used for all channels.  This mean should apply to all of the Inception and MobileNet models, but other models there might be different.
for example, the VGG16 model just had the weights converted from Caffe.  If we look at the link to the VGG16 for Caffe page, we see the means are done like the other Caffe models:
```
https://gist.github.com/ksimonyan/211839e770f7b538e2d8#description

the following BGR values should be subtracted: [103.939, 116.779, 123.68]
```

## Scale
typical 8 bit per pixel per channel images will have a scale of 0-255.  Many CNN networks use the native scale, but some don't.  As was seen in a snippet of the Caffe prototxt file, the **transform_param** would show whether there was a scale.  In the example of LeNet for Caffe, you can see it has a scale pameter of **0.00390625**

[lenet_train_test.prototxt](https://github.com/BVLC/caffe/blob/master/examples/mnist/lenet_train_test.prototxt)
```
  transform_param {
    scale: 0.00390625
  }
  ```
  this may seem like a strange number, but it is actually just 1/256.  So the input 8 bit image is being scaled down to an image from 0-1 instead of 0-255
 
 ---
Back to the example of TensorFlow for Inception V3.  Below the **input_mean** the **input_std** is also listed.  All this is is a scaling factor.  You divide 255/128 and it's about 2.  So in this case, the scale is two, but the mean subtraction is 128.  So in the end the scale is actually -1 to 1

[retrain script](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/image_retraining/retrain.py#L872)
```
input_mean = 128
input_std = 128
```
## Color Channel configuration
different models may be trained with different color channel orientations (either RGB or BGR).  Typically Caffe models seem to be trained with BGR whereas the Slim TensorFlow models (at least Inception and MobileNet) are trained in RGB.  

Once you figure out the color channel orientation for your model, you will need to know the way the image is loaded.  For example opencv will open images in BGR but skimiage will open the image in RGB.  
```
skimage.io.imread will open the image in RGB
cv2.imread will open the image in BGR
Caffe trained models will probably be BGR
TensorFlow trained models will probably be in RGB
```
## Categories
for models that are trained on the Imagenet database, some have 1000 output classes, and some have 1001 output classes.  The extra output class is a background class.  The list below has the list of the 1000 classes not including the background.
[synset_words.txt](https://github.com/HoldenCaulfieldRye/caffe/blob/master/data/ilsvrc12/synset_words.txt)
Most Caffe trained models seem to follow the 1000 class convention, and TensorFlow trained models follow the 1001 class convention.  So for the TensorFlow models, an offset needs to be added.  You can see this as documented in the TensorFlow github [here](https://github.com/tensorflow/models/tree/master/research/slim#the-resnet-and-vgg-models-have-1000-classes-but-the-imagenet-dataset-has-1001)

# Putting it all together
now with all of these factors, let's go through two examples

## Caffe Example
let's use the Berkeley Caffe GoogLeNet model as an example.  the basic model parameters are:
```
Scale: 0-255 (before mean subtraction)
Mean: based on mean_binary.proto file
Color channel configuration: BGR
output categories: 1000
input size: 224x224
labels_offset=0
```
code snippet:
```
#load the label files
labels_offset=0 # no background class offset
labels_file='./synset_words.txt'
labels=numpy.loadtxt(labels_file,str,delimiter='\t')
#Load blob
with open('./googlenet.blob', mode='rb') as f:
	blob = f.read()
graph = device.AllocateGraph(blob)
graph.SetGraphOption(mvnc.GraphOption.ITERATIONS, 1)
iterations = graph.GetGraphOption(mvnc.GraphOption.ITERATIONS)
img = cv2.imread('./dog.jpg') # using OpenCV for reading the image, it will be in BGR
img=cv2.resize(img,(224,224)) # resize to 224x224
img-=[104,117,124] # subtract mean
#run, get the result and print results per the synset_words.txt
graph.LoadTensor(img.astype(numpy.float16), 'user object')
output, userobj = graph.GetResult()
order = output.argsort()[::-1][:6]
print('\n------- predictions --------')
for i in range(0,5):
	print ('prediction ' + str(i) + ' is ' + labels[order[i]-labels_offset])
```

## TensorFlow example
let's use the TensorFlow Slim Inception V3
```
Scale: -1 to 1 (after mean subtraction)
Mean: 128
Color channel configuration: RBG
output categories: 1001
input size: 299x299
labels_offset=1
```
code snippet:
```
#load the label files
labels_offset=1 # background class offset of 1
labels_file='./synset_words.txt'
labels=numpy.loadtxt(labels_file,str,delimiter='\t')
#Load blob
with open('./inceptionv3.blob', mode='rb') as f:
	blob = f.read()
graph = device.AllocateGraph(blob)
graph.SetGraphOption(mvnc.GraphOption.ITERATIONS, 1)
iterations = graph.GetGraphOption(mvnc.GraphOption.ITERATIONS)
#import the image and do the proper scaling
img = cv2.imread('./dog.jpg').astype(numpy.float32) # using OpenCV for reading the image, it will be in BGR
img=cv2.resize(img,(299,299)) # resize to 299x299
img=cv2.cvtColor(img,cv2.COLOR_BGR2RGB) # need to convert to RBG
img-=[128,128,128] # subtract mean 
img /=128. # scale the image
#run, get the result and print results per the synset_words.txt
graph.LoadTensor(img.astype(numpy.float16), 'user object')
output, userobj = graph.GetResult()
order = output.argsort()[::-1][:6]
print('\n------- predictions --------')
for i in range(0,5):
	print ('prediction ' + str(i) + ' is ' + labels[order[i]-labels_offset])
```

feedback or comments?  let me know darren.s.crews@intel.com

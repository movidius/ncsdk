# Configuring Your Network for Intel® Movidius™ NCS
This guide will help you get all of the configuration information correct when creating your network for the Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS). All of these parameters are critical. If you don't get them right, your network won't give you the accuracy that was achieved by the team that trained the model. The configuration parameters are as follows:
* Mean subtraction
* Scale
* Color channel configuration
* Class prediction
* Input image size

**Let's go through these one at a time.**

## Mean Subtraction
Mean subtraction on the input data to a convolutional neural network (CNN) is a common technique. The mean is calculated on the data set. For example, the mean on Imagenet is calculated on a per channel basis to be:
```
104, 117, 123
These numbers are in BGR orientation.
```
### Caffe Specific Examples
This mean calculation can be calculated with a tool that comes with caffe ([compute_image_mean.cpp](https://github.com/BVLC/caffe/blob/master/tools/compute_image_mean.cpp)). Caffe provides a script to do it, as well ([make_imagenet_mean.sh](https://github.com/BVLC/caffe/blob/master/examples/imagenet/make_imagenet_mean.sh)).

---
This will create an output file often called mean_binary.proto. You can see an example of this in the training prototxt file for AlexNet:

[train_val.prototxt](https://github.com/BVLC/caffe/blob/master/models/bvlc_alexnet/train_val.prototxt)
```
 transform_param {
    mirror: true
    crop_size: 227
    mean_file: "data/ilsvrc12/imagenet_mean.binaryproto"
  }
```
---
In the GoogLeNet prototxt file, they have put in the values directly:

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
Some models don't use mean subtraction. See the LeNet example below. There is no mean in the transform_param, but there is a scale that we'll get to later:

[lenet_train_test.prototxt](https://github.com/BVLC/caffe/blob/master/examples/mnist/lenet_train_test.prototxt)
```
  transform_param {
    scale: 0.00390625
  }
  ```
### TensorFlow™ Specific Examples
TensorFlow™ documentation of mean is not as straightforward as Caffe. The TensorFlow Slim models for image classification are a great place to get high quality pre-trained models:

[slim models](https://github.com/tensorflow/models/tree/master/research/slim#pre-trained-models)

The following file is the mean (and scale) for both Inception V3 and MobileNet V1:

[retrain script](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/image_retraining/retrain.py#L872)
```
input_mean = 128
```
In the case of the Inception V3 model, there is not a per color channel mean. The same mean is used for all channels. This mean should apply to all of the Inception and MobileNet models, but other models might be different.

For example, the VGG16 model had the weights converted from Caffe. If we look at the link to the VGG16 for Caffe page, we see the means are done like the other Caffe models:
```
https://gist.github.com/ksimonyan/211839e770f7b538e2d8#description

The following BGR values should be subtracted: [103.939, 116.779, 123.68]
```

## Scale
Typical 8-bit per pixel per channel images will have a scale of 0-255. Many CNN networks use the native scale, but some don't. As was seen in a snippet of the Caffe prototxt file, the **transform_param** would show whether there was a scale. In the example of LeNet for Caffe, you can see it has a scale pameter of **0.00390625**.

[lenet_train_test.prototxt](https://github.com/BVLC/caffe/blob/master/examples/mnist/lenet_train_test.prototxt)
```
  transform_param {
    scale: 0.00390625
  }
  ```
This may seem like a strange number, but it is actually just 1/256. The input 8-bit image is being scaled down to an image from 0-1 instead of 0-255.
 
 ---
Regarding the example of TensorFlow for Inception V3, the **input_mean** the **input_std** are listed below. This is a scaling factor. You divide 255/128, and it's about 2. In this case, the scale is two, but the mean subtraction is 128. In the end, the scale is actually -1 to 1.

[retrain script](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/image_retraining/retrain.py#L872)
```
input_mean = 128
input_std = 128
```
## Color Channel Configuration
Different models may be trained with different color channel orientations (either RGB or BGR). Typically, Caffe models seem to be trained with BGR, whereas the Slim TensorFlow models (at least Inception and MobileNet) are trained in RGB.  

Once you figure out the color channel orientation for your model, you will need to know the way the image is loaded. For example, opencv will open images in BGR, but skimiage will open the image in RGB.  
```
* skimage.io.imread will open the image in RGB
* cv2.imread will open the image in BGR
* Caffe trained models will probably be BGR
* TensorFlow trained models will probably be in RGB
```
## Categories
For models that are trained on the ImageNet database, some have 1000 output classes and some have 1001 output classes. The extra output class is a background class. The following list has the list of the 1000 classes not including the background:
[synset_words.txt](https://github.com/HoldenCaulfieldRye/caffe/blob/master/data/ilsvrc12/synset_words.txt)

Most Caffe trained models seem to follow the 1000 class convention, and TensorFlow trained models follow the 1001 class convention. For the TensorFlow models, an offset needs to be added. You can see this documented in the [TensorFlow GitHub](https://github.com/tensorflow/models/tree/master/research/slim#the-resnet-and-vgg-models-have-1000-classes-but-the-imagenet-dataset-has-1001).

# Putting It All Together
Now with all of these factors, let's go through two examples.

## Caffe Example
Let's use the Berkeley Caffe GoogLeNet model as an example. The basic model parameters are:
```
Scale: 0-255 (before mean subtraction)
Mean: based on mean_binary.proto file
Color channel configuration: BGR
output categories: 1000
input size: 224x224
labels_offset=0
```
Code snippet:
```
#Load the label files
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
#Run, get the result and print results per the synset_words.txt
graph.LoadTensor(img.astype(numpy.float16), 'user object')
output, userobj = graph.GetResult()
order = output.argsort()[::-1][:6]
print('\n------- predictions --------')
for i in range(0,5):
	print ('prediction ' + str(i) + ' is ' + labels[order[i]-labels_offset])
```

## TensorFlow Example
Let's use the TensorFlow Slim Inception V3:
```
Scale: -1 to 1 (after mean subtraction)
Mean: 128
Color channel configuration: RBG
output categories: 1001
input size: 299x299
labels_offset=1
```
Code snippet:
```
#Load the label files
labels_offset=1 # background class offset of 1
labels_file='./synset_words.txt'
labels=numpy.loadtxt(labels_file,str,delimiter='\t')
#Load blob
with open('./inceptionv3.blob', mode='rb') as f:
	blob = f.read()
graph = device.AllocateGraph(blob)
graph.SetGraphOption(mvnc.GraphOption.ITERATIONS, 1)
iterations = graph.GetGraphOption(mvnc.GraphOption.ITERATIONS)
#Import the image and do the proper scaling
img = cv2.imread('./dog.jpg').astype(numpy.float32) # using OpenCV for reading the image, it will be in BGR
img=cv2.resize(img,(299,299)) # resize to 299x299
img=cv2.cvtColor(img,cv2.COLOR_BGR2RGB) # need to convert to RBG
img-=[128,128,128] # subtract mean 
img /=128. # scale the image
#Run, get the result and print results per the synset_words.txt
graph.LoadTensor(img.astype(numpy.float16), 'user object')
output, userobj = graph.GetResult()
order = output.argsort()[::-1][:6]
print('\n------- predictions --------')
for i in range(0,5):
	print ('prediction ' + str(i) + ' is ' + labels[order[i]-labels_offset])
```

Feedback or comments? Let me know at darren.s.crews@intel.com.

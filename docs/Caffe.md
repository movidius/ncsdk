# Caffe Support

## Introduction
[Caffe](http://caffe.berkeleyvision.org/) is a deep learning framework developed by Berkeley AI Research ([BAIR](http://bair.berkley.edu)) and by community contributors. The setup script currently downloads Berkley Vision and Learning Center (BVLC) Caffe and installs it in a system location. Other versions of Caffe are not supported at this time. For more information, please visit http://caffe.berkeleyvision.org/.

* Default Caffe installation location: /opt/movidius/caffe<br>
* Check out Berkley Vision's Web Image Classification [demo](http://demo.caffe.berkeleyvision.org/)

## Caffe Zoo
Berkley Vision hosts a Caffe Model Zoo for researchers and engineers to contribute Caffe models for various tasks. Please visit the [Berkley Caffe Zoo](http://caffe.berkeleyvision.org/model_zoo.html) page to learn more about the caffe zoo, how to create your own Caffe Zoo model, and contribute.

Caffe Zoo has several models contributed, including a model network that can classify images for age and gender. This network, trained by [Gil Levi](https://gist.github.com/GilLevi) and Tal Hassner, is available at [Gender Net Caffe Zoo Model on GitHub](https://gist.github.com/GilLevi/c9e99062283c719c03de).

Caffe models consists of two files that are used for compiling the caffe model using the [Neural Compute Compiler](tools/compile.md).
* Caffe Network Description (.prototxt): Text file that describes the topology and layers of the network
* Caffe Weights (.caffemodel): Contains the weights for each layer that are obtained after training a model

## Neural Compute Caffe Layer Support
The following layers are supported in Caffe by the Intel® Movidius™ Neural Compute SDK. The Intel® Movidius™ Neural Compute Stick does not support training, so some layers that are only required for training are not supported.

### Activation/Neuron	
* bias
* elu
* prelu
* relu
* scale
* sigmoid
* tanh

### Common	
* inner_product

### Normalization	
* batch_norm
* lrn

### Utility	
* concat
* eltwise
* flatten
* parameter
* reshape
* slice
* softmax

### Vision	
* conv 
  * Regular Convolution - 1x1s1, 3x3s1, 5x5s1, 7x7s1, 7x7s2, 7x7s4
  * Group Convolution - <1024 groups total
* deconv
* pooling

# Known Issues
### Caffe Input Layer

Limitation: Batch Size, which is the first dimension, must always be 1

Limitation: The number of inputs must be 1

Limitation: We don't support this "input_param" format for the input layer:

```
1 name: "GoogleNet" 
2 layer { 
3   name: "data" 
4   type: "Input" 
5   top: "data" 
6   input_param { shape: { dim: *10* dim: 3 dim: 224 dim: 224 } } 
7 } 
```

We only support this "input_shape" format for the input layer:

```
name: "GoogleNet" 
input: "data"
 input_shape 
{ dim:1 dim:3 dim:224 dim:224 } 
```

### Input Name
Input name should be always called "data"
This works:
```
name: "GoogleNet" 
input: "data"
 input_shape 
{ dim:1 dim:3 dim:224 dim:224 } 
```
This does not:
```
name: "GoogleNet" 
input: "data_x"
 input_shape 
{ dim:1 dim:3 dim:224 dim:224 } 
```

### Non-Square Convolutions
Limitation: We don't support non-square convolutions such as 1x20.
```
input: "data"
 input_shape 
{ dim:1 dim:1 dim:1 dim:64 } 
layer {
 name: "conv1_x"
 type: "Convolution"
 bottom: "data"
 top: "conv1_x"
  convolution_param {
   num_output: 3
   kernel_h: 1
   kernel_w: 20
   stride: 1
  }
```

### Crop Layer
Limitation: Crop layer cannot take reference size layer from input:"data".

```
layer {
  name: "score"
  type: "Crop"
  bottom: "upscore"
  bottom: "data"
  top: "score"
  crop_param {
    axis: 2
    offset: 18
  }
}
```

### Size Limitations
Compiled Movidius "graph" file < 320 MB; 
Intermediate layer buffer size < 100 MB
```
[Error 35] Setup Error: Not enough resources on Myriad to process this network
```

Scratch Memory size < 112 KB

```
[Error 25] Myriad Error: "Matmul scratch memory [112640] lower than required [165392]"
```

## Caffe Networks
The following networks are validated and known to work on the Intel Movidius Neural Compute SDK:
- GoogleNet V1
- SqueezeNet V1.1
- LeNet
- CaffeNet
- VGG (Sousmith VGG_A)
- AlexNet
- TinyYolo v1

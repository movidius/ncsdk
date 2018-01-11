# mvNCCheck

Type|Function
------------ | -------------
Library|Command Line Tools
Output|
Revision|1.08
See also| [mvNCCompile](compile.md), [mvNCProfile](profile.md), [TensorFlow™ Info](../TensorFlowUsage.md)

## Overview
This command line tool validates (checks) a Caffe or TensorFlow™ nerual network on the Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS.)  The check is done by running an inference on the NCS and also on the host computer in software using the supplied network and appropriate framework libraries.  The results for both inferences (NCS results vs. framework's expected results) are compared to determine a if the network passes or fails and the top 5 inference results are provided as output. 


## Syntax

### Caffe
```bash
mvNCCheck network.prototxt [-w weights_file] [-s Max Number of Shaves] [-in Input Node Name] [-on Output Node Name] [-is Input-Width Input-Height] [-o Output Graph Filename] [-i image filename] [-id Top-1 Validation ID] [-S scale factor] [-M Mean Subtraction Number or npy filename] [-cs Color Sequence]
```

### TensorFlow™
```bash
mvNCCheck network.meta [-s Max Number of Shaves] [-in Input Node Name] [-on Output Node Name] [-is Input-Width Input-Height] [-o Output Graph Filename] [-i image filename] [-id Top-1 Validation ID] [-S scale factor] [-M Mean Subtraction Number or npy filename] [-cs Color Sequence]
```

Argument|Description
------------ | -------------
network.prototxt(caffe)<br>network.meta(TensorFlow™)|Name of the network file. 
[-h --help] | Display help for the command
[-w weights_file]|Weights filename from training. For Caffe this is the .caffemodel file name.  For TensorFlow™ it must be the network name prefix. If omitted zero weights will be used for Caffe models. 
[-s Max # of Shaves]|Default: 1<br><br>Selects the maximum number of SHAVEs (1,2,4,8, or 12) to use for network layers.<br><br>Note: The NCS runtime code may use less than the MAX SHAVE value for some layers where measurements have typically shown no inference performance degradation (and consequently a power benefit) of using fewer SHAVEs.
[-in Input Node Name]|By default the network is processed from the input tensor. This option allows a user to select an alternative start point in the network.<br><br>This enables partial network processing. When used together with the -on option, a user can isolate one or more layers in a network for analysis.
[-on Output Node Name]|By default the network is processed through to the output tensor. This option allows a user to select an alternative end point in the network.<br><br>This enables partial network processing. When used together with the -in option, a user can isolate one or more layers in a network for analysis.
[-is Input-Width Input-Height]|Input size is typically described as a part of the network. For networks that do not have dimension constraints on the input tensor, this option can be used to set the desired input dimensions.<br><br>Only two dimensions are defined because the batch size is always 1 and the number of color planes is assumed to be 3.
[-o Output Graph Filename]|Default: "graph"<br><br>Output graph container filename. If not provided, “graph” will be used.
[-i image filename]|Image to use as input to validation run.<br>If not set, a randomly generated image will be used.
[-id Top-1 Validation ID]|Expected id for Top-1 validation.
[-S scale factor]|Scale each value of the input by this amount.<br>E.g., if the network expects input values in the range 0-255, put 255 here (1 is default, as the range 0-1 is the default).
[-M Mean Subtraction Number or npy filename]|Subtract this from the input (applied after scale). E.g., if the network expects a mean file to be subtracted from the input image, put it here.
[-cs Color Sequence]|Color Sequence of Input channels:<br>  2,1,0: BGR (Default) <br>  0,1,2 : RGB

## Known Issues

## Example
### Caffe
```bash

mvNCCheck deploy.prototxt -w bvlc_googlenet.caffemodel -s 12 -in input -on prob -is 224 224 -o GoogLeNet.graph -cs 2,1,0

```
### TensorFlow™
```bash
mvNCCheck inception_v1.meta -s 12 -in=input -on=InceptionV1/Logits/Predictions/Reshape_1 -is 224 224 -o InceptionV1.graph -cs 0,1,2

```

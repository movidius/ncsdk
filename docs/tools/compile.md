# mvNCCompile

Type|Function
------------ | -------------
Library|Command Line Tools
Output| Compiled 'graph' file 
Revision|1.08
See also| [mvNCProfile](profile.md), [mvNCCheck](check.md), [TensorFlow™ Info](../TensorFlow.md)

## Overview
This command line tool compiles and converts the network file and weights file described in Caffe or TensorFlow™ into Intel® Movidius™ internal Graphfile format. The graph file is loaded into the Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS)
during runtime using the NCSDK API. The graph file then can be executed by sending an image to the Intel Movidius NCS for inferencing.

## Syntax

### Caffe
```bash
mvNCCompile network.prototxt [-w weights_file] [-s Max Number of Shaves] [-in Input Node Name] [-on Output Node Name] [-is Input-Width Input-Height] [-o Output Graph Filename]
```
### TensorFlow™
```bash
mvNCCompile network.meta [-s Max Number of Shaves] [-in Input Node Name] [-on Output Node Name] [-is Input-Width Input-Height] [-o Output Graph Filename]
```

Argument|Description
------------ | -------------
network.prototxt(Caffe)<br>network.meta(TensorFlow™)|Name of the network file. 
[-w weights_file]|Weights filename from training (only applies to Caffe, not to be used with TensorFlow™.) If omitted, zero weights will be used. 
[-s Max # of Shaves]|Default: 1<br><br>Selects the maximum number of SHAVEs (1, 2, 4, 8, or 12) to use for network layers.<br><br>Note: The NCS runtime code may use less than the MAX SHAVE value for some layers where measurements have typically shown no inference performance degradation (and consequently a power benefit) of using fewer SHAVEs.
[-in Input Node Name]|By default the network is processed from the input tensor. This option allows a user to select an alternative start point in the network.<br><br>This enables partial network processing. When used together with the -on option, a user can isolate one or more layers in a network for analysis.
[-on Output Node Name]|By default the network is processed through to the output tensor. This option allows a user to select an alternative end point in the network.<br><br>This enables partial network processing. When used together with the -in option, a user can isolate one or more layers in a network for analysis. Note: Beware that the parser stops at the first instance of the output node name (e.g., a Relu following a Conv will not be processed if it shares the same name).
[-is Input-Width Input-Height]|Input size is typically described as a part of the network. For networks that do not have dimension constraints on the input tensor, this option can be used to set the desired input dimensions.<br><br>Only two dimensions are defined because the batch size is always 1 and the number of color planes is assumed to be 3.
[-o Output Graph Filename]|Default: "graph"<br><br>Output graph container filename. If not provided, “graph” will be used.

## Known Issues

## Example
### Caffe
```bash

mvNCCompile deploy.prototxt -w bvlc_googlenet.caffemodel -s 12 -in input -on prob -is 224 224 -o GoogLeNet.graph

```
### TensorFlow™
```bash

mvNCCompile inception-v1.meta -s 12 -in=input -on=InceptionV1/Logits/Predictions/Reshape_1 -is 224 224 -o InceptionV1.graph

```


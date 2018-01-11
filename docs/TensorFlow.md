# TensorFlow™ Support

# Introduction
[TensorFlow™](https://www.tensorflow.org/) is a deep learning framework pioneered by Google. The NCSDK introduced TensorFlow™ support with the 1.09.xx NCSDK release and TensorFlow™ 1.3. Validation for each release happens on the TensorFlow™ version noted in the release notes. As described on the TensorFlow™ website, "TensorFlow™ is an open source software library for numerical computation using data flow graphs. Nodes in the graph represent mathematical operations, while the graph edges represent the multidimensional data arrays (tensors) communicated between them."

* Default installation location: /opt/movidius/tensorflow

# TensorFlow Model Zoo
TensorFlow has a model GitHub repo at https://github.com/tensorflow/models similar to the Caffe Zoo for Caffe. The TensorFlow models GitHub repository contains several models that are maintained by the respective authors, unlike Caffe, which is not a single GitHub repo.

# Save Session with Graph and Checkpoint Information
The code below shows one way to save a tensorflow session which can be compiled for the NCS.
```python
import numpy as np
import tensorflow as tf

from tensorflow.contrib.slim.nets import inception

slim = tf.contrib.slim

def run(name, image_size, num_classes):
  with tf.Graph().as_default():
    image = tf.placeholder("float", [1, image_size, image_size, 3], name="input")
    with slim.arg_scope(inception.inception_v1_arg_scope()):
        logits, _ = inception.inception_v1(image, num_classes, is_training=False, spatial_squeeze=False)
    probabilities = tf.nn.softmax(logits)
    init_fn = slim.assign_from_checkpoint_fn('inception_v1.ckpt', slim.get_model_variables('InceptionV1'))

    with tf.Session() as sess:
        init_fn(sess)
        saver = tf.train.Saver(tf.global_variables())
        saver.save(sess, "output/"+name)

run('inception-v1', 224, 1001)
```
# Compile for TensorFlow
The command here shows how to compile the saved session from the above code sample
```
mvNCCompile output/inception-v1.meta -in=input -on=InceptionV1/Logits/Predictions/Reshape_1 -s12

```

# Guidence for Compiling TensorFlow Networks
If you are training a TensorFlow network you will want to [follow the guidance for creating an inference only version of the network](tf_compile_guidance.md) that is suitable for compiling via the ncsdk compiler.

# TensorFlow Networks Supported
* Inception V1
* Inception V2
* Inception V3
* Inception V4
* Inception ResNet V2
* MobileNet_v1_1.0 variants:
    * MobileNet_v1_1.0_224
    * MobileNet_v1_1.0_192
    * MobileNet_v1_1.0_160
    * MobileNet_v1_1.0_128
    * MobileNet_v1_0.75_224
    * MobileNet_v1_0.75_192
    * MobileNet_v1_0.75_160
    * MobileNet_v1_0.75_128
    * MobileNet_v1_0.5_224
    * MobileNet_v1_0.5_192
    * MobileNet_v1_0.5_160
    * MobileNet_v1_0.5_128
    * MobileNet_v1_0.25_224
    * MobileNet_v1_0.25_192
    * MobileNet_v1_0.25_160
    * MobileNet_v1_0.25_128
    
_*see release notes for supported networks for a particular release_



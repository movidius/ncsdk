# TensorFlow™ Support

# Introduction
[TensorFlow™](https://www.tensorflow.org/) is a deep learning framework pionered by Google. The NCSDK introduced TensorFlow™ support with the 1.09.xx NCSDK release.  Validation has been done with TensorFlow™ r1.3.  The TensorFlow™ website describes it as "TensorFlow™ is an open source software library for numerical computation using data flow graphs. Nodes in the graph represent mathematical operations, while the graph edges represent the multidimensional data arrays (tensors) communicated between them."

Default Installation Location: /opt/movidius/tensorflow

# TensorFlow™ Model Zoo
TensorFlow™ has a model GitHub repo at https://github.com/tensorflow/models similar to the Caffe Zoo for Caffe.  The TensorFlow™ models GitHub repository contains several models which are maintained by the respective autors unlike Caffe which is not a single GitHub repo.

# Save Session with graph and checkpoint information

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
# Compile for TensorFlow™

```
mvNCCompile output/inception-v1.meta -in=input -on=InceptionV1/Logits/Predictions/Reshape_1 -s12

```

# Neural Compute TensorFlow™ Layer Support

# TensorFlow™ Networks Supported
* Inception V1
* Inception V3
* Inception V4
* Inception ResNet V1 (FaceNet)
* Inception ResNet V2
* MobileNet



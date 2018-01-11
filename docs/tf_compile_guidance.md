# Guidence for Compiling TensorFlow™ Networks
Below you will find general guidance for compiling a TensorFlow™ network that was built for training rather than inference.  The general guidance is illustrated with changes to make to the [mnist_deep.py available from the tensorflow github repository](https://github.com/tensorflow/tensorflow/blob/r1.4/tensorflow/examples/tutorials/mnist/mnist_deep.py).  The changes are shown as typical diff output where a '-' at the front of a line indicates the line is removed, and a '+' at the front of a line indicates the line should be added.  Lines without a '-' or '+' are unchanged and provided for context.

In order to compile a TensorFlow™ network for the NCS you will need to save a version of the network that is specific to deployment/inference and omits the training features.  The following list of steps includes what users need to do to compile a typical TensorFlow™ network for the NCS.  Every step may not apply to every network, but should be taken as general guidence.


- Make sure there is a name set for the first layer of the network.  This is not strictly required but makes compiling much easier because if you don't explicitly name the first and last layer you will need to determine what name those layers were given and provide those to the compiler.  For mnist_deep.py you would make the following change for the first node to give it the name "input":

```python
   # Create the model
-  x = tf.placeholder(tf.float32, [None, 784])
+  x = tf.placeholder(tf.float32, [None, 784], name="input")
```

- Add tensorflow code to save the trained network.  For mnist_deep.py the change to save the trained network is:

```python
+  saver = tf.train.Saver()
+
   with tf.Session() as sess:
...

   print('test accuracy %g' % accuracy.eval(feed_dict={
       x: mnist.test.images, y_: mnist.test.labels, keep_prob: 1.0}))
+
+  graph_location = "."
+  save_path = saver.save(sess, graph_location + "/mnist_model")
```

- Run the code to train the network and make sure saver.save() is called to save the trained network.  After the program completes, if it was successful, saver.save() will have created the following files:
  - mnist_model.index
  - mnist_model.data-00000-of-00001
  - mnist_model.meta

- Remove training specific code from the network, and add code to read in the previously saved network to create an inference only version.  For this step its advised that you copy the original tensorflow code to a new file and modify the new file.  For example if you are working with mnist_deep.py you could copy that to mnist_deep_inference.py.  Things to remove from the inference code are:
  - Dropout layers
  - Training specific code 
    - Reading or importing training and testing data
    - Cross entropy/accuracy code
    - Placeholders except the input tensor.

The ncsdk compiler does not resolve unknown placeholders.  Often extra placeholders are used for training specific variables so they are not necessary for inference.  Placeholder variables that cannot be removed should be replaced by constants in the inference graph.
    

For mnist_deep.py you would make the following changes

```python
import tempfile 
-  from tensorflow.examples.tutorials.mnist import input_data

...
-  # Dropout - controls the complexity of the model, prevents co-adaptation of
-  # features.
-  with tf.name_scope('dropout'):
-    keep_prob = tf.placeholder(tf.float32)
-    h_fc1_drop = tf.nn.dropout(h_fc1, keep_prob)

...

-    y_conv = tf.matmul(h_fc1_drop, W_fc2) + b_fc2
-  return y_conv, keep_prob
+    y_conv = tf.matmul(h_fc1, W_fc2) + b_fc2
+  return y_conv
  
...

-  # Import data
-  mnist = input_data.read_data_sets(FLAGS.data_dir, one_hot=True)

...

-  # Define loss and optimizer
-  y_ = tf.placeholder(tf.float32, [None, 10])

...

   # Build the graph for the deep net
-  y_conv, keep_prob = deepnn(x)
+  # No longer need keep_prob since removing dropout layers.
+  y_conv = deepnn(x)

...

-  with tf.name_scope('loss'):
-    cross_entropy = tf.nn.softmax_cross_entropy_with_logits(labels=y_,
-                                                            logits=y_conv)
-  cross_entropy = tf.reduce_mean(cross_entropy)
-  
-  with tf.name_scope('adam_optimizer'):
-    train_step = tf.train.AdamOptimizer(1e-4).minimize(cross_entropy)
-  
-  with tf.name_scope('accuracy'):
-    correct_prediction = tf.equal(tf.argmax(y_conv, 1), tf.argmax(y_, 1))
-    correct_prediction = tf.cast(correct_prediction, tf.float32)
-  accuracy = tf.reduce_mean(correct_prediction)
-
-  graph_location = tempfile.mkdtemp()
-  print('Saving graph to: %s' % graph_location)
-  train_writer = tf.summary.FileWriter(graph_location)
-  train_writer.add_graph(tf.get_default_graph())
+  
+   saver = tf.train.Saver(tf.global_variables())
+
   with tf.Session() as sess:
       sess.run(tf.global_variables_initializer())
+      sess.run(tf.local_variables_initializer())
+      # read the previously saved network.
+      saver.restore(sess, '.' + '/mnist_model')
+      # save the version of the network ready that can be compiled for NCS
+      saver.save(sess, '.' + '/mnist_inference')

-  for i in range(5000):
-    batch = mnist.train.next_batch(50)
-    if i % 100 == 0:
-      train_accuracy = accuracy.eval(feed_dict={
-          x: batch[0], y_: batch[1], keep_prob: 1.0})
-      print('step %d, training accuracy %g' % (i, train_accuracy))
-    train_step.run(feed_dict={x: batch[0], y_: batch[1], keep_prob: 0.5})
-    
-    print('test accuracy %g' % accuracy.eval(feed_dict={
-        x: mnist.test.images, y_: mnist.test.labels, keep_prob: 1.0}))
-    save_path = saver.save(sess, "./model.ckpt")
```

- Make sure the last node is named.  As with the first node, this is not strictly required but you need to know the name to compile it.  This is the change to make to mnist_deep.py in order to have a last softmax layer with a node name of "output":

```python
   # Build the graph for the deep net
-  y_conv, keep_prob = deepnn(x)
+  y_conv = deepnn(x)
+  output = tf.nn.softmax(y_conv, name='output')
```

- Run the inference version of the code to save a session that is suitable for compiling via the ncsdk compiler.  This will only take a second since its not actually training the network, just resaving it in an NCS-friendly way.  After you run, if successfull, the following files will be created.
  - mnist_inference.index
  - mnist_inference.data-00000-of-00001
  - mnist_inference.meta

- Compile the final saved network with the following command and if it all works you should see the mnist_inference.graph file created in the current directory.  Note you pass in only the weights file prefix "mnist_inference" for the -w option for a TensorFlow network on the compile command line.  The full command is below.

```bash
mvNCCompile mnist_inference.meta -w mnist_inference -s 12 -in input -on output -o mnist_inference.graph
```

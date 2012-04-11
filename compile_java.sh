#! /bin/bash
javac -cp lib/java/suasync-1.1.0.jar src/java/com/stumbleupon/async/GenericCallback.java &&
jar cvf lib/java/suasync-generic-callback.jar -C src/java com/stumbleupon/async/GenericCallback.class &&
rm src/java/com/stumbleupon/async/GenericCallback.class

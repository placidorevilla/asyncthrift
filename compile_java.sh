#! /bin/bash
javac java/com/stumbleupon/async/GenericCallback.java &&
jar cvf lib/java/suasync-generic-callback.jar -C java com/stumbleupon/async/GenericCallback.class &&
rm java/com/stumbleupon/async/GenericCallback.class

#! /bin/bash
mkdir -p build/java
javac -cp build/lib/java/suasync-*.jar -d build/java src/java/com/stumbleupon/async/GenericCallback.java &&
jar cvf build/lib/java/suasync-generic-callback.jar -C build/java com/stumbleupon/async/GenericCallback.class

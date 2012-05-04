#! /bin/bash

for i in $(pwd)/build/lib/asyncthrift-1.0.0/java/*.jar; do
	CLASSPATH="$CLASSPATH:$i"
done
export JAVA_HOME=/usr/lib/jvm/java-6-sun
export LD_LIBRARY_PATH=$JAVA_HOME/jre/lib/amd64/server/
export CLASSPATH

exec build/bin/asyncthrift --config conf "$@"

#! /bin/bash

for i in $(pwd)/lib/java/*.jar; do
	CLASSPATH="$CLASSPATH:$i"
done
export JAVA_HOME=/usr/lib/jvm/java-6-sun
export LD_LIBRARY_PATH=$JAVA_HOME/jre/lib/amd64/server/
export CLASSPATH

exec build/thrift "$@"

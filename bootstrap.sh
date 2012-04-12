#! /bin/bash

#qmake

DEFAULT_ORIGIN='http://opentsdb.googlecode.com/files'

PACKAGES=(asynchbase-1.2.0.jar
log4j-over-slf4j-1.6.4.jar
netty-3.3.1.Final.jar
slf4j-api-1.6.4.jar
http://repo2.maven.org/maven2/org/slf4j/slf4j-simple/1.6.4/slf4j-simple-1.6.4.jar
suasync-1.2.0.jar
zookeeper-3.3.4.jar)

mkdir -p build/lib/java

for p in ${PACKAGES[@]}; do
	URL="$DEFAULT_ORIGIN/$p"
	[ `echo $p | cut -c -7` = 'http://' ] && URL="$p"
	echo "Downloading $URL"
	curl -sS "$URL" -o "./build/lib/java/`basename $p`"
done


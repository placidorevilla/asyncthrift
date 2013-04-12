Asyncthrift
===========

Asyncthrift is a partial implementation of the HBase thrift gateway, using the same interface with vastly enhanced capabilities.

Features
--------

* Completely asynchronous implementation (improved performance and scalability)
* Memory efficient (bounded buffers)
* Disk spooling of HBase transactions (improved reliability of the whole system)
* Separated processes for logging and forwarding transactions

Installation
------------

Asyncthrift is buildable as a debian package on Debian squeeze and Ubuntu precise.

Compilation dependencies are correctly defined on ```debian/control``` and include:

* Qt >= 4.6
* Sun Java6 (on Ubuntu use OAB-Java: https://github.com/flexiondotorg/oab-java6/)
* libdaemon
* Log4CXX
* Boost >= 1.40

You are going to need the thrift-compiler package (>= 0.8.0), which you can build yourself from the thrift sources.

To build as a debian package:

	dpkg-buildpackage -b

Usage
-----

There are init scripts for both the logger and the forwarder processes, which you can start with

	/etc/init.d/asyncthrift-logger start
	/etc/init.d/asyncthrift-forwarder start

There are several configuration files in /etc/asyncthrift and /etc/default/asyncthrift with which you can tune the logger and forwarder behaviour.

Development
-----------

You can build the code without the debian infrastructure by installing all the dependencies and doing:

	./configure && cd build && make

The configure script has some options which you can see by passing the -h parameter

Build: autoconf , ./configure, make 

Xama's CPP linux generics library
	* Exception - std::exception-based class
	* Logger - singleton logger 

System:
	* different libc/syscalls wrappers with Exception/Logger
	* FileLock - fcntl's flock file locker

Netlink:
	* Listener - netlink's event listener

IPC:
	* Mutex  - futex based mutex
	* BufferManager/Buffer - factory/product for shared-memory-based and IPC::Mutex syncronized buffers


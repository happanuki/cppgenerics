#pragma once

#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>
#include <string>
#include <limits>

#include "Exception.h"

namespace System {

	 std::unique_ptr<char[]> getBufferRAI(size_t size, bool isZero = false) throw (std::exception&);
	 std::unique_ptr<char[]> getBufferRAI_nt(size_t size, bool isZero = false) noexcept;


	 int open(const char *pathname, int flags, mode_t mode = 0) throw (std::exception&);
	 void close(int fd) throw (std::exception&);

	 std::unique_ptr<int,std::function<void(int*)>> openAutoClose(const char *pathname, int flags, mode_t mode = 0) throw (std::exception&);

	 size_t read(int fd, void *buf, size_t count) throw (std::exception&);
	 size_t write(int fd, const void *buf, size_t count) throw (std::exception&);
	 off_t lseek(int fd, off_t offset, int whence) throw (std::exception&);
	 off_t pread(int fd, void *buf, size_t count, off_t offset) throw (std::exception&);

	/*
	 * readExact(), writeExact() returns number of read() and write() iterations
	 * noexcept versions returns -1, if error
	 */
	 size_t readExact(int fd, void *buf, size_t count) throw (std::exception&);
	 size_t writeExact(int fd, const void *buf, size_t count) throw (std::exception&);

	 ssize_t readExact_nt(int fd, void *buf, size_t count, size_t* bytesSuccRead = nullptr) noexcept;
	 ssize_t writeExact_nt(int fd, const void *buf, size_t count, size_t* bytesSuccWritten = nullptr) noexcept;


	 struct stat getFileStat(const std::string& path) throw (std::exception&);
	 bool isFileExist(const std::string& fileName) noexcept;
	 bool isDirExist(const std::string& dirName) noexcept;

	 void mkdir_p(std::string pathName) throw (std::exception&); //aka mkdir -p

	 std::string dirname(const std::string& path);
	 std::string basename(const std::string& path);


	 void initModule (const char* path, const char *param_values) throw (std::exception&);
	 void deleteModule (std::string name, int flags = 0) throw (std::exception&); //std::string name should be by value
	 int deleteModule_nt (std::string name, int flags = 0) noexcept;

	 int getRandom(int min = 0 , int max = std::numeric_limits<int>::max() ) noexcept;

	 uint64_t getCurUsecs() throw (std::exception&);
	 void nanoSleep(uint64_t nanosecs) throw (std::exception&);
	
	 time_t time(time_t *time) throw (std::exception&);
	 std::string timeStr(time_t* timetOtherPtr) throw (std::exception&);
	
	 int socket(int domain, int type, int protocol) throw (std::exception&);
	 void bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) throw (std::exception&);
	 int recvmsg(int sockfd, struct msghdr *msg, int flags) throw (std::exception&);

	 pid_t gettid() noexcept;

	 int sched_getscheduler(pid_t pid) throw (std::exception&);
     void sched_setscheduler(pid_t pid, int policy, const struct sched_param *param) throw (std::exception&) ;

	 int getpriority( int which, id_t who) throw (std::exception&);
	 void setpriority( int which, id_t who, int prio) throw (std::exception&);

	 void resched() noexcept;

}

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <linux/sched.h>
#include <sys/mount.h>


#include <random>
#include <ctime>

#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"

std::unique_ptr<char[]> System::getBufferRAI(size_t size, bool isZero) throw (std::exception&)
{
	std::unique_ptr<char[]> out( new char[size] );
	if (out.get() == nullptr) {
		THROW_SYS_EXCEPTION("new char["+std::to_string(size)+"] failed");
	}

	if (isZero) {
		memset(out.get(),0,size);
	}

	return out;
}

std::unique_ptr<char[]> System::getBufferRAI_nt(size_t size, bool isZero) noexcept
{
	std::unique_ptr<char[]> out( new char[size] );

	if (isZero && out.get() != nullptr) {
		memset(out.get(),0,size);
	}

	return out;
}

int System::open(const char *pathname, int flags, mode_t mode) throw (std::exception&)
{
	auto ret = ::open(pathname,flags,mode);
	if (ret < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: ")+pathname+" "+std::to_string(flags)+" "+std::to_string(mode));
	}
	return ret;
}

void System::close(int fd) throw (std::exception&)
{
	auto ret = ::close(fd);
	if (ret < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: ")+std::to_string(fd));
	}
}

std::unique_ptr<int,std::function<void(int*)>> System::openAutoClose(const char *pathname, int flags, mode_t mode) throw (std::exception&)
{
	return std::unique_ptr<int,std::function<void(int*)>> ( new int( open(pathname,flags,mode)), [](int* in) {
		(void)::close(*in);
		delete in;
	});
}

size_t System::read(int fd, void *buf, size_t count) throw (std::exception&)
{
	auto out = ::read(fd,buf,count);
	if (out < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: ") + std::to_string(fd) + " " + std::to_string((ulong)buf) + " " + std::to_string(count));
	}
	return out;
}


size_t System::write(int fd, const void *buf, size_t count) throw (std::exception&)
{
	auto out = ::write(fd,buf,count);
	if (out < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: ") + std::to_string(fd) + " " + std::to_string((ulong)buf) + " " + std::to_string(count));
	}
	return out;
}

size_t System::readExact(int fd, void *buf, size_t count) throw (std::exception&)
{
	decltype(count) soFar = 0;
	size_t iters = 0;

	while( soFar < count) {
		auto rd = read(fd, (uint8_t*)buf + soFar,count - soFar);
		if (rd == 0) /* EOF */ {
			THROW_LOGIC_EXCEPTION("EOF reached, count: " + std::to_string(count) + " soFar: " + std::to_string(soFar));
		}

		soFar += rd;
		++iters;
	}

	return iters;
}

size_t System::writeExact(int fd, const void *buf, size_t count) throw (std::exception&)
{
	decltype(count) soFar = 0;
	size_t iters = 0;

	while( soFar < count) {
		soFar += write(fd, (uint8_t*)buf + soFar,count - soFar);
		++iters;
	}

	return iters;
}

ssize_t System::readExact_nt(int fd, void *buf, size_t count, size_t* bytesSuccRead) noexcept
{
	decltype(count) soFar = 0;
	size_t iters = 0;

	while( soFar < count) {
		auto rd = ::read(fd, (uint8_t*)buf + soFar,count - soFar);
		if (rd <= 0) {
			return rd;
		}

		soFar += rd;

		if (bytesSuccRead != nullptr) {
			*bytesSuccRead = soFar;
		}
		++iters;
	}

	return iters;
}

ssize_t System::writeExact_nt(int fd, const void *buf, size_t count, size_t* bytesSuccWritten) noexcept
{
	decltype(count) soFar = 0;
	size_t iters = 0;

	while( soFar < count) {
		auto wr = ::write(fd, (uint8_t*)buf + soFar,count - soFar);
		if (wr < 0) {
			return wr;
		}

		soFar += wr;

		if (bytesSuccWritten != nullptr) {
			*bytesSuccWritten = soFar;
		}
		++iters;
	}

	return iters;
}


off_t System::lseek(int fd, off_t offset, int whence) throw (std::exception&)
{
	auto out = ::lseek(fd,offset,whence);
	if (out < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: fd: ") + std::to_string(fd)+"  offset: " + std::to_string(offset) + "  whence: " + std::to_string(whence));
	}
	return out;
}


struct stat System::getFileStat(const std::string& path) throw (std::exception&)
{
    struct stat fStats;
    int err;
    if ((err=::stat(path.c_str(),&fStats))) {
    	THROW_SYS_EXCEPTION("stat failed");
    }
    return fStats;

}


off_t System::pread(int fd, void *buf, size_t count, off_t offset) throw (std::exception&)
{
	auto out = ::pread(fd,buf,count, offset);
	if (out < 0) {
		THROW_SYS_EXCEPTION(std::string("ARGS: ") + std::to_string(fd) + " " + std::to_string((ulong)buf) + " " + std::to_string(count));
	}
	return out;
}


bool System::isFileExist(const std::string& fileName) noexcept
{
        return !access(fileName.c_str(),F_OK);
}


bool System::isDirExist(const std::string& dirName) noexcept
{
	bool res = false;

	try {
		res = (bool)( S_ISDIR( getFileStat(dirName).st_mode));
    }
	catch(...) {}

	return res;
}


void System::mkdir_p(std::string pathName) throw (std::exception&)
{
	if (System::isDirExist(pathName)) {
		return;
	}
	else {
		std::string pathCopy (pathName.c_str());
		auto parentPath = std::string( dirname_(const_cast<char*>(pathCopy.c_str())));
		mkdir_p(parentPath);

		auto ret = mkdir(pathName.c_str(), 0755);
		if (ret) {
			THROW_SYS_EXCEPTION("mkdir " << pathName << " failed");
		}
	}
}


std::string System::dirname_(const std::string& path)
{
	return ::dirname(const_cast<char*>(path.c_str()));
}


std::string System::basename_(const std::string& path)
{
	return ::basename(const_cast<char*>(path.c_str()));
}


static inline int init_module(void *module_image, ulong len, const char *param_values) noexcept
{
	return syscall(__NR_init_module, module_image, len, param_values);
}

static inline int delete_module(const char* name, int flags) noexcept
{
	return syscall(__NR_delete_module, name, flags);
}

void System::initModule (const char* path, const char *param_values) throw (std::exception&)
{
	auto fd = openAutoClose(path,O_RDONLY);

	struct stat fd_stat = getFileStat(path);

	auto buf = getBufferRAI(fd_stat.st_size);

	readExact(*fd,buf.get(),fd_stat.st_size);

	::close(*fd);

	if ( init_module( buf.get(), fd_stat.st_size, param_values) != 0) {
		THROW_SYS_EXCEPTION("init "+std::string(path)+"failed");
	}
}

void System::deleteModule (std::string name, int flags) throw (std::exception&)
{
	for (auto& it: name) {
		if (it == '-') {
			it ='_';
		}
	}

	auto out = delete_module(name.c_str(),flags);
	if (out != 0 ) {
		THROW_SYS_EXCEPTION("delete "+std::string(name)+" failed");
	}
}

int System::deleteModule_nt (std::string name, int flags) noexcept
{
	for (auto& it: name) {
		if (it == '-') {
			it ='_';
		}
	}
	return delete_module(name.c_str(),flags);
}

int System::getRandom(int min, int max) noexcept
{
	static std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

uint64_t System::getCurUsecs() throw (std::exception&)
{
	timeval time;

	auto out = gettimeofday(&time,NULL);
	if ( out == -1) {
		THROW_SYS_EXCEPTION("gettimeofday() failed");
	}

	return time.tv_sec * 1E6 + time.tv_usec;
}

time_t System::time(time_t* time) throw (std::exception&)
{
        auto out = ::time(time);
        if ( out == -1) {
                THROW_SYS_EXCEPTION("time failed");
        }
        return out;
}


std::string System::timeStr(time_t* timetOtherPtr) throw (std::exception&)
{
        time_t _tm = time(timetOtherPtr );
        struct tm * curTime = localtime ( &_tm );
        std::string curTimeS( asctime(curTime));

        //remove trailing \n
        curTimeS.erase(curTimeS.end() - 1, curTimeS.end());
        return curTimeS;
}


void System::nanoSleep(uint64_t nanosecs) throw (std::exception&)
{
	long secs = nanosecs / 1E9;
	long nsecs =  nanosecs % (uint)1E9;

	timespec ts = { secs, nsecs };

	auto out = ::nanosleep(&ts,nullptr);
	if (out == -1) {
		THROW_SYS_EXCEPTION("nanosleep failed");
	}
}

int System::socket(int domain, int type, int protocol) throw (std::exception&)
{
	auto fd = ::socket(domain,type,protocol);
	if (fd < 0) {
		THROW_SYS_EXCEPTION("socket() failed");
	}
	return fd;
}

void System::bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) throw (std::exception&)
{
	auto out = ::bind(sockfd,addr,addrlen);
	if (out < 0 ) {
		THROW_SYS_EXCEPTION("bind() failed");
	}
}

int System::recvmsg(int sockfd, struct msghdr *msg, int flags) throw (std::exception&)
{
	auto ret = ::recvmsg(sockfd, msg, flags);
	if ( ret < 0) {
		THROW_SYS_EXCEPTION("recvmsg failed");
	}

	return ret;
}
//
//
//bool System::recvmsgPred(
//						int sockfd,
//						struct msghdr *msg,
//						std::function< bool(void*)> pred,
//						void* predArgs					) throw (std::exception&)
//{
//	auto soFar = 0;
//
//	while ( soFar != sizeof(msg) || ! pred(predArgs) ) {
//		auto ret = ::recvmsg(sockfd, msg, MSG_DONTWAIT);
//		if (ret == 0) {
//			THROW_LOGIC_EXCEPTION("con peer shutdown");
//		}
//		else if (ret < -1) {
//			if ( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
//				usleep(100000);
//				continue;
//			}
//			else {
//				THROW_SYS_EXCEPTION("recvmsg failed");
//			}
//		}
//		else {
//			soFar+=ret;
//		}
//	}
//
//	return soFar == sizeof(msg);
//}


pid_t System::gettid() noexcept
{
	return syscall(__NR_gettid);
}


int System::sched_getscheduler(pid_t pid) throw (std::exception&)
{
	auto ret = ::sched_getscheduler(pid);
	if ( ret < 0 ) {
		THROW_SYS_EXCEPTION("sched_getscheduler() failed");
	}
	return ret;
}


void System::sched_setscheduler(pid_t pid, int policy, const struct sched_param *param) throw (std::exception&)
{
	auto ret = ::sched_setscheduler(pid, policy, param);
	if ( ret < 0 ) {
		THROW_SYS_EXCEPTION("sched_setscheduler() failed");
	}
}


int System::getpriority( int which, id_t who) throw (std::exception&)
{
	errno = 0;

	auto ret = ::getpriority(which,who);
	if (errno) {
		THROW_SYS_EXCEPTION("getpriority() failed");
	}

	return ret;
}

void System::setpriority( int which, id_t who, int prio ) throw (std::exception&)
{
	auto ret = ::setpriority(which,who,prio);
	if ( ret < 0 ) {
		THROW_SYS_EXCEPTION("setpriority() failed");
	}
}


void System::resched() noexcept
{
	try {
		auto prevSched = System::sched_getscheduler( 0 ); // 0 for current thread
		auto prevPrio = System::getpriority(PRIO_PROCESS, 0 );

		sched_param p = { 0 };

		System::sched_setscheduler( 0, SCHED_IDLE, &p);

		sched_yield();

		p = { prevPrio };

		System::sched_setscheduler(0, prevSched, &p);

	}
	catch (std::exception& e) {
		WARNSTDOUTT(" scheduling by raw sched_yield");
		WARNSTDOUTT(e.what());
		sched_yield();
	}
}

void System::mount(	const std::string& source,
					const std::string& target,
					const std::string& fs_type,
					unsigned long mountflags,
					const void *data) throw (std::exception&)
{
    auto ret = ::mount(	source.c_str(),
    					target.c_str(),
						fs_type.c_str(),
						mountflags,
						data);
    if (! ret) {
    	THROW_SYS_EXCEPTION("mount() "<< source << " [ " << target << " ] failed");
    }

}


winsize System::getTerminalSize() throw (std::exception&)
{
	struct winsize w;

	auto ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	if (! ret ) {
		THROW_SYS_EXCEPTION("ioctl TIOCGWINSZ failed, unknown terminal size");
	}

	return w;
}



#include "linux.h"
#include "posix-internal.h"

#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <chrono>

#define to_string(arg) boost::lexical_cast<std::string>(arg)
namespace {
	std::string epoll_message(const char* operation, int efd, int ofd) {
		return std::string("Error while fd ") + operation + " " + to_string(ofd) + " to epoll " + to_string(efd) + ": " + strerror(errno);
	}
}

namespace linux {
	epoll::epoll(std::function<void (event)>&& _callback) : efd(epoll_create1(EPOLL_CLOEXEC)), callback(_callback) {
	}

	uint32_t epoll::reader = EPOLLIN;
	uint32_t epoll::writer = EPOLLOUT;

	result::maybe epoll::add(int fd, event event_) {
		struct epoll_event events;
		events.events = event_.events;
		events.data = *reinterpret_cast<epoll_data*>(&event_.feedback);
		if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &events) == -1) {
			bool throws = (errno == EEXIST || errno == EPERM);
			return result::maybe(errno, epoll_message("adding", efd, fd), throws);
		}
		return result::maybe(0, "", false);
	}

	result::maybe epoll::modify(int fd, event event_) {
		struct epoll_event events;
		events.events = event_.events;
		events.data = *reinterpret_cast<epoll_data*>(&event_.feedback);
		if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &events) == -1)
			return result::maybe(errno, epoll_message("modifying", efd, fd), errno != ENOENT);
		return result::maybe(0, "", false);
	}

	result::maybe epoll::remove(int fd) {
		if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) == -1)
			return result::maybe(errno, epoll_message("removing", efd, fd), errno != ENOENT);
		return result::maybe(0, "", false);
	}

	static int my_wait(int efd, const std::function<void (epoll::event)>& callback, int max, int timeout, const sigset_t* sigset) {
		std::vector<epoll_event> buffer(max);
		int retval = epoll_pwait(efd, &buffer[0], max, timeout, sigset);
		for (auto i : buffer) {
			epoll::event tmp(i.events, *reinterpret_cast<const epoll::event::data*>(&i.data));
			callback(tmp);
		}
		return retval;
	}
	int epoll::wait(int max) const {
		return my_wait(efd, callback, max, -1, NULL);
	}
	int epoll::wait(int max, const std::chrono::milliseconds& timeout) const {
		return my_wait(efd, callback, max, timeout.count(), NULL);
	}
	int epoll::wait(int max, const signal::set& sigset) const {
		return my_wait(efd, callback, max, -1, &sigset.get_ptr()->raw);
	}
	int epoll::wait(int max, const std::chrono::milliseconds& timeout, const signal::set& sigset) const {
		return my_wait(efd, callback, max, timeout.count(), &sigset.get_ptr()->raw);
	}

	namespace {
		int signalfd_for(int fd, int signo, bool nonblock) {
			sigset_t tmp;
			sigemptyset(&tmp);
			sigaddset(&tmp, signo);
			int ret = ::signalfd(fd, &tmp, SFD_CLOEXEC | (nonblock ? SFD_NONBLOCK : 0 ));
			if (ret < 0) {

			}
			return ret;
		}
		int signalfd_for(int fd, const signal::set& sigset, bool nonblock) {
			int ret = ::signalfd(fd, &sigset.get_ptr()->raw, SFD_CLOEXEC | (nonblock ? SFD_NONBLOCK : 0 ));
			if (ret < 0) {

			}
			return ret;
		}
	}
	signalfd::signalfd(int signo, bool nonblock) : fd(signalfd_for(-1, signo, nonblock)) {
	}
	signalfd::signalfd(const signal::set& set, bool nonblock) : fd(signalfd_for(-1, set, nonblock)) {
	}
	void signalfd::set_mask(int signo) {
		signalfd_for(*this, signo, false);
	}
	void signalfd::set_mask(const signal::set& sigset) {
		signalfd_for(*this, sigset, false);
	}

	int signalfd::receive() const {
		return 0;
	}
}

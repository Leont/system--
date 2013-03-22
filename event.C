#include "event.h"

namespace event {
	namespace handler {
		linux::epoll loop(handler::descriptor::main_handler);
		std::map<int, descriptor::collection> descriptor::callbacks;
		std::map<int, signal::entry> signal::callbacks;
	}
	handler::descriptor add_reader(int fd, handler::descriptor::callback callback) {
		return handler::descriptor(fd, epoll::reader, callback);
	}

	handler::signal add_signal(int signo, handler::signal::callback callback) {
		signalfd fd(signo, true);
		return handler::signal(signo, fd, callback);
	}

	int wait(int max) {
		return handler::loop.wait(max);
	}
}

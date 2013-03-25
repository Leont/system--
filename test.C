#include "posix.h"
#include "event.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

using namespace posix;
using namespace event;

int main() {
	signal::set foo;
	signal::set bar = foo;

	try {
		handler::descriptor tmp = add_reader(0, [&](int) { std::cout << "Got it!" << std::endl; });
		event::wait(1);
	}
	catch(const posix::system_exception& ex) {
		std::cout << "Got exception: " << ex.what() << std::endl;
		throw;
	}
	return 0;
}

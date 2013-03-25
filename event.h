#include <vector>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>

namespace event {
	namespace handler {
		class descriptor {
			public:
			typedef std::function<void (int)> callback;
			class impl;
			private:
			std::shared_ptr<impl> real;
			public:
			descriptor(int, uint32_t, callback&&);
		};

		class signal {
			public:
			typedef std::function<void (int)> callback;
			private:
			class impl;
			std::shared_ptr<impl> real;
			public:
			signal(int signo, callback&& _callback);
		};
		class timer {
		};
	}
	
	extern handler::descriptor add_reader(int fd, handler::descriptor::callback&& callback);
	extern handler::signal add_signal(int signo, handler::signal::callback&& callback);

	extern int wait(int max);
}

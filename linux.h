#undef linux

#include "posix.h"
#include <chrono>
#include <functional>

namespace linux {
	using namespace posix;
	class epoll {
		descriptor efd;
		public:
		struct event {
			union data {
			   void*    ptr;
			   int      fd;
			   uint32_t u32;
			   uint64_t u64;
			};
			uint32_t events;
			data feedback;
			event(uint32_t _events, data _feedback) : events(_events), feedback(_feedback) {
			}
			event(uint32_t _events, void* _feedback) : events(_events), feedback{_feedback} {
			}
			event(uint32_t _events, int _feedback) : events(_events), feedback{reinterpret_cast<void*>(_feedback)} {
			}
		};
		static uint32_t reader;
		static uint32_t writer;

		typedef std::function<void (event)> callback_type;
		private:
		std::function<void (event)> callback;
		public:
		explicit epoll(std::function<void (event)>&&);
		result::maybe add(int fd, event event);
		result::maybe modify(int fd, event event);
		result::maybe remove(int fd);

		int wait(int max) const;
		int wait(int max, const std::chrono::milliseconds&) const;
		int wait(int max, const signal::set&) const;
		int wait(int max, const std::chrono::milliseconds&, const signal::set&) const;
	};

	class futex;

	class signalfd {
		descriptor fd;
		public:
		class weak {
			descriptor::weak fd;
			friend class signalfd;
			public:
			weak(const signalfd& strong) : fd(strong) {
			}
		};
		friend class weak;
		explicit signalfd(int, bool);
		explicit signalfd(const signal::set&, bool);
		signalfd(const weak& other) : fd(other.fd) {
		}
		void set_mask(int signo);
		void set_mask(const signal::set&);
		int receive() const;
		operator descriptor() const {
			return fd;
		}
		operator int() const {
			return fd;
		}
	};
	class timerfd {
		public:
		timerfd();
	};
};


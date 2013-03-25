#include "linux.h"
#include "event.h"

namespace event {
	using namespace linux;

	namespace handler {
		class loop;
		namespace {
			class collection {
				struct entry {
					uint32_t events;
					descriptor::callback callback;
					public:
					entry(uint32_t _events, descriptor::callback _callback) : events(_events), callback(_callback) {
					}
				};
				epoll& poller;
				std::map<const descriptor::impl*, entry> entries;
				int fd;
				uint32_t event_bits;
				void recalculate() {
					uint32_t new_bits;
					for (auto cb : entries) {
						auto node = cb.second;
						new_bits |= node.events;
					}
					if (new_bits != event_bits) {
						poller.modify(fd, epoll::event(new_bits, this));
						event_bits = new_bits;
					}
				}
				collection(const collection&) = delete;
				public:
				collection(epoll& _poller, int _fd, uint32_t _event_bits) : poller(_poller), entries(), fd(_fd), event_bits(_event_bits) {
					poller.add(fd, epoll::event(event_bits, this));
				}
				void add(const descriptor::impl* key, uint32_t event, descriptor::callback&& value) {
					entries.insert(std::make_pair(key, entry(event, value)));
					if ((event_bits & event) != event)
						recalculate();
				}
				void erase(const descriptor::impl* key) {
					entries.erase(key);
					if (size())
						recalculate();
				}
				void feed(epoll::event event) const {
					for (auto cb : entries) {
						auto node = cb.second;
						if (node.events & event.events)
							node.callback(fd);
					}
				}
				size_t size() const {
					return entries.size();
				}
				static void main_handler(const epoll::event& event) {
					static_cast<collection*>(event.feedback.ptr)->feed(event);
				}
				~collection() {
					if (fd >= 0)
						poller.remove(fd);
				}
			};
		}
		class loop {
			epoll raw;
			boost::ptr_map<int, collection> callbacks;
			public:
			loop(std::function<void (epoll::event)>&& first_callback) : raw(std::forward<std::function<void (epoll::event)>>(first_callback)), callbacks() {
			}
			void add(const descriptor::impl* desc, int fd, uint32_t event, descriptor::callback&& _callback) {
				auto iter = callbacks.find(fd);
				if (iter == callbacks.end())
					iter = callbacks.insert(fd, new collection(raw, fd, event)).first;
				iter->second->add(desc, event, std::forward<descriptor::callback>(_callback));
			}
			void remove(const descriptor::impl* desc, int fd) {
				callbacks.at(fd).erase(desc);
				if (callbacks.at(fd).size() == 0)
					callbacks.erase(fd);
			}
			int wait(int timeout) {
				return raw.wait(timeout);
			}
		};
		//static epoll loop(collection::main_handler);

		class descriptor::impl {
			loop& poller;
			int fd;
			friend class loop;
			public:
			impl(loop& _poller, int _fd, uint32_t event, callback&& _callback) : poller(_poller), fd(_fd) {
				poller.add(this, fd, event, std::forward<callback>(_callback));
			}
			~impl() {
				poller.remove(this, fd);
			}
		};
		loop poller(collection::main_handler);

		descriptor::descriptor(int fd, uint32_t event, callback&& _callback) : real(new impl(poller, fd, event, std::forward<callback>(_callback))) {
		}

		class signal::impl {
			typedef std::pair<signalfd, callback> entry;
			static std::map<int, entry> callbacks;

			int signo;
			const descriptor next;
			public:
			impl(int _signo, callback&& _callback) : impl(_signo, signalfd(_signo, true), std::forward<callback>(_callback)) {
			}
			impl(int _signo, const signalfd& fd, callback&& _callback) : signo(_signo), next(fd, epoll::reader, first_callback) {
				posix::signal::block[signo] = true;
				callbacks.insert(std::make_pair(signo, std::make_pair(fd, std::forward<callback>(_callback))));
			}
			~impl() {
				callbacks.erase(signo);
				posix::signal::block[signo] = false;
			}
			static void first_callback(int fd) {
				const entry& temp = callbacks.at(fd);
				while(int signo = temp.first.receive())
					temp.second(signo);
			}
		};
		std::map<int, signal::impl::entry> signal::impl::callbacks;

		signal::signal(int signo, callback&& _callback) : real(new impl(signo, std::forward<callback>(_callback))) {
		}
	}

	handler::descriptor add_reader(int fd, handler::descriptor::callback&& callback) {
		return handler::descriptor(fd, epoll::reader, std::forward<handler::descriptor::callback>(callback));
	}

	handler::signal add_signal(int signo, handler::signal::callback&& _callback) {
		return handler::signal(signo, std::forward<handler::signal::callback>(_callback));
	}

	int wait(int max) {
		return handler::poller.wait(max);
	}
}

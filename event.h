#include "linux.h"

#include <vector>
#include <map>

namespace event {
	using namespace linux;

	namespace handler {
		extern epoll loop;
		class descriptor {
			public:
			typedef std::function<void (int fd, const epoll::event&)> callback;
			private:
			class impl;
			class collection {
				struct entry {
					uint32_t events;
					descriptor::callback callback;
					public:
					entry(uint32_t _events, descriptor::callback _callback) : events(_events), callback(_callback) {
					}
				};
				std::map<impl*, entry> entries;
				int fd;
				uint32_t event_bits;
				void recalculate() {
					uint32_t new_bits;
					for (auto cb : entries) {
						auto node = cb.second;
						new_bits |= node.events;
					}
					if (new_bits != event_bits) {
						loop.modify(fd, epoll::event(new_bits, this));
						event_bits = new_bits;
					}
				}
				public:
				collection(int _fd, uint32_t _event_bits) : entries(), fd(_fd), event_bits(_event_bits) {
					loop.add(fd, epoll::event(event_bits, this));
					fprintf(stderr, "Adding %d\n", fd);
				}
				collection(const collection&) = delete;
				collection(collection&& other) : entries(), fd(other.fd), event_bits(other.event_bits) {
					swap(entries, other.entries);
					other.fd = -1;
				}
				void add(impl* key, uint32_t event, callback value) {
					entries.insert(std::make_pair(key, entry(event, value)));
					if ((event_bits & event) != event)
						recalculate();
				}
				void erase(impl* key) {
					entries.erase(key);
					if (size())
						recalculate();
				}
				void feed(epoll::event event) const {
					for (auto cb : entries) {
						auto node = cb.second;
						if (node.events & event.events)
							node.callback(fd, event);
					}
				}
				size_t size() const {
					return entries.size();
				}
				~collection() {
					if (fd >= 0) {
						fprintf(stderr, "Removing %d\n", fd);
						loop.remove(fd);
					}
				}
			};
			static std::map<int, collection> callbacks;
			class impl {
				int fd;
				public:
				impl(int _fd, uint32_t event, callback callback) : fd(_fd) {
					auto iter = callbacks.find(fd);
					if (iter == callbacks.end())
						iter = callbacks.insert(std::make_pair(fd, collection(fd, event))).first;
					iter->second.add(this, event, callback);
				}
				~impl() {
					fprintf(stderr, "Destroying for %d\n", fd);
					callbacks.at(fd).erase(this);
					if (callbacks.at(fd).size() == 0)
						callbacks.erase(fd);
				}
			};
			std::shared_ptr<impl> real;
			public:
			descriptor(int fd, uint32_t event, const callback& callback) : real(new impl(fd, event, callback)) {
			}
			static void main_handler(const epoll::event& event) {
				int fd = event.feedback.fd;
				fprintf(stderr, "Feeding?\n");
				callbacks.at(fd).feed(event);
				fprintf(stderr, "Fed!\n");
			}
		};

		class signal {
			public:
			typedef std::function<void (int)> callback;
			typedef std::pair<signalfd, callback> entry;
			private:
			static std::map<int, entry> callbacks;

			int signo;
			const descriptor next;
			static void first_callback(int fd, epoll::event) {
				const entry& temp = callbacks.at(fd);
				while(int signo = temp.first.receive())
					temp.second(signo);
			}
			public:
			signal(int _signo, const signalfd& fd, callback callback) : signo(_signo), next(fd, epoll::reader, first_callback) {
				posix::signal::block[signo] = true;
				callbacks.insert(std::make_pair(signo, std::make_pair(fd, callback)));
			}
			~signal() {
				callbacks.erase(signo);
				posix::signal::block[signo] = false;
			}
		};
		class timer {
		};
	}
	
	extern handler::descriptor add_reader(int fd, handler::descriptor::callback callback);
	extern handler::signal add_signal(int signo, handler::signal::callback callback);

	extern int wait(int max);
}

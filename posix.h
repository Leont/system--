#include <string>
#include <memory>
#include <cstring>
#include "memory.h"

namespace posix {
	class system_exception {
		const std::string message;
		int errval;
		public:
		system_exception(const std::string& _message, int _errval) : message(_message), errval(_errval) {
		}
		bool operator ==(int other) const {
			return errval == other;
		}
		bool operator !=(int other) const {
			return errval != other;
		}
		const char* what() const {
			return message.c_str();
		}
	};

	namespace result {
		/*
		class io {
			int return_value;
			int errval;
			const std::string description;
			public:
			io(int _return_value, int _errval, std::string&& _description) : return_value(_return_value), errval(_errval), description(_description) {
			}
			int value() const {
				return return_value;
			}
			int error() const {
				return errval;
			}
			operator bool() const {
				return return_value >= 0;
			}
			~io() {
				if (!success)
					throw system_exception(description, errval);
			}
		};*/

		class maybe {
			int errval;
			const std::string description;
			bool should_throw;
			mutable bool checked = false;
			public:
			maybe(int _errval, std::string&& _description, bool _should_throw) : errval(_errval), description(_description), should_throw(_should_throw) {
			}
			int error() const {
				checked = true;
				return errval;
			}
			operator bool() const {
				return errval == 0;
			}
			void throw_it() const {
				if (errval != 0)
					throw system_exception(description, errval);
			}
			~maybe() {
				if (should_throw && !checked)
					throw_it();
			}
		};
	}

	class descriptor {
		std::shared_ptr<int> desc;
		static void closer(int);
		public:
		class weak {
			std::weak_ptr<int> desc;
			public:
			weak(const descriptor& outside) : desc(outside.desc) {
			}
			friend class descriptor;
		};
		friend class weak;
		explicit descriptor(int fd) : desc(new int(fd)) {
		}
		descriptor(const weak& inside) : desc(inside.desc) {
		}
		int fileno() const {
			if (desc)
				return *desc;
			else
				return -1;
		}
		operator int() const {
			return fileno();
		}
		void close(bool force = false);
	};

	namespace signal {
		struct info {
			int signo;
			public:
		};
		/*
		typedef void (*handler)(int, siginfo_t, void*);
		class _handle {
			class setter {
				int signal;
				public:
				setter(int _signal) : signal(_signal) {
				}
				setter& operator=(const handler&);
				operator handler&() const;
			};
			handler operator[](int) const;
			setter operator[](int signal) {
				return setter(signal);
			}
		};
		extern _handle handle;*/

		class set {
			class setter {
				set& sigset;
				int signal;
				friend class set;
				setter(set& _sigset, int _signal) : sigset(_sigset), signal(_signal) {
				}
				public:
				setter& operator=(bool new_value);
				operator bool() const {
					return sigset[signal];
				}
			};
			public:
			struct impl;
			typedef memory::copy_ptr<impl> ptr_type;
			ptr_type ptr;
			set();
			set(const set&);
			explicit set(ptr_type&&);
			explicit set(int);
			set(std::initializer_list<int>);
			set& operator=(const set&);
			bool operator[](int) const;
			setter operator[](int signal) {
				return setter(*this, signal);
			}

			const impl* get_ptr() const {
				return ptr;
			}
			~set();
		};

		extern bool suspend(const set&);
		extern int wait(int signo);
		extern int wait(const set&);

		class _block {
			class setter {
				int signal;
				friend class _block;
				setter(int _signal) : signal(_signal) {
				}
				public:
				setter& operator=(bool);
				operator bool() const;
			};
			public:
			_block() {
			}
			bool operator[](int signal) const;
			setter operator[](int signal) {
				return setter(signal);
			}
			operator set() const;
			_block& operator=(const set&);
		};
		extern _block block;

		class lock {
			set sigset;
			public:
			explicit lock(int signo);
			explicit lock(const set&);
			~lock() {
				block = sigset;
			}
		};

		class _pending {
			public:
			_pending() {
			}
			bool operator[](int signal) const;
			operator set() const;
		};
		extern _pending pending;
	}

	class clock {
		public:
		static clock wall;
		static clock monotonic;
	};
}


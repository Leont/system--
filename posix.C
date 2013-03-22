#include "posix.h"
#include "posix-internal.h"

#include <boost/lexical_cast.hpp>

#include <unistd.h>
#include <signal.h>

namespace posix {
	void descriptor::closer(int fd) {
		int temp_errno = errno;
		::close(fd);
		errno = temp_errno;
	}
	void descriptor::close(bool force) {
		if (!desc.unique() && !force) {
			desc = NULL;
			return;
		}
		int ret = ::close(fileno());
		desc = NULL;
		if (ret != 0) {
			std::string message = std::string("Error while closing fd ") + boost::lexical_cast<std::string>(fileno()) + ": " + strerror(errno);
			throw system_exception(message, errno);
		}
	}

	namespace signal {
		//_handle handle;

		set::set() : ptr(new set::impl()) {
		}
		set::set(ptr_type&& value) : ptr(value) {
		}
		set::set(int signo) : ptr(new set::impl()) {
			sigset_t& tmp = ptr->raw;
			sigemptyset(&tmp);
			sigaddset(&tmp, signo);
		}
		set::set(std::initializer_list<int> signals) : ptr() {
			for (auto i : signals)
				sigaddset(&ptr->raw, i);
		}
		set::set(const set& other) : ptr(other.ptr) {
		}
		set& set::operator=(const set& other) {
			ptr = other.ptr;
			return *this;
		}
		set::~set() {
		}
		bool set::operator[](int signal) const {
			return sigismember(&ptr->raw, signal);
		}
		set::setter& set::setter::operator=(bool new_value) {
			sigset_t& temp = sigset.ptr->raw;
			if (new_value)
				sigaddset(&temp, signal);
			else
				sigdelset(&temp, signal);
			return *this;
		}

		bool suspend(const set& sigset) {
			return !sigsuspend(&sigset.get_ptr()->raw);
		}
		int wait(int signo) {
			sigset_t mask;
			sigemptyset(&mask);
			sigaddset(&mask, signo);
			int ret;
			int val = sigwait(&mask, &ret);
			if (val != 0) {
				if (val == EINVAL)
					throw(system_exception(std::string("Can't wait for signal: ") + strerror(val), val));
				else if (val == EINTR)
					return 0;
			}
			return ret;
		}
		int wait(const set& sigset) {
			int ret;
			int val = sigwait(&sigset.get_ptr()->raw, &ret);
			if (val != 0) {
				if (val == EINVAL)
					throw(system_exception(std::string("Can't wait for signals: ") + strerror(val), val));
				else if (val == EINTR)
					return 0;
			}
			return ret;
		}

		bool _block::operator[](int signal) const {
			sigset_t temp;
			sigprocmask(SIG_BLOCK, NULL, &temp);
			return sigismember(&temp, signal);
		}
		_block::setter::operator bool() const {
			sigset_t temp;
			sigprocmask(SIG_BLOCK, NULL, &temp);
			return sigismember(&temp, signal);
		}
		_block::setter& _block::setter::operator=(bool new_value) {
			int operation = new_value ? SIG_BLOCK : SIG_UNBLOCK;
			sigset_t temp;
			sigemptyset(&temp);
			sigaddset(&temp, signal);
			sigprocmask(operation, &temp, NULL);
			return *this;
		}
		_block::operator set() const {
			sigset_t temp;
			sigprocmask(SIG_BLOCK, NULL, &temp);
			return set(set::ptr_type(new set::impl(temp)));
		}
		_block& _block::operator=(const set& sigset) {
			sigprocmask(SIG_SETMASK, &sigset.get_ptr()->raw, NULL);
			return *this;
		}
		_block block;

		namespace {
			set::impl* block_and_return(int signo) {
				sigset_t newsig;
				sigemptyset(&newsig);
				sigaddset(&newsig, signo);
				sigset_t oldsig;
				sigprocmask(SIG_BLOCK, &newsig, &oldsig);
				return new set::impl(oldsig);
			}
			set::impl* block_and_return(const set& newset) {
				sigset_t oldsig;
				sigprocmask(SIG_BLOCK, &newset.get_ptr()->raw, &oldsig);
				return new set::impl(oldsig);
			}
		}
		lock::lock(int signo) : sigset(block_and_return(signo)) {
		}
		lock::lock(const set& newset) : sigset(block_and_return(newset)) {
		}

		bool _pending::operator[](int signal) const {
			sigset_t temp;
			sigpending(&temp);
			return sigismember(&temp, signal);
		}
		_pending::operator set() const {
			sigset_t temp;
			sigpending(&temp);
			return set(set::ptr_type(new set::impl(temp)));
		}
		_pending pending;
	}
}

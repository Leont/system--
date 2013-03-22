namespace posix {
	namespace signal {
		struct set::impl {
			sigset_t raw;
			impl() : raw() {
			}
			impl(const impl& other) : impl(other.raw) {
			}
			impl(impl&& other) : impl(std::move(other.raw)) {
			}
			impl(const sigset_t& other) : raw(other) {
			}
		};
	}
}


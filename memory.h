#include <memory>

namespace memory {
	template<typename T> class copy_ptr {
		T* ptr;
		public:
		copy_ptr() : ptr(NULL) {
		}
		copy_ptr(const copy_ptr& other) : ptr(new T(*other.ptr)) {
		}
		copy_ptr(copy_ptr&& other) : ptr(std::move(other.ptr)) {
			other.ptr = NULL;
		}
		copy_ptr(T* newptr) : ptr(newptr) {
		}
		copy_ptr& operator=(const copy_ptr& other) {
			delete ptr;
			ptr = new T(*other.ptr);
			return *this;
		}
		copy_ptr& operator=(copy_ptr&& other) {
			delete ptr;
			ptr = std::move(other.ptr);
			other.ptr = NULL;
			return *this;
		}
		copy_ptr& operator=(T* newptr) {
			delete ptr;
			ptr = newptr;
			return *this;
		}
		~copy_ptr() {
			delete ptr;
		}

		T* get() const {
			return ptr;
		}
		operator T*() const {
			return ptr;
		}
		T* operator->() const {
			return ptr;
		}
	};
}


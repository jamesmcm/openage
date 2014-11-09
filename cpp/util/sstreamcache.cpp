// Copyright 2014-2014 the openage authors. See copying.md for legal info.

#include "sstreamcache.h"

#include <array>

namespace openage {
namespace util {

CachableOSStream::CachableOSStream()
	:
	flag{ATOMIC_FLAG_INIT} {}

CachableOSStream *CachableOSStream::acquire() {
	static std::array<CachableOSStream, 128> cache;

	// find an available cache element
	for (CachableOSStream &elem : cache) {
		if (elem.flag.test_and_set() == false) {
			return &elem;
		}
	}

	// the whole cache is currently in use... that probably means that your
	// code is broken.
	// anyways, have fun with your dynamically-allocated object.
	// TODO print warning debug message about this?
	// then again, printing a log message about the logging system being
	// overloaded might not be the most clever idea.
	return new CachableOSStream;
}

void CachableOSStream::release(CachableOSStream *cs) {
	if (cs == nullptr) {
		return;
	}

	// std::atomic_flag doesn't have a simple way of getting the truth
	// value, so we have to do a "full" test_and_set() instead.
	if (cs->flag.test_and_set()) {
		cs->stream.str("");
		cs->stream.clear();

		cs->flag.clear();
	} else {
		delete cs;
	}
}

OSStreamPtr::OSStreamPtr()
	:
	stream_ptr{CachableOSStream::acquire()} {}

OSStreamPtr::~OSStreamPtr() {
	CachableOSStream::release(this->stream_ptr);
}

OSStreamPtr::OSStreamPtr(OSStreamPtr &&other) noexcept {
	this->stream_ptr = other.stream_ptr;
	other.stream_ptr = nullptr;
}

OSStreamPtr &OSStreamPtr::operator =(OSStreamPtr &&other) noexcept {
	this->stream_ptr = other.stream_ptr;
	other.stream_ptr = nullptr;

	return *this;
}

std::string OSStreamPtr::get() const {
	return this->stream_ptr->stream.str();
}

}} // namespace openage::util

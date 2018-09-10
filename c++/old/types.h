#pragma once

template <class T>
struct maybe {
	T value;
	bool valid;
	
	maybe(T v, bool u) : value(v) {
		valid = u;
	}
	maybe(T v) : value(v) {
		valid = true;
	}
	maybe() {
		valid = false;
	}
	
	bool operator !() const {
		return !valid;
	}
	operator bool() const {
		return valid;
	};
	T& operator * () {
		return value;
	}
};

typedef struct {

} nill;

template <class K, class V>
struct noop {
  typedef bool aug_t;
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(K a, V b) { return 0;}
  static aug_t combine(aug_t a, aug_t b) { return 0;}
};


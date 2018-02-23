#ifndef SCOPE_GUARD_H_
#define SCOPE_GUARD_H_

template<typename F>
class scope_guard{
private:
	F	func;
	bool	exec;

public:
	scope_guard(F &&func_)
	 : func(std::forward<F>(func_)), exec(true) {}
	~scope_guard(void) { if(exec) func(); }
	void release(void) { exec = false; }
};

// if `f` is a l-value reference of type U, F will be deduced to be U&
// thus yielding scope_guard<U&>(U&)
// if `f` is a r-value reference of type U, F will be deduced to be U
// thus yielding scope_guard<U>(U&&)
template<typename F>
scope_guard<F> make_scope_guard(F &&f){
	return scope_guard<F>(std::forward<F>(f));
}

#endif //SCOPE_GUARD_H_

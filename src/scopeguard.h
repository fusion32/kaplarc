#ifndef SCOPEGUARD_H_
#define SCOPEGUARD_H_

template<typename F>
class ScopeGuard{
private:
	F func;
	bool active;
public:
	ScopeGuard(F &&func_)
	 : func(std::forward<F>(func_)), active(true) {}
	~ScopeGuard(void) { if(active) func(); }
	void dismiss(void) { active = false; }
};

template<typename F>
ScopeGuard<F> make_scope_guard(F &&f){
	return ScopeGuard<F>(std::forward<F>(f));
}

#endif //SCOPEGUARD_H_

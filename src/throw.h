#ifndef THROW_H_
#define THROW_H_
#include <system_error>
inline void throw_error(const std::error_code &ec){
	if(ec) throw_exception(std::system_error(ec));
}
template<typename Exception>
inline void throw_exception(Exception e){
	std::terminate();
}
#endif //THROW_H_

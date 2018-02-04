#ifndef ASIO_H_
#define ASIO_H_

#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <chrono>

#define ASIO_STANDALONE 1
#define ASIO_HAS_CSTDINT 1
#define ASIO_HAS_STD_ADDRESSOF 1
#define ASIO_HAS_STD_ARRAY 1
#define ASIO_HAS_STD_SHARED_PTR 1
#define ASIO_HAS_STD_CHRONO 1
#define ASIO_HAS_STD_TYPE_TRAITS 1
#include <asio.hpp>
#endif //ASIO_H_

#ifndef PLATFORM_H_
#define PLATFORM_H_

#if defined(_WIN32)
	#define PLATFORM_WINDOWS 1

#elif defined(__linux__)
	#define PLATFORM_LINUX 1

#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
		defined(__OpenBSD__)
	#define PLATFORM_BSD 1

#else
	#error platform not supported
#endif

#endif //PLATFORM_H_

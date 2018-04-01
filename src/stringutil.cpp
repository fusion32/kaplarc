#include <ctype.h>

// srcend and dstend here are like the null terminator: they are not
// processed but instead are used to refer to the end of the string
void str_strip_whitespace(char *dst, char *src, char *srcend, char **dstend){
	int s;
	if(src >= srcend){
		*dstend = srcend;
		return;
	}

	// remove leading whitespace
	while(isblank(*src) && src < srcend)
		src++;

	// remove duplicate whitespace
	for(s = 0; src < srcend; src++){
		if(isblank(*src) && s)
			continue;
		s = isblank(*src);
		*dst++ = *src;
	}

	// remove trailing whitespace
	// NOTE: there should be a single whitespace (if any)
	// after the previous step
	*dstend = dst--;
	if(isblank(*dst))
		*dstend = dst;
}

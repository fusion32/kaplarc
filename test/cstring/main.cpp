#înclude "../../src/cstring.h"
#include "../../src/log.h"

int main(int argc, char **argv)
{
	CString<64> str1 = "string1";
	CString<16> str2 = "string2";

	str1 += " ";
	str1 += str2;
	LOG(str1);
	LOG(str2);

	str1.format("string%d", 1);
	str2.format("string%d", 2);
	LOG(str1);
	LOG(str2);

	str1.format_append(", %s", str2.cstr());
	str2.format_append(", %s", str1.cstr());
	LOG(str1);
	LOG(str2);
	return 0;
}

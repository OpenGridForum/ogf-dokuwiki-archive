#include <stdlib.h>
#ifdef WIN32
	#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	if (argc != 2) return -1;
	#ifdef WIN32
		Sleep(atoi(argv[1])*1000);
	#endif
}




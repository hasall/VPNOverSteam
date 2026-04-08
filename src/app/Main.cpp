int ManualMain();
int AutomaticMain(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	
#ifndef _WIN32 // set path to steamclient.so library for linux
	system("export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH");
	system("export STEAM_CLIENT_PATH=./");
#endif

	if (argc > 1) {
		return AutomaticMain(argc, argv);
	}

	return ManualMain();
}
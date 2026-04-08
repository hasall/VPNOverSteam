#include <cstdlib>

int ManualMain();
int AutomaticMain(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	// If command-line arguments are provided, run in automatic mode; otherwise, run in manual mode
	if (argc > 1) {
		return AutomaticMain(argc, argv);
	}

	return ManualMain();
}
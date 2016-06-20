#include <iostream>
#include "program_manager.h"

using namespace std;

void print_usage()
{
	cout << "Usage:" << endl
		<< "preview_with_no_render.exe [sensor_name] [resolution] [frame_rate] [duration_in_second]" << endl
		<< "e.g: preview_with_no_render.exe IMX135 1920x1080 30 60" << endl;
}

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		cout << "Wrong Input! Please refer to the usage" << endl;
		print_usage();
		return 1;
	}
	CoInitializeEx(0, COINIT_MULTITHREADED);
	MFStartup(MF_VERSION);
	Program_Manager PM(argv[0], argv[1], argv[2], argv[3], argv[4]);
	PM.Start_Running();
	MFShutdown();
	CoUninitialize();
	system("PAUSE");
	return 0;
}
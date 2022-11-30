#include <iostream>
#include <Windows.h>
#include "../TerminatorT800/Common.h"
using namespace std;
int main(int argc,const char* argv[])
{
	if (argc < 2)
	{
		cout << "[*]Usage: TestProgram <Pid>" << endl;
		return 0;
	}

	HANDLE Device = CreateFile(L"\\\\.\\TERMINATORLINK", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (Device == INVALID_HANDLE_VALUE)
	{
		cout << "[-]OpenDriverFailed." << endl;
		return -1;
	}
	ProcessData data;
	data.ProcessId = atoi(argv[1]);

	DWORD returned;
	BOOL success = DeviceIoControl(Device, TERMINATOR_TERMINATEPROCESS, &data, sizeof(data), nullptr, 0, &returned, nullptr);
	cout << "[*]Send IO Control Sucessfully." << endl;
	
	CloseHandle(Device);
}



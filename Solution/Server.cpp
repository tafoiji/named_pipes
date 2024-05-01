#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <windows.h>
#include <fstream>
#include <limits>

using std::cout;
using std::cin;
using std::string;
using std::wstring;

int COUNT_CLIENTS = 5;
int COUNT_OBJECTS = 5;
wstring CLIENT_PROCESS_NAME = wstring(L"client");
wstring FILE_NAME = L"default.bin";

PROCESS_INFORMATION* pi;
HANDLE* hPipes;
HANDLE* hThreads;
HANDLE* hSemaphores;

struct employee
{
	int num; // идентификационный номер сотрудника 
	char name[10]; // имя сотрудника 
	double hours; // количество отработанных часов 
	employee(int num, string name, double hours) : num(num),  hours(hours) 
	{
		if (name.size() > 9)
		{
			strcpy(this->name, "");
			return;
		}

		strcpy(this->name, name.c_str());
	}
	employee() : num(0), hours(0), name("") {}
};

DWORD WINAPI handleQuery(void* param)
{
	int id = (int)param;
	int q_type;
	int i;
	DWORD bytesNumber;
	employee emp;

	while (true)
	{
		
		if (!ReadFile(hPipes[id], &q_type, sizeof(int), &bytesNumber, FALSE))
		{
			cout << "cannot read query type";
			break;
		}

		if (q_type == 3)
		{
			break;
		}

		if (!ReadFile(hPipes[id], &i, sizeof(int), &bytesNumber, FALSE))
		{
			cout << "cannot read query type";
			break;
		}

		//q_type == 1 => modificate
		//q_type == 2 => read

		std::fstream file(FILE_NAME, std::ios::in | std::ios::out | std::ios::binary);

		if (q_type == 1)
		{
			if (!ReadFile(hPipes[id], &emp, sizeof(employee), &bytesNumber, FALSE))
			{
				cout << "cannot read new employee";
				file.close();
				break;
			}

			if (i < 0 || i > COUNT_OBJECTS)
			{
				cout << "incorrect file index\n";
				file.close();
				continue;
			}

			for (int j = 0; j < COUNT_OBJECTS; j++)
			{
				WaitForSingleObject(hSemaphores[i], INFINITE);
			}

			std::streampos linePointer(std::streamoff(i * sizeof(employee)));
			file.seekp(linePointer);
			file.write((char*)&emp, sizeof(employee));
			for (int j = 0; j < COUNT_OBJECTS; j++)
			{
				ReleaseSemaphore(hSemaphores[i], 1, NULL);
			}
		}
		else if (q_type == 2)
		{
			if (i < 0 || i > COUNT_OBJECTS)
			{
				cout << "incorrect file index\n";
				file.close();
				continue;
			}


			WaitForSingleObject(hSemaphores[i], INFINITE);

			std::streampos linePointer(std::streamoff(i * sizeof(employee)));
			file.seekp(linePointer);
			file.read((char*)&emp, sizeof(employee));
			if (!WriteFile(hPipes[id], &emp, sizeof(employee), &bytesNumber, FALSE))
			{
				cout << "cannot send new employee to client";
				file.close();
				break;
			}

			ReleaseSemaphore(hSemaphores[i], 1, NULL);
		}

		file.close();
		
	}

	DisconnectNamedPipe(hPipes[id]);
	CloseHandle(hPipes[id]);

	return 0;
}

void closeHandles()
{
	if (pi)
	{
		for (int i = 0; i < COUNT_CLIENTS; i++)
		{
			if (pi[i].hProcess)
			{
				CloseHandle(pi[i].hProcess);
				CloseHandle(pi[i].hProcess);
			}
		}

		delete[]pi;
	}

	if (hPipes)
	{
		for (int i = 0; i < COUNT_CLIENTS; i++)
		{
			if (hPipes[i])
			{
				CloseHandle(hPipes[i]);
			}
		}

		delete[]hPipes;
	}

	if (hThreads)
	{
		for (int i = 0; i < COUNT_CLIENTS; i++)
		{
			if (hThreads[i])
			{
				CloseHandle(hThreads[i]);
			}
		}

		delete[]hThreads;
	}

	if (hSemaphores)
	{
		for (int i = 0; i < COUNT_CLIENTS; i++)
		{
			if (hSemaphores[i])
			{
				CloseHandle(hSemaphores[i]);
			}
		}

		delete[]hSemaphores;
	}
}

void terminateProcesses(DWORD exitCode)
{
	if (pi)
	{
		for (int i = 0; i < COUNT_CLIENTS; i++)
		{
			if (pi[i].hProcess)
			{
				TerminateProcess(pi[i].hProcess, exitCode);
			}
		}
	}
}

int main()
{
	cout << "Enter number of clients\n";
	int temp;
	cin >> temp;
	COUNT_CLIENTS = temp;

	cout << "Enter file name\n";
	cin.ignore(256, '\n');
	getline(std::wcin, FILE_NAME);
	std::fstream file(FILE_NAME, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	if (file.bad() || !file || file.fail())
	{
		cout << "file is not opened\n";
		return 0;
	}

	char name[10];
	int cntObjects = 0;
	cout << "Enter name and hours of employee\nTo stop press CTRL + Z\n";
	while (cin >> name)
	{
		double hours;
		cin >> hours;
		employee emp(cntObjects++, name, hours);
		file .write((char*)&emp, sizeof(employee));
	}

	COUNT_OBJECTS = cntObjects;


	file.seekg(0);
	for (int i = 0; i < COUNT_OBJECTS; i++)
	{
		employee emp;
		file.read((char*)&emp, sizeof(employee));
		cout << emp.num << ' ' << emp.name << ' ' << emp.hours << '\n';
	}

	file.close();


	pi = new PROCESS_INFORMATION[COUNT_CLIENTS];
	hPipes = new HANDLE[COUNT_CLIENTS];
	hThreads = new HANDLE[COUNT_CLIENTS];
	hSemaphores = new HANDLE[COUNT_OBJECTS];
	if (!hSemaphores || !pi || !hPipes || !hThreads)
	{
		cout << "cannot create handle array\n";
		return -1;
	}

	for (int i = 0; i < COUNT_OBJECTS; i++)
	{
		hSemaphores[i] = CreateSemaphore(NULL, COUNT_OBJECTS, COUNT_OBJECTS, (CLIENT_PROCESS_NAME + std::to_wstring(i)).c_str());
		if (!hSemaphores[i])
		{
			cout << "cannot create event for all lines\n";
			return GetLastError();
		}
	}

	for (int i = 0; i < COUNT_CLIENTS; i++)
	{
		hPipes[i] = CreateNamedPipe(
			(wstring(L"\\\\.\\pipe\\") + CLIENT_PROCESS_NAME + std::to_wstring(i)).c_str(),
			PIPE_ACCESS_DUPLEX, 
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
			COUNT_CLIENTS,
			0,
			0, 
			INFINITE, 
			NULL
		);

		if (hPipes[i] == INVALID_HANDLE_VALUE)
		{
			//CloseHandle(hPipes);
			cout << "Pipe number " << i << " is not created\n";
			closeHandles();
			cout << "Press any char to finish the server: ";
			char c;
			cin >> c;
			return GetLastError();
		}

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(NULL, (LPWSTR)(CLIENT_PROCESS_NAME + std::to_wstring(i) + wstring(L".exe ") + std::to_wstring(i)).c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi[i]))
		{
			//CloseHandle(pi[i].hProcess);
			//CloseHandle(pi[i].hThread);
			std::wcout << L"Process " << (CLIENT_PROCESS_NAME + std::to_wstring(i) + L".exe").c_str() << L" is not created\n";
			terminateProcesses(228);
			closeHandles();
			system("pause");
			return GetLastError();
		}
		
		if (!ConnectNamedPipe(hPipes[i], (LPOVERLAPPED)NULL ))
		{
			cout << "The connection failed.\n" << "The last error code: " << GetLastError() << '\n';
			closeHandles();
			system("pause");
			return GetLastError();
		}


		DWORD id;
		hThreads[i] = CreateThread(NULL, 0, handleQuery, (void*)i, NULL, &id);
		if (!hThreads[i])
		{
			closeHandles();
			cout << "cannot create thread for handling queries\n";
			system("pause");
			return GetLastError();
		}

		CloseHandle(&pi[i].hProcess);
	}

	WaitForMultipleObjects(COUNT_CLIENTS, hThreads, TRUE, INFINITE);

	closeHandles();

	cout << "FINAL FILE\n";

	std::fstream out(FILE_NAME, std::ios::binary | std::ios::in | std::ios::out);
	for (int i = 0; i < COUNT_OBJECTS; i++)
	{
		employee emp;
		out.read((char*)&emp, sizeof(employee));
		cout << emp.num << ' ' << emp.name << ' ' << emp.hours << '\n';
	}

	out.close();

	system("pause");
}
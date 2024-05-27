#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <string>

using namespace std;

int PROCESS_NUM = 0;
const wstring CLIENT_PROCESS_NAME = L"client";

HANDLE hPipe;

struct employee
{
	int num; // идентификационный номер сотрудника 
	char name[10]; // имя сотрудника 
	double hours; // количество отработанных часов 
	employee(int num, string name, double hours) : num(num), hours(hours)
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

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "error. There are not 1 argument\n";
		return -1;
	}

	PROCESS_NUM = atoi(argv[1]);
	if (!WaitNamedPipe(
		(wstring(L"\\\\.\\pipe\\") + CLIENT_PROCESS_NAME + to_wstring(PROCESS_NUM)).c_str(), // указатель на имя канала
		INFINITE // интервал ожидания
	))
	{
		cout << "Unable to connect to server\n";
		return GetLastError();
	}

	hPipe = CreateFile(
		(wstring(L"\\\\.\\pipe\\") + CLIENT_PROCESS_NAME + to_wstring(PROCESS_NUM)).c_str(), // имя канала
		GENERIC_WRITE | GENERIC_READ, // записываем в канал

		FILE_SHARE_READ | FILE_SHARE_WRITE, // разрешаем только запись в канал
		(LPSECURITY_ATTRIBUTES)NULL, // защита по умолчанию
		OPEN_EXISTING, // открываем существующий канал
		0, // атрибуты по умолчанию
		(HANDLE)NULL // дополнительных атрибутов нет
	);

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		cerr << "Connection with the named pipe failed." << endl
			<< "The last error code: " << GetLastError() << endl;
		cout << "Press any char to finish the client: ";

		char c;
		cin >> c;
		return 0;
	}

	while (true)
	{
		cout << "ENTER QUERY:\n1: Modificate line\n2: Read line\n3: Exit\n";
		int q;
		cin >> q;
		DWORD numberBytes;
		if (!WriteFile(hPipe, &q, sizeof(int), &numberBytes, NULL))
		{
			cout << "ERROR! Cannot send new query type to server\n";
			continue;
		}

		if (q == 3)
		{
			break;
		}
		else
		{
			int id;
			cout << "Enter employee's id\n";
			cin >> id;

			if (!WriteFile(hPipe, &id, sizeof(int), &numberBytes, NULL))
			{
				cout << "ERROR! Cannot send new query type to server\n";
				continue;
			}

			bool isCorrectEmp = false;
			if (!ReadFile(hPipe, &isCorrectEmp, sizeof(bool), &numberBytes, NULL))
			{
				cout << "ERROR! Cannot send new query type to server\n";
				continue;
			}

			if (!isCorrectEmp) 
			{
				cout << "Incorrect employee id\n";
				continue;
			}
			else
			{
				cout << "Correct employee id\n";
			}

			if (q == 1)
			{
				employee emp;
				if (!ReadFile(hPipe, &emp, sizeof(employee), &numberBytes, NULL))
				{
					cout << "ERROR! Cannot get employee from server\n";
					continue;
				}
				else
				{
					cout << emp.num << ' ' << emp.name << ' ' << emp.hours << "\n";
				}

				cout << "enter new name\n";
				char name[10];
				cin >> name;

				cout << "enter new hours count\n";
				double hours;
				cin >> hours;
				employee empNew(id, name, hours);
				
				if (!WriteFile(hPipe, &empNew, sizeof(employee), &numberBytes, NULL))
				{
					cout << "ERROR! Cannot send new employee to server\n";
					continue;
				}
				else
				{
					DWORD data = 1;
					ReadFile(hPipe, &data, sizeof(DWORD), &numberBytes, NULL);
					cout << "Data is modificated\n";
					cout << "enter any char to continue\n";
					char c;
					cin >> c;
					WriteFile(hPipe, &data, sizeof(DWORD), &numberBytes, NULL);
				}
			}
			else
			{
				employee emp;
				if (!ReadFile(hPipe, &emp, sizeof(employee), &numberBytes, NULL))
				{
					cout << "ERROR! Cannot get employee from server\n";
					continue;
				}
				else
				{
					cout << emp.num << ' ' << emp.name << ' ' << emp.hours << "\n";
					cout << "enter any char to continue\n";
					char c;
					cin >> c;
					DWORD data = 1;
					WriteFile(hPipe, &data, sizeof(DWORD), &numberBytes, NULL);
					
				}
			}
		}
	}


}
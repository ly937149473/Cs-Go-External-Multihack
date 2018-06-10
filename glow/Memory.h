#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <iostream>

struct PModule
{
	DWORD dwBase;
	DWORD dwSize;
};

class process
{

public:
	bool Attach(char* pName, DWORD rights)
	{
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);

		do
			if (!strcmp(entry.szExeFile, pName)) {
				pID = entry.th32ProcessID;
				CloseHandle(handle);
				_process = OpenProcess(rights, false, pID);
				return true;
			}
		while (Process32Next(handle, &entry));
		return false;
	}
	PModule GetModule(char* moduleName) {
		HANDLE module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(mEntry);

		do {
			if (!strcmp(mEntry.szModule, (LPSTR)moduleName)) {
				CloseHandle(module);

				PModule mod = { (DWORD)mEntry.hModule, mEntry.modBaseSize };
				return mod;
			}
		} while (Module32Next(module, &mEntry));

		PModule mod = { (DWORD)false, (DWORD)false };
		return mod;
	}

	template <class T>
	T Read(DWORD addr) {
		T _read;
		ReadProcessMemory(_process, (LPVOID)addr, &_read, sizeof(T), NULL);
		return _read;
	}
	template <class T>
	void Write(DWORD addr, T val) {
		WriteProcessMemory(_process, (LPVOID)addr, &val, sizeof(T), NULL);
	}

	DWORD FindPattern(DWORD start, DWORD size, const char* sig, const char* mask) {
		BYTE* data = new BYTE[size];

		unsigned long bytesRead;
		if (!ReadProcessMemory(_process, (LPVOID)start, data, size, &bytesRead)) {
			return NULL;
		}

		for (DWORD i = 0; i < size; i++) {
			if (DataCompare((const BYTE*)(data + i), (const BYTE*)sig, mask)) {
				return start + i;
			}
		}
		return NULL;
	}

	DWORD FindPatternArray(DWORD start, DWORD size, const char* mask, int count, ...) {
		char* sig = new char[count + 1];
		va_list ap;
		va_start(ap, count);
		for (int i = 0; i < count; i++) {
			char read = va_arg(ap, char);
			sig[i] = read;
		}
		va_end(ap);
		sig[count] = '\0';
		return FindPattern(start, size, sig, mask);
	}

private:
	HANDLE _process;
	DWORD pID;
	bool DataCompare(const BYTE* pData, const BYTE* pMask, const char* pszMask) {
		for (; *pszMask; ++pszMask, ++pData, ++pMask) {
			if (*pszMask == 'x' && *pData != *pMask) {
				return false;
			}
		}
		return (*pszMask == NULL);
	}
};
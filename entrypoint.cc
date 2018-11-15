#include <vector>
#include <string>
#include <future>
#include <iostream>
#include <algorithm>

#include <conio.h>
#include <windows.h>
#include <tlhelp32.h>

static int suspicious_strings;

template <class InIter1, class InIter2>
void find_all(InIter1 buf_start, InIter1 buf_end, InIter2 pat_start, InIter2 pat_end)
{
	for (InIter1 pos = buf_start; buf_end != (pos = std::search(pos, buf_end, pat_start, pat_end)); ++pos)
	{
		++suspicious_strings;
	}
}

/**
 * @brief
 *  
 * @param state The state of the memory info
 * @param type The type of the memory info
 * 
 * @return true if state matches mem_commit and type is mapped or private
 */
auto is_valid_info(const DWORD state, const DWORD type) -> bool
{
	return state == MEM_COMMIT && (type == MEM_MAPPED || type == MEM_PRIVATE);
}

auto find_string_locations(const HANDLE process, std::string const &pattern) -> void
{
	unsigned char *p = nullptr;
	MEMORY_BASIC_INFORMATION info;

	for (p = nullptr; VirtualQueryEx(process, p, &info, sizeof info) == sizeof info; p += info.RegionSize)
	{
		std::vector<char> buffer;
		std::vector<char>::iterator pos;

		if (is_valid_info(info.State, info.Type))
		{
			SIZE_T bytes_read;

			buffer.resize(info.RegionSize);
			ReadProcessMemory(process, p, &buffer[0], info.RegionSize, &bytes_read);
			buffer.resize(bytes_read);

			find_all(buffer.begin(), buffer.end(), pattern.begin(), pattern.end());
		}
	}
}

auto is_javaw(const PROCESSENTRY32W &entry) -> bool
{
	return std::wstring(entry.szExeFile) == L"javaw.exe";
}

BOOL CALLBACK enum_windows_proc(const HWND hwnd, const LPARAM l_param)
{
	const auto &pids = *reinterpret_cast<std::vector<DWORD>*>(l_param);

	DWORD window_id;
	GetWindowThreadProcessId(hwnd, &window_id);

	for (auto pid : pids)
	{
		if (window_id == pid)
		{
			std::wstring title(GetWindowTextLength(hwnd) + 1, L'\0');
			GetWindowTextW(hwnd, &title[0], title.size());

			if (title.size() == 17) /// minecraft 1.7.10
			{
				std::cout << "[#] found desired window: \n";
				std::cout << "[#] process id: " << pid << '\n';
				std::wcout << "[#] process title: " << title << "\n";
			}
		}
	}

	return TRUE;
}

template<typename T, size_t N>
T * end(T(&ra)[N])
{
	return ra + N;
}

/**
 * @brief Resets the console's color to the default
 * 
 * @param h_console The console handle to use for https://docs.microsoft.com/en-us/windows/console/setconsoletextattribute
 */
auto reset_console_color(const HANDLE h_console) -> void
{
	SetConsoleTextAttribute(h_console, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
}

int main()
{
	const auto start = std::chrono::system_clock::now();
	const auto h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	std::vector<DWORD> pids;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof entry;

	if (!Process32FirstW(snap, &entry))
	{
		std::wcout << "unable to open snapshot entry";
		return 0;
	}

	int pid;

	do
	{
		if (is_javaw(entry))
		{
			pid = entry.th32ProcessID;
		}
	} while (Process32NextW(snap, &entry));

	pids.emplace_back(pid); /// put these in a vector, in future scan lsass and explorer for strings as well

	EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&pids)); /// enumerate the pid for windows

	const char *strings[] =
	{
		"Aim Assist" "Auto Clicker"
	}; /// NOTE: these are case sensitive

	std::vector<std::string> bad_strings(strings, end(strings)); /// define

	const auto process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, pid); /// don't inherit handle

	SetConsoleTextAttribute(h_console, FOREGROUND_RED);
	std::for_each(bad_strings.begin(), bad_strings.end(), [&](const std::string string)
	{
		std::async(std::launch::async, find_string_locations, process, string); /// maybe use about 5 threads to speed things up by ~70%
	});

	if (suspicious_strings > 0)
	{

		std::cout << std::endl << "found ";
		SetConsoleTextAttribute(h_console, FOREGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_INTENSITY);
		std::cout << suspicious_strings;

		reset_console_color(h_console);

		std::cout << " blacklisted string literals";
	} else
	{
		SetConsoleTextAttribute(h_console, FOREGROUND_GREEN | FOREGROUND_INTENSITY | FOREGROUND_INTENSITY);
		std::cout << std::endl << "unable to find any blacklisted string literals in memory" << std::endl;
	}

	reset_console_color(h_console);
	const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();

	if (diff < 500) /// minecraft crashed due to rpm, they injected something
	{
		SetConsoleTextAttribute(h_console, FOREGROUND_RED);
		std::cout << "player recently self destructed a client" << std::endl;
	}

	reset_console_color(h_console);
	std::cout << std::endl << "ran application in " << diff << "ms" << std::endl;
	std::cout << std::endl << "press any key to exit out of the program" << std::endl;
	_getch();

	return 0;
}

#include "ProcessInfo.h"
#include "SystemInfo.h"
#include <stdio.h>

#ifdef __linux__
#include <unistd.h>
#endif

int main()
{
	SystemInfo si;
	for (int i = 0; i < 1000000000; i++);
	printf("System:\n");
	printf("cpu usage = %.2f%%\n", si.GetSystemCPUUsage());
	printf("total memory = %.2fMB\n", si.GetSystemMemoryTotal());
	printf("used memory = %.2fMB\n", si.GetSystemMemoryUsed());
	
	printf("\n");

#ifdef _WIN32
	unsigned pid = GetCurrentProcessId();
#elif defined(__linux__)
	unsigned pid = getpid();
#endif
	ProcessInfo pi(pid);
	printf("Process %u:\n", pid);
	for (int i = 0; i < 1000000000; i++);
	printf("uptime = %llus\n", pi.GetProcessUptime());
	printf("thread count = %lu\n", pi.GetProcessThreadCount());
	printf("used memory = %.2fMB\n", pi.GetProcessMemoryUsed());
	printf("cpu usage = %.2f%%\n", pi.GetProcessCPUUsage());
	return 0;
}

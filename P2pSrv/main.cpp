#include "WindowsService.h"
#include <iostream>

/**
* @brief Main entry point for the Windows service application
*
* This function serves as the primary entry point for the service application.
* It handles command-line arguments for service installation/uninstallation
* and starts the service control dispatcher when run as a service.
*
* @param argc Number of command-line arguments
* @param argv Array of command-line argument strings
* @return int Application exit code (0 for success, non-zero for failure)
*
* @details Command-line argument handling:
* - "install": Installs the service in the Service Control Manager
* - "uninstall": Removes the service from the Service Control Manager
* - No arguments: Starts the service control dispatcher (normal service execution)
* - Invalid arguments: Displays usage information
*
* @details Service execution flow:
* 1. Parses command-line arguments
* 2. For install/uninstall: Calls appropriate static methods and exits
* 3. For normal execution: Sets up SERVICE_TABLE_ENTRY and calls StartServiceCtrlDispatcher
* 4. StartServiceCtrlDispatcher blocks until service terminates
* 5. Returns appropriate exit code based on operation result
*
* @note When run as a service, this function will not return until the service stops
*/
int _tmain(int argc, TCHAR* argv[])
{
	// Check command line arguments
	if (argc > 1)
	{
		if (_tcscmp(argv[1], _T("install")) == 0)
		{
			return CWindowsService::InstallService() ? 0 : 1;
		}
		else if (_tcscmp(argv[1], _T("uninstall")) == 0)
		{
			return CWindowsService::UninstallService() ? 0 : 1;
		}
		else
		{
			_tprintf(_T("Usage: %s [install|uninstall]\n"), argv[0]);
			return 1;
		}
	}

	// Service dispatch table
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)CWindowsService::ServiceMain },
		{ NULL, NULL }
	};

	// Start the service control dispatcher
	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}

	return 0;
}

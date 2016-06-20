#ifndef DEVICELIST_H
#define DEVICELIST_H

#include <Windows.h>
#include <mfidl.h>
#include <Mfapi.h>
#include "common.h"
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mfplat.lib")

class DeviceList
{
	UINT32      m_cDevices;         // Used to save the number of available devices
	IMFActivate **m_ppDevices;      // A pointer which points to the list of devices (another pointer)

public:
	DeviceList() : m_ppDevices(NULL), m_cDevices(0)
	{
		EnumerateDevices();
	}
	~DeviceList()
	{
		Clear();
	}

	UINT32  Count() const { return m_cDevices; }
	void    Clear();
	HRESULT EnumerateDevices();
	HRESULT GetDevice(UINT32 index, IMFActivate **ppActivate);
	HRESULT GetDeviceName(UINT32 index, WCHAR **ppszName);
};

#endif // DEVICELIST


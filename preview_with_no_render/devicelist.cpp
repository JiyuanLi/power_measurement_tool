#include "devicelist.h"

void DeviceList::Clear()
{
	for (UINT32 i = 0; i < m_cDevices; i++)
	{
		SafeRelease(&m_ppDevices[i]);   // Release each device
	}
	CoTaskMemFree(m_ppDevices);         // (Windows API)Release the list of devices
	m_ppDevices = NULL;                 // Point the m_ppDevice to NULL(To make sure it is not pointed to a random memory location)

	m_cDevices = 0;                     // Reset the number of available devices
}

// Initialize Device List
HRESULT DeviceList::EnumerateDevices()
{
	HRESULT hr = S_OK;
	IMFAttributes *pAttributes = NULL;

	Clear();

	// Initialize an attribute store. We will use this to
	// specify the enumeration parameters.

	hr = MFCreateAttributes(&pAttributes, 1);

	// Ask for source type = video capture devices
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetGUID(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
			);
	}

	// Enumerate devices.
	if (SUCCEEDED(hr))
	{
		hr = MFEnumDeviceSources(pAttributes, &m_ppDevices, &m_cDevices);
	}

	SafeRelease(&pAttributes);

	return hr;
}

// Get the required device pointer
HRESULT DeviceList::GetDevice(UINT32 index, IMFActivate **ppActivate)
{
	if (index >= Count())
	{
		return E_INVALIDARG;
	}

	*ppActivate = m_ppDevices[index];
	(*ppActivate)->AddRef();

	return S_OK;
}

// Get the name of the required device
HRESULT DeviceList::GetDeviceName(UINT32 index, WCHAR **ppszName)
{
	if (index >= Count())
	{
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;

	hr = m_ppDevices[index]->GetAllocatedString(
		MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
		ppszName,
		NULL
		);

	return hr;
}

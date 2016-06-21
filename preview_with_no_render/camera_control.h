#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H
#include <iostream>
#include <string>
#include <map>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <Dbt.h>
#include <Windows.h>
#include "devicelist.h"

#pragma comment(lib, "Ole32.Lib")
#pragma comment(lib, "mf.Lib")
#pragma comment(lib, "Mfplat.Lib")
#pragma comment(lib, "Mfuuid.Lib")
#pragma comment(lib, "Mfreadwrite.Lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Strmiids.lib")

class CameraControl: public IMFSourceReaderCallback
{
private:
	std::map<std::string, GUID> subtype_mapping;

	DWORD					m_streamIndex;

	DeviceList				DeviceListObj;

	long                    m_nRefCount;                        // Reference count.
	CRITICAL_SECTION        m_critsec;
	CameraState             CurrentState;

	IMFSourceReader         *m_pReader;
	BOOL                    m_bFirstSample;
	LONGLONG                m_llBaseTime;
	WCHAR                   *m_pwszSymbolicLink;

	HRESULT                 ConfigureSourceReader(long width, long height, int fps, std::string format);
	HRESULT                 EndCaptureInternal();
	HRESULT                 CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key);
public:
	CameraControl();
	virtual ~CameraControl();

	static HRESULT			CreateInstance(CameraControl **ppPlayer);

	// IUnknown methods
	STDMETHODIMP            QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG)    AddRef();
	STDMETHODIMP_(ULONG)    Release();

	// IMFSourceReaderCallback methods
	STDMETHODIMP OnReadSample(
		HRESULT hrStatus,
		DWORD dwStreamIndex,
		DWORD dwStreamFlags,
		LONGLONG llTimestamp,
		IMFSample *pSample
		);

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *)
	{
		return S_OK;
	}

	STDMETHODIMP OnFlush(DWORD)
	{
		return S_OK;
	}

	// Other Methods
	HRESULT                 OpenMediaSource(IMFActivate *pActivate);

	bool					Is_initialized(std::string sensor_name, int width, int height, int fps, std::string format);
	bool					Is_format_supported(std::string input_format);
	void					display_supported_format();
	int						preview_start();
	int						preview_stop();
};

#endif
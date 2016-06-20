#include "camera_control.h"
#include <Wmcodecdsp.h>
#include <algorithm>
#include <sstream>

using namespace std;

/*-------------CameraControl Control Class Private Methods----------------*/

// Private constructor
CameraControl::CameraControl() :
m_nRefCount(1),
m_pReader(NULL),
m_bFirstSample(TRUE),
m_llBaseTime(0),
m_pwszSymbolicLink(NULL),
CurrentState(State_Stop)
{
	InitializeCriticalSection(&m_critsec);
}

// Private destructor
CameraControl::~CameraControl()
{
	DeleteCriticalSection(&m_critsec);
}

// Configure the Resolution and fps of the Source Reader
HRESULT CameraControl::ConfigureSourceReader(long width, long height, int fps)
{
	GUID subtypes[] = {
		MFVideoFormat_NV12, MFVideoFormat_YUY2, MFVideoFormat_UYVY,
		MFVideoFormat_RGB32, MFVideoFormat_RGB24, MFVideoFormat_IYUV
	};

	HRESULT hr = S_OK;
	BOOL    bUseNativeType = FALSE;

	GUID subtype = { 0 };

	IMFMediaType *pSourceType = NULL;

	int typeIndex = -1;
	int typeCount = 0;
	while (SUCCEEDED(hr = m_pReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, typeCount, &pSourceType)))
	{
		PROPVARIANT varRes;
		PROPVARIANT varFPS;
		PropVariantInit(&varRes);
		PropVariantInit(&varFPS);

		// Get Resolution
		pSourceType->GetItem(MF_MT_FRAME_SIZE, &varRes);
		long type_width = varRes.cyVal.Hi;
		long type_height = varRes.cyVal.Lo;

		// Get FPS
		pSourceType->GetItem(MF_MT_FRAME_RATE, &varFPS);
		int type_fps = static_cast<int>(ceil(double(varFPS.uhVal.HighPart) / double(varFPS.uhVal.LowPart)));

		if (type_width == width&&type_height == height&&type_fps == fps)
		{
			typeIndex = typeCount;
			break;
		}
		SafeRelease(&pSourceType);
		typeCount++;
	}

	if (typeIndex == -1) { hr = -1;  goto done; }

	hr = pSourceType->GetGUID(MF_MT_SUBTYPE, &subtype);

	if (FAILED(hr)) { goto done; }

	for (UINT32 i = 0; i < ARRAYSIZE(subtypes); i++)
	{
		if (subtype == subtypes[i])
		{
			hr = m_pReader->SetCurrentMediaType(
				(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
				NULL,
				pSourceType
				);

			bUseNativeType = TRUE;
			break;
		}
	}

	if (!bUseNativeType)
	{
		cout << "Warning: Native Media Type is not in match media type list! Will choose one from the list instead" << endl;
		for (UINT32 i = 0; i < ARRAYSIZE(subtypes); i++)
		{
			hr = pSourceType->SetGUID(MF_MT_SUBTYPE, subtypes[i]);

			if (FAILED(hr)) { goto done; }

			hr = m_pReader->SetCurrentMediaType(
				(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
				NULL,
				pSourceType
				);

			if (SUCCEEDED(hr))
			{
				break;
			}
		}
	}

done:
	SafeRelease(&pSourceType);
	return hr;
}

HRESULT CameraControl::EndCaptureInternal()
{
	EnterCriticalSection(&m_critsec);
	HRESULT hr = S_OK;
	CurrentState = State_Stop;
	SafeRelease(&m_pReader);
	CoTaskMemFree(m_pwszSymbolicLink);
	m_pwszSymbolicLink = NULL;
	LeaveCriticalSection(&m_critsec);
	return hr;
}

HRESULT CameraControl::CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key)
{
	PROPVARIANT var;
	PropVariantInit(&var);

	HRESULT hr = S_OK;

	hr = pSrc->GetItem(key, &var);
	if (SUCCEEDED(hr))
	{
		hr = pDest->SetItem(key, var);
	}

	PropVariantClear(&var);
	return hr;
}

/*-------------Capture Control Class Public Methods----------------*/
HRESULT CameraControl::CreateInstance(
	CameraControl **ppCapture // Receives a pointer to the CCapture object.
	)
{
	if (ppCapture == NULL)
	{
		return E_POINTER;
	}

	CameraControl *pCapture = new (std::nothrow) CameraControl();

	if (pCapture == NULL)
	{
		return E_OUTOFMEMORY;
	}

	// The CCapture constructor sets the ref count to 1.
	*ppCapture = pCapture;

	return S_OK;
}

// Get Interface
HRESULT CameraControl::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(CameraControl, IMFSourceReaderCallback),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

// Add Reference
ULONG CameraControl::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

// Release CaptureControl Obj
ULONG CameraControl::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

HRESULT CameraControl::OnReadSample(
	HRESULT hrStatus,
	DWORD /*dwStreamIndex*/,
	DWORD /*dwStreamFlags*/,
	LONGLONG llTimeStamp,
	IMFSample *pSample      // Can be NULL
	)
{
	EnterCriticalSection(&m_critsec);

	if (CurrentState == State_Stop)
	{
		SafeRelease(&pSample);
		LeaveCriticalSection(&m_critsec);
		return S_OK;
	}

	HRESULT hr = S_OK;

	if (FAILED(hrStatus))
	{
		hr = hrStatus;
		goto done;
	}

	if (pSample)
	{
		if (m_bFirstSample)
		{
			m_llBaseTime = llTimeStamp;
			m_bFirstSample = FALSE;
		}

		// rebase the time stamp
		llTimeStamp -= m_llBaseTime;
		hr = pSample->SetSampleTime(llTimeStamp);
		if (FAILED(hr)) { goto done; }
		
		static int frame_counter = 0;
		static LONGLONG first_frame_time_stamp = llTimeStamp;
		static bool has_fps_output = false;
		static int fps_size = 0;
		
		if (frame_counter == 15)
		{
			if (has_fps_output)
			{
				for (int i = 0; i < fps_size; ++i)
					cout << "\b";
			}
			LONGLONG time_difference = (llTimeStamp - first_frame_time_stamp)/15;
			double int_fps = 1000 * 10000000 / time_difference;
			double db_fps = double (int_fps) / 100;
			cout << db_fps;
			has_fps_output = true;
			frame_counter = 0;
			first_frame_time_stamp = llTimeStamp;
			ostringstream ost;
			ost << db_fps;
			fps_size = ost.str().length();
		}
		frame_counter++;
	}
	else
	{
		if (!m_bFirstSample)
		{
			cout << "Error: No Sample was read! System will exit" << endl;
			goto done;
		}
	}

	// Read another sample.
	hr = m_pReader->ReadSample(
		(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
		0,
		NULL,   // actual
		NULL,   // flags
		NULL,   // timestamp
		NULL    // sample
		);

done:
	LeaveCriticalSection(&m_critsec);
	return hr;
}

// Create Source Reader from IMFActivate
HRESULT CameraControl::OpenMediaSource(IMFActivate *pSelectedDevice)
{
	HRESULT hr = S_OK;
	IMFMediaSource *pMediaSource = NULL;
	IMFAttributes *pSourceReaderAttributes = NULL;

	hr = pSelectedDevice->ActivateObject(IID_PPV_ARGS(&pMediaSource));
	if (SUCCEEDED(hr))
	{
		hr = MFCreateAttributes(&pSourceReaderAttributes, 2);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSourceReaderAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFCreateSourceReaderFromMediaSource(
			pMediaSource,
			pSourceReaderAttributes,
			&m_pReader
			);
	}
	SafeRelease(&pMediaSource);
	SafeRelease(&pSourceReaderAttributes);
	return hr;
}

bool CameraControl::Is_initialized(string sensor_name, int width, int height, int fps)
{
	/***************************** Open Selected Device *******************************/
	IMFActivate *Device = NULL;
	int Number_of_Device;
	this->DeviceListObj.EnumerateDevices();
	Number_of_Device = DeviceListObj.Count();
	if (Number_of_Device == 0)
	{
		cout << "Error: No Camera Device is detected!" << endl;
		return false;
	}
	for (UINT32 device_index = 0; device_index < Number_of_Device; ++device_index)
	{
		WCHAR *ptr_temp_sensor_name = NULL;
		this->DeviceListObj.GetDeviceName(device_index, &ptr_temp_sensor_name);
		wstring wstr_temp_sensor_name = ptr_temp_sensor_name;
		string str_temp_sensor_name(wstr_temp_sensor_name.length(), ' ');
		copy(wstr_temp_sensor_name.begin(), wstr_temp_sensor_name.end(), str_temp_sensor_name.begin());
		transform(str_temp_sensor_name.begin(), str_temp_sensor_name.end(), str_temp_sensor_name.begin(), toupper);

		if (str_temp_sensor_name == sensor_name)
		{
			DeviceListObj.GetDevice(device_index, &Device);
			cout << "Camera selected Successfully! Camera Name: " << str_temp_sensor_name << endl;
			break;
		}
	}
	if (Device == NULL)
	{
		cout << "Error: " << sensor_name << " is not detected on this device!" << endl;
		return false;
	}
	HRESULT hr = S_OK;

	hr = this->OpenMediaSource(Device);
	SafeRelease(&Device);

	/************************ Configure Source Reader **********************/
	EnterCriticalSection(&m_critsec);

	if (!SUCCEEDED(hr))
	{
		cout << "Error: Camera cannot be open!" << endl;
		return false;
	}
	hr = this->ConfigureSourceReader(width, height, fps);
	if (!SUCCEEDED(hr))
	{
		cout << "Error: " << width << "x" << height << "@" << fps << "fps is not supported!" << endl;
		return false;
	}
	LeaveCriticalSection(&m_critsec);
	bool return_value = false;
	if (SUCCEEDED(hr))
		return_value = true;
	else
		cout << "Error: Camera Initialization Failed!" << endl;
	return return_value;
}

int CameraControl::preview_start()
{ 
	EnterCriticalSection(&m_critsec);
	std::cout << "Preview has been started!" << std::endl; 
	m_bFirstSample = TRUE;
	m_llBaseTime = 0;
	CurrentState = State_Start;

	IMFMediaType *pSourceReaderType = NULL;
	m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pSourceReaderType);
	if (pSourceReaderType == NULL)
	{
		cout << "Error: Media Type was set unsuccessfully on Media Source!" << endl;
		return 1;
	}

	// Request the first video frame.
	HRESULT hr = S_OK;
	hr = this->m_pReader->ReadSample(
		(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
		0,
		NULL,
		NULL,
		NULL,
		NULL
		);
	bool return_val = false;
	if (!SUCCEEDED(hr))
		cout << "Error: Read Sample Failed!" << endl;
	else
		cout << "Frame Rate: " << endl;
	LeaveCriticalSection(&m_critsec);
	return 0; 
}

int CameraControl::preview_stop()
{ 
	EndCaptureInternal();
	std::cout << endl << "Preview has been stopped!" << std::endl; 
	return 0; 
}
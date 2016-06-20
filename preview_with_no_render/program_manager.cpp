#include "program_manager.h"
#include <Windows.h>
#include <io.h>
#include <sstream>
#include <thread>

using namespace std;

Program_Manager::Program_Manager(
	char *ch_arg_0, 
	char *ch_sensor_name, 
	char *ch_resolution, 
	char *ch_frame_rate, 
	char *ch_duration)
{
	this->str_arg_0 = ch_arg_0;
	this->str_sensor_name = ch_sensor_name;
	this->str_resolution = ch_resolution;
	this->str_frame_rate = ch_frame_rate;
	this->str_duration_in_second = ch_duration;
}

bool Program_Manager::Is_sensor_name_valid()
{
	// Verify Sensor Name:
	string sensor_list_doc_path("supported_sensor_list.txt");
	if (find(this->str_arg_0.begin(), this->str_arg_0.end(), ':') != this->str_arg_0.end())
	{
		string folder_path(this->str_arg_0.begin(), find(this->str_arg_0.rbegin(), this->str_arg_0.rend(), '\\').base());
		sensor_list_doc_path = folder_path + sensor_list_doc_path;
	}
	
	if (!(_access(sensor_list_doc_path.c_str(), 0)==0))
	{
		cout << "Could not find supported_sensor_list.txt, will use built-in list for checking" << endl;
		this->set_supported_sensor.insert({"IMX135", "IMX241", "IMX214", "OV2722", "OV2740", "OV5693", "OV8858", "MICROSOFT LIFECAM FRONT" });
	}
	else
	{
		ifstream sensor_list_doc(sensor_list_doc_path.c_str(), 'r');
		string temp_sensor;
		while (sensor_list_doc >> temp_sensor)
		{
			this->set_supported_sensor.insert(temp_sensor);
		}
		sensor_list_doc.close();
	}
	transform(this->str_sensor_name.begin(), this->str_sensor_name.end(), this->str_sensor_name.begin(), toupper);
	if (set_supported_sensor.find(this->str_sensor_name) == set_supported_sensor.end())
	{
		cout << "Input Checking Failed: Sensor " << this->str_sensor_name << " is NOT Supported!" << endl
			<< "Supported Sensor List: " << endl;
		for (auto sensor : this->set_supported_sensor)
			cout << sensor;
		cout << endl;
		return false;
	}
	cout << "Input Checking Successful: Sensor " << this->str_sensor_name << " Supported!" << endl;
	return true;
}

bool Program_Manager::Is_resolution_valid()
{
	string pattern("(\\d+)x(\\d+)");
	regex r(pattern, regex::icase);
	smatch results;
	if (regex_match(this->str_resolution, results, r))
	{
		cout << "Input Checking Successful: Resolution " << this->str_resolution << " will be tested!" << endl;
		stringstream temp_stringstream_1;
		stringstream temp_stringstream_2;
		temp_stringstream_1 << results[1].str();
		temp_stringstream_1 >> this->int_width;
		temp_stringstream_2 << results[2].str();
		temp_stringstream_2 >> this->int_height;
		return true;
	}
	cout << "Input Checking Failed: Resolution " << this->str_resolution << " is not valid!" << endl;
	return false;
}

bool Program_Manager::Is_frame_rate_valid()
{
	string pattern("\\d+");
	regex r(pattern);
	if (regex_match(this->str_frame_rate, r))
	{
		cout << "Input Checking Successful: Frame Rate " << this->str_frame_rate << " will be tested!" << endl;
		stringstream temp_stringstream;
		temp_stringstream << this->str_frame_rate;
		temp_stringstream >> this->int_frame_rate;
		return true;
	}
	cout << "Input Checking Failed: Frame Rate " << this->str_frame_rate << " is not valid" << endl;
	return false;
}

bool Program_Manager::Is_duration_valid()
{
	string pattern("\\d+");
	regex r(pattern);
	if (regex_match(this->str_duration_in_second, r))
	{
		cout << "Input Checking Successful: Application will run " << this->str_duration_in_second << ((this->int_duration > 1) ? " seconds" : " second") << endl;
		stringstream temp_stringstream;
		temp_stringstream << this->str_duration_in_second;
		temp_stringstream >> this->int_duration;
		return true;
	}
	cout << "Input Checking Failed: Duration " << this->str_duration_in_second << " is not valid" << endl;
	return false;
}

void Start_Preview(void *arg)
{
	CameraControl *ptr_Sensor = (CameraControl *)arg;
	ptr_Sensor->preview_start();
}

void Program_Manager::Start_Running()
{
	if (this->Is_sensor_name_valid()
		&& this->Is_resolution_valid()
		&& this->Is_frame_rate_valid()
		&& this->Is_duration_valid()
		&& this->selected_sensor_control.Is_initialized(
			this->str_sensor_name,
			this->int_width,
			this->int_height,
			this->int_frame_rate))
	{
		thread camera_execution_thread(Start_Preview, &(this->selected_sensor_control));
		//Start_Preview(&selected_sensor_control);
		Sleep(this->int_duration*1000);
		this->selected_sensor_control.preview_stop();
		if (camera_execution_thread.joinable())
		{
			camera_execution_thread.join();
		}
	}
}
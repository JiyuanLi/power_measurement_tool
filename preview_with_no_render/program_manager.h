#ifndef PROGRAM_MANAGER_H
#define PROGRAM_MANAGER_H
#include "camera_control.h"
#include <fstream>
#include <algorithm>
#include <set>
#include <regex>

class Program_Manager
{
private:
	int int_width;
	int int_height;
	int int_frame_rate;
	int int_duration;

	std::string str_arg_0;
	std::string str_sensor_name;
	std::string str_resolution;
	std::string str_frame_rate;
	std::string str_duration_in_second;
	std::set<std::string> set_supported_sensor;

	bool Is_sensor_name_valid();
	bool Is_resolution_valid();
	bool Is_frame_rate_valid();
	bool Is_duration_valid();

	CameraControl selected_sensor_control;

public:
	Program_Manager(char *ch_arg_0, char *ch_sensor_name, char *ch_resolution, char *ch_frame_rate, char *ch_duration);
	void Start_Running();
};
#endif
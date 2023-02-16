#include "stdafx.h"
#include "TemperatureSource.h"

/// <summary>
/// Initialises a temperature source.
/// </summary>
TemperatureSource::TemperatureSource()
{
	// initialise members
	data = NULL;
	range.ResetActualRange();
	frameTimeStamp = 0;
	cameraParams = CameraParameters();
	streamCount = 1;
	selectedStreamIndex = 0;
	newDataCallback = NULL;
	newDataCallbackUserData = NULL;
}

/// <summary>
/// Disposes the temperature source.
/// </summary>
TemperatureSource::~TemperatureSource()
{
	// data is a void*, the array will be managed by a derived class
}

/// <summary>
/// Registers a callback for notifying when new temperature data are available. Register NULL to unregister.
/// </summary>
/// <param name="callback">The callback.</param>
/// <param name="userData">User data object. It will be passed as argument when the callback is called.</param>
void TemperatureSource::RegisterNewDataCallback(NewDataCallback callback, void* userData)
{
	newDataCallback = callback;
	newDataCallbackUserData = userData;
}

/// <summary>
/// Ensures processing new temperature data, calls the new data callback.
/// </summary>
void TemperatureSource::DataReceived()
{
	// call the new data callback, if registered
	if (newDataCallback)
		newDataCallback(data, &range, frameTimeStamp, newDataCallbackUserData);
}

/// <summary>
/// Sets the selected stream index.
/// </summary>
/// <returns>Returns an error code.</returns>
int TemperatureSource::SetSelectedStreamIndex(uint8_t value)
{
	// check index bounds
	if (value >= streamCount)
		return LRC_ERR_INVALID_VALUE;
	
	// set the index
	selectedStreamIndex = value;
	return LRC_ERR_SUCCESS;
}

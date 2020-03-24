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
	newDataCallback = NULL;
}

/// <summary>
/// Disposes the temperature source.
/// </summary>
TemperatureSource::~TemperatureSource()
{
	// free the temperature data array
	if (data)
		delete data;
}

/// <summary>
/// Allocates the temperature data array according to the camera parameters.
/// </summary>
void TemperatureSource::AllocateData()
{
	// free the previous array if any
	if (data)
		delete data;

	// allocate the new array
	data = new float[cameraParams.width * cameraParams.height];
}

/// <summary>
/// Registers a callback for notifying when new temperature data are available. Register NULL to unregister.
/// </summary>
/// <param name="callback">The callback.</param>
void TemperatureSource::RegisterNewDataCallback(NewDataCallback callback)
{
	newDataCallback = callback;
}

/// <summary>
/// Ensures processing new temperature data, calls the new data callback.
/// </summary>
void TemperatureSource::DataReceived()
{
	// call the new data callback, if registered
	if (newDataCallback)
		newDataCallback(data, &range, frameTimeStamp);
}

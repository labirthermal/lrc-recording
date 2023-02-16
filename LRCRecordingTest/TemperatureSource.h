// Port of TemperatureSource from LabIR 1.5.17

#pragma once

#include "LRCErrorCodes.h"
#include "DataStructures.h"

/// <summary>
/// Base class for temperature data sources.
/// </summary>
class TemperatureSource
{
protected:
	/// <summary>
	/// The temperature data.
	/// </summary>
	void* data;

	/// <summary>
	/// The data range.
	/// </summary>
	Range range;

	/// <summary>
	/// The data time stamp.
	/// </summary>
	uint64_t frameTimeStamp;

	/// <summary>
	/// The camera parameters.
	/// </summary>
	CameraParameters cameraParams;

	/// <summary>
	/// The object parameters.
	/// </summary>
	ObjectParameters objectParams;

	/// <summary>
	/// The number of streams in this temperature source.
	/// </summary>
	uint8_t streamCount;

	/// <summary>
	/// The selected stream index.
	/// </summary>
	uint8_t selectedStreamIndex;

public:
	/// <summary>
	/// Function prototype of the callback for notifying when new temperature data are available.
	/// </summary>
	/// <param name="data">The frame data.</param>
	/// <param name="range">The data range.</param>
	/// <param name="timeStamp">The frame UTC time stamp in milliseconds since 1.1.1970.</param>
	/// <param name="userData">User data object. Set when registering the callback.</param>
	typedef void (*NewDataCallback)(void* data, Range* range, uint64_t timeStamp, void* userData);

private:
	/// <summary>
	/// Callback for notifying when new temperature data are available.
	/// </summary>
	NewDataCallback newDataCallback;

	/// <summary>
	/// User data object associated with the new data callback.
	/// </summary>
	void* newDataCallbackUserData;



public:
	/// <summary>
	/// Initialises a temperature source.
	/// </summary>
	TemperatureSource();

	/// <summary>
	/// Disposes the temperature source.
	/// </summary>
	~TemperatureSource();

protected:
	/// <summary>
	/// Allocates the temperature data array according to the camera parameters.
	/// </summary>
	virtual void AllocateData() = 0;

	/// <summary>
	/// Ensures processing new temperature data, calls the new data callback.
	/// </summary>
	virtual void DataReceived();

public:
	/// <summary>
	/// Registers a callback for notifying when new temperature data are available. Register NULL to unregister.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <param name="userData">User data object. It will be passed as argument when the callback is called.</param>
	void RegisterNewDataCallback(NewDataCallback callback, void* userData);

	/// <summary>
	/// Gets the current frame time stamp.
	/// </summary>
	uint64_t GetFrameTimeStamp() { return frameTimeStamp; }

	/// <summary>
	/// Gets a pointer to the temperature data array. The data are stored in row major format.
	/// </summary>
	void* GetTemperatureData() { return data; }

	/// <summary>
	/// Gets a pointer to the temperature data range.
	/// </summary>
	Range GetDataRange() { return range; }

	/// <summary>
	/// Gets the frame width.
	/// </summary>
	uint16_t GetWidth() { return cameraParams.width; }

	/// <summary>
	/// Gets the frame height.
	/// </summary>
	uint16_t GetHeight() { return cameraParams.height; }

	/// <summary>
	/// Gets the object parameters.
	/// Used for off-line sources such as recording files.
	/// </summary>
	virtual ObjectParameters GetObjectParameters() = 0;

	/// <summary>
	/// Sets the object parameters.
	/// Used for on-line sources such as cameras.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	virtual int SetObjectParameters(ObjectParameters value) = 0;

	/// <summary>
	/// Gets the number of streams.
	/// </summary>
	uint8_t GetStreamCount() { return streamCount; }

	/// <summary>
	/// Gets the selected stream index.
	/// </summary>
	uint8_t GetSelectedStreamIndex() { return selectedStreamIndex; }

	/// <summary>
	/// Sets the selected stream index.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	virtual int SetSelectedStreamIndex(uint8_t value);
};


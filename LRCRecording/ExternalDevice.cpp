#include "stdafx.h"
#include "ExternalDevice.h"

/// <summary>
/// Creates a basic external device for reading LRC files.
/// </summary>
/// <param name="name">The device name.</param>
/// <param name="dataSize">The device data size.</param>
BasicExternalDevice::BasicExternalDevice(string name, uint16_t dataSize)
{
	this->name = name;
	this->dataSize = dataSize;
	this->data = new char[dataSize];
}

/// <summary>
/// Disposes the external device.
/// </summary>
BasicExternalDevice::~BasicExternalDevice()
{
	delete[] data;
}

/// <summary>
/// Sets the device binary data. The array length is <see cref="DataSize"/>.
/// </summary>
/// <param name="data">The binary data array.</param>
/// <remarks>Setter is used when reading from a file.</remarks>
void BasicExternalDevice::SetBinaryData(const char* data)
{
	// make a copy of the data
	memcpy(this->data, data, dataSize);
}

/// <summary>
/// Gets the device formatted data.
/// </summary>
/// <returns>Returns the device formatted data.</returns>
string BasicExternalDevice::GetFormattedData()
{
	string result = "{";

	// first item without comma
	if (dataSize > 0)
		result.append(to_string(data[0]));

	// following items separated by a comma
	for (int i = 1; i < dataSize; i++)	// starts from 1
		result.append(", " + to_string(data[i]));

	result.append("}");
	return result;
}

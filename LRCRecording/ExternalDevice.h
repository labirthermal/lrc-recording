#pragma once

using namespace std;

/// <summary>
/// External device providing additional data to recorded frames.
/// </summary>
class IExternalDevice
{
public:
	/// <summary>
	/// Gets the device name.
	/// </summary>
	virtual string GetName() = 0;

	/// <summary>
	/// Gets the size of the device data. Must be fixed.
	/// </summary>
	virtual uint16_t GetDataSize() = 0;

	/// <summary>
	/// Gets the device binary data. The array length is <see cref="DataSize"/>.
	/// </summary>
	/// <remarks>Getter is used when writing to a file.</remarks>
	virtual const char* GetBinaryData() = 0;

	/// <summary>
	/// Sets the device binary data. The array length is <see cref="DataSize"/>.
	/// </summary>
	/// <param name="data">The binary data array.</param>
	/// <remarks>Setter is used when reading from a file.</remarks>
	virtual void SetBinaryData(const char* data) = 0;

	/// <summary>
	/// Gets the device formatted data.
	/// </summary>
	/// <returns>Returns the device formatted data.</returns>
	virtual string GetFormattedData() = 0;
};



class BasicExternalDevice : public IExternalDevice
{
protected:
	/// <summary>
	/// The device name.
	/// </summary>
	string name;

	/// <summary>
	/// The device data size.
	/// </summary>
	uint16_t dataSize;

	/// <summary>
	/// The device data.
	/// </summary>
	char* data;

public:
	/// <summary>
	/// Creates a basic external device for reading LRC files.
	/// </summary>
	/// <param name="name">The device name.</param>
	/// <param name="dataSize">The device data size.</param>
	BasicExternalDevice(string name, uint16_t dataSize);

	/// <summary>
	/// Disposes the external device.
	/// </summary>
	~BasicExternalDevice();

	/// <summary>
	/// Gets the device name.
	/// </summary>
	string GetName() override { return name; }

	/// <summary>
	/// Gets the size of the device data. Must be fixed.
	/// </summary>
	uint16_t GetDataSize() override { return dataSize; }

	/// <summary>
	/// Gets the device binary data. The array length is <see cref="DataSize"/>.
	/// </summary>
	/// <remarks>Getter is used when writing to a file.</remarks>
	const char* GetBinaryData() override { return data; }

	/// <summary>
	/// Sets the device binary data. The array length is <see cref="DataSize"/>.
	/// </summary>
	/// <param name="data">The binary data array.</param>
	/// <remarks>Setter is used when reading from a file.</remarks>
	void SetBinaryData(const char* data) override;

	/// <summary>
	/// Gets the device formatted data.
	/// </summary>
	/// <returns>Returns the device formatted data.</returns>
	string GetFormattedData() override;
};

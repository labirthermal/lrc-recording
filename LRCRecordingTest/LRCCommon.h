#pragma once

#include "stdafx.h"
#include "DataStructures.h"
#include "ExternalDevice.h"

/// <summary>
/// Common things for LRC reader and writer.
/// </summary>
class LRCCommon
{
public:
	/// <summary>
	/// LRC file signature.
	/// </summary>
	static const char FileTypeHeader[4];



	/// <summary>
	/// Data type for writing temperatures to file.
	/// </summary>
	enum FileDataType : uint8_t
	{
		UInt16Float,	// float temperatures saved as uint16, the frame temperature range is stretched to uint16 range
		Float,			// float temperatures saved as floats
		UInt16Raw		// uint16 intensities (raw data) saved as uint16
	};

	/// <summary>
	/// Determines the size of data element.
	/// </summary>
	/// <param name="type">The file data type.</param>
	/// <returns>Returns the size of data element.</returns>
	static int FileDataTypeSize(FileDataType type)
	{
		switch (type)
		{
		case LRCCommon::FileDataType::UInt16Float:
		case LRCCommon::FileDataType::UInt16Raw:
			return sizeof(uint16_t);

		case LRCCommon::FileDataType::Float:
			return sizeof(float);

		default:
			return -1;
		}
	}



	/// <summary>
	/// Struct that holds info about LRC stream.
	/// Contains TemperatureSource it's data type and units.
	/// </summary>
	struct StreamInfo
	{
		/// <summary>
		/// File data type of this stream.
		/// </summary>
		FileDataType dataType;

		/// <summary>
		/// Units of temperature.
		/// </summary>
		TemperatureSourceUnits units;

		/// <summary>
		/// Object parameters of this stream.
		/// </summary>
		ObjectParameters objectParams;
	};
};

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
	enum FileDataType : uint8_t { UInt16, Float };

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

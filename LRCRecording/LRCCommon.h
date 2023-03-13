#pragma once

#include "pch.h"
#include "DataStructures.h"
#include "ExternalDevice.h"

/// <summary>
/// Common things for LRC reader and writer.
/// </summary>
class LRCRECORDING_API LRCCommon
{
public:
	/// <summary>
	/// Flags indicating the presence of optional information in the file header.
	/// </summary>
	enum FileHeaderInfoFlags : uint32_t
	{
		None					= 0,

		RecordingName			= 1 << 0, // string
		RecordingLocation		= 1 << 1, // string
		RecordingClient			= 1 << 2, // string

		OperatorName			= 1 << 3, // string
		OperatorContact			= 1 << 4, // string
		OperatorQualification	= 1 << 5, // string

		CameraName				= 1 << 6, // string
		CameraAperture			= 1 << 7, // string
		CameraFilter			= 1 << 8, // string
		CameraWavelength		= 1 << 9, // uint16
		CameraIntegrationTime	= 1 << 10, // uint16
		CalibrationDate			= 1 << 11, // datetime

		FileNote				= 1 << 12, // string
		FrameNoteLengthBytes	= 1 << 13, // uint16

		// standard (ÈSN ISO 18434-1) fields
		CheckedDevices			= 1 << 14, // string
		OperationalConditions	= 1 << 15, // string

		// Qualitative check
		EnvironmentAttenuation	= 1 << 16, // string
		EnvironmentConditions	= 1 << 17, // string
		AllWindowFilterInfo		= 1 << 18, // string
		SpecialConditions		= 1 << 19, // string

		// Quantitative check
		MaxAndMeasuredLoad		= 1 << 20, // string
	};



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




	/// <summary>
	/// LRC file signature.
	/// </summary>
	static const char FileTypeHeader[4];
};

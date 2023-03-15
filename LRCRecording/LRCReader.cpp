#include "pch.h"
#include "LRCReader.h"

/// <summary>
/// Creates the LRC reader.
/// </summary>
LRCReader::LRCReader()
{
	// initialise fields
	sourceFileName = "";
	frameNote = "";
	streamCount = 0;
	streams = NULL;
	lrcVersion = 0;
}

/// <summary>
/// Closes the LRC reader.
/// </summary>
LRCReader::~LRCReader()
{
	Close();
}

/// <summary>
/// Opens a LRC file and reads the first frame.
/// </summary>
/// <param name="fileName">The input file name.</param>
/// <returns>Returns an error code.</returns>
int LRCReader::Open(string fileName)
{
	int err;

	// store the source file name
	this->sourceFileName = fileName;

	// open the input file
	fileIn.open(fileName, ifstream::binary);
	if (fileIn.fail())
		return LRC_ERR_FILE_OPEN;

	// read file header
	err = ReadFileHeader();
	if (LRC_FAILED(err))
		return err;

	// allocate memory for the temperature data
	AllocateData();
	// calculate header and frame sizes
	CalculateSizes();

	// determine file size
	struct stat fileStat;
	if (stat(fileName.c_str(), &fileStat) != 0)
		return LRC_ERR_DET_FILE_SIZE;
	// calculate the number of frames in the file
	frameCount = (int)((fileStat.st_size - fileHeaderSize) / frameSize);

	// get the time stamp of the first frame
	err = ReadFrameHeader();
	if (LRC_FAILED(err))
		return err;
	firstFrameTimeStamp = GetFrameTimeStamp();
	// determine recording duration
	if (frameCount == 1)
		duration = 0;
	else
	{
		// seek to the last frame, do not read it
		SeekToPositionOfFrame(frameCount - 1);
		// get the last frame time stamp
		uint64_t lastFrameTimeStamp;
		err = GetNextFrameTimeStamp(&lastFrameTimeStamp);
		if (LRC_FAILED(err))
			return err;
		// calculate the recording duration
		duration = lastFrameTimeStamp - firstFrameTimeStamp;
	}

	// read the first frame (must rewind after determining the duration)
	err = Rewind();
	if (LRC_FAILED(err))
		return err;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Closes the currently open file.
/// </summary>
void LRCReader::Close()
{
	// stop playback
	Pause();

	// close the input file
	if (fileIn.is_open())
		fileIn.close();

	// reset fields
	sourceFileName = "";
	frameNote = "";
	lrcVersion = 0;

	// release memory
	if (streamHeaderSizes)
	{
		delete[] streamHeaderSizes;
		streamHeaderSizes = NULL;
	}
	if (streamSizes)
	{
		delete[] streamSizes;
		streamSizes = NULL;
	}
	if (streams)
	{
		delete[] streams;
		streams = NULL;
		streamCount = 0;
	}
	if (floatData)
	{
		delete[] floatData;
		floatData = NULL;
	}
	if (uint16Data)
	{
		delete[] uint16Data;
		uint16Data = NULL;
	}
	// zero out the output data pointer (it was one of floatData or uint16Data)
	data = NULL;

	// dispose the dynamic header information
	for (map<LRCCommon::FileHeaderInfoFlags, void*>::iterator it = dynamicHeaderInfo.begin(); it != dynamicHeaderInfo.end(); it++)
		if (it->second != NULL)
			delete it->second;
	dynamicHeaderInfo.clear();

	// dispose external devices
	for (map<uint8_t, IExternalDevice*>::iterator it = externalDevices.begin(); it != externalDevices.end(); it++)
		delete it->second;
	// clear the list
	externalDevices.clear();
}

/// <summary>
/// Allocates the temperature data array according to the camera parameters.
/// </summary>
void LRCReader::AllocateData()
{
	// free the previous arrays if any
	if (floatData)
	{
		delete[] floatData;
		floatData = NULL;
	}
	if (uint16Data)
	{
		delete[] uint16Data;
		uint16Data = NULL;
	}

	// calculate the number of pixels in frame
	int pixelCount = cameraParams.width * cameraParams.height;

	// inspect the streams in the file
	for (int i = 0; i < streamCount; i++)
	{
		// allocate reading buffers as necessary
		switch (streams[i].dataType)
		{
		case LRCCommon::FileDataType::UInt16Float:
			// both arrays are necessary for reading uint16 and converting to float
			if (uint16Data == NULL)
				uint16Data = new uint16_t[pixelCount];
			if (floatData == NULL)
				floatData = new float[pixelCount];
			break;

		case LRCCommon::FileDataType::Float:
			// float array for reading float data
			if (floatData == NULL)
				floatData = new float[pixelCount];
			break;

		case LRCCommon::FileDataType::UInt16Raw:
			// uint16 array for reading uint16 data
			if (uint16Data == NULL)
				uint16Data = new uint16_t[pixelCount];
			break;
		}
	}

	// set the correct array for output
	SetOutputData();
}

/// <summary>
/// Sets which data to output (which data type).
/// </summary>
void LRCReader::SetOutputData()
{
	switch (GetSelectedStreamDataType())
	{
	case CameraDataType::Float:
		data = floatData;
		break;

	case CameraDataType::UInt16:
		data = uint16Data;
		break;

	default:
		data = NULL;
	}
}

/// <summary>
/// Gets the camera data type of the currently selected stream.
/// </summary>
CameraDataType LRCReader::GetSelectedStreamDataType()
{
	switch (streams[selectedStreamIndex].dataType)
	{
	case LRCCommon::FileDataType::UInt16Float:
	case LRCCommon::FileDataType::Float:
		return CameraDataType::Float;
		// theoretically, if uint16 data have been saved as float (doesn't make much sense, but it's possible),
		// we will read them as floats, not converted back to uint16

	case LRCCommon::FileDataType::UInt16Raw:
		return CameraDataType::UInt16;

	default:
		return CameraDataType::Unknown;
	}
}

/// <summary>
/// Sets the selected stream index.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCReader::SetSelectedStreamIndex(uint8_t value)
{
	// call the parent method
	int err = PlayableSource::SetSelectedStreamIndex(value);
	if (LRC_FAILED(err))
		return err;

	// update which data array to output
	SetOutputData();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Calculates sizes of headers and block from current settings.
/// </summary>
void LRCReader::CalculateSizes()
{
	// get the file header size
	fileHeaderSize = (long)fileIn.tellg();

	// frame time stamp
	frameHeaderSize = sizeof(int64_t);
	// frame note
	if (headerFlags & LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes)
		frameHeaderSize += *(uint16_t*)dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes];

	// calculate the size of external device IDs and data
	int sumDataSize = 0;
	for (map<uint8_t, IExternalDevice*>::iterator it = externalDevices.begin(); it != externalDevices.end(); it++)
		sumDataSize += it->second->GetDataSize();
	frameHeaderSize += sumDataSize + sizeof(uint8_t) * externalDevices.size(); // data of external devices

	// calculate the size of streams
	streamHeaderSizes = new long[streamCount];
	streamSizes = new long[streamCount];
	for (int i = 0; i < streamCount; i++)
	{
		// determine data element size
		int dataTypeSize = LRCCommon::FileDataTypeSize(streams[i].dataType);

		// calculate stream header size
		streamHeaderSizes[i] =
			2 * sizeof(float) + // stream header data range
			2 * sizeof(float) + // stream header camera range
			8 * sizeof(float) + sizeof(uint8_t); // stream header object parameters

		// calculate stream size
		streamSizes[i] = streamHeaderSizes[i] + dataTypeSize * cameraParams.width * cameraParams.height;
	}

	// calculate the whole frame size
	frameSize = frameHeaderSize;
	for (int i = 0; i < streamCount; i++)
		frameSize += streamSizes[i];
}

//
// Reads a 32bit integer from a compressed format.
//
Expected<int> LRCReader::Read7BitEncodedInt()
{
	int result = 0;
	char shift = 0;
	char b;

	do
	{
		if (shift >= 35)
			return Expected<int>::Failed(LRC_ERR_FILE_READ);

		fileIn.read(&b, sizeof(char));
		result |= (b & 0x7F) << shift;
		shift += 7;

	} while (b & 0x80);

	return Expected<int>(result);
}

/// <summary>
/// Reads a string from the file.
/// </summary>
/// <param name="length">If positive, specifies the number of bytes to read. If negative, reads one byte and uses that as the length. If zero, returns an error.</param>
/// <returns>Returns the string.</returns>
Expected<string> LRCReader::ReadString(int length)
{
	// the length cannot be zero
	if (length == 0)
		return Expected<string>::Failed(LRC_ERR_INVALID_VALUE);

	// if the string length is not specified
	if (length <= 0)
	{
		// read the string length
		Expected<int> len = Read7BitEncodedInt();
		if (!len.HasValue())
			return Expected<string>::Failed(len.error);
		length = len.value;
	}

	// the string is fixed length so theoretically it may not be null terminated
	int lengthPlusNull = length + 1;

	// read the string bytes
	char* buffUtf8 = new char[lengthPlusNull];
	fileIn.read(buffUtf8, length);
	if (fileIn.fail())
		return Expected<string>::Failed(LRC_ERR_FILE_READ);
	// make sure the string is null terminated
	buffUtf8[length] = '\0';

	// apparently it's not possible to convert a UTF-8 string to an ANSI string directly
	// convert UTF-8 to ANSI wide
	wchar_t* buffAnsiWide = new wchar_t[lengthPlusNull];
	MultiByteToWideChar(CP_UTF8, 0, buffUtf8, -1, buffAnsiWide, lengthPlusNull);
	// convert ANSI wide to ANSI narrow
	char* buffAnsi = new char[lengthPlusNull];
	WideCharToMultiByte(CP_ACP, 0, buffAnsiWide, -1, buffAnsi, lengthPlusNull, NULL, NULL);
	string s(buffAnsi);

	delete[] buffUtf8, buffAnsiWide, buffAnsi;
	return Expected<string>(s);
}

/// <summary>
/// Reads data of file header.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCReader::ReadFileHeader()
{
	// check the file signature
	char signature[4];
	fileIn.read(signature, 4);
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	if (strcmp(signature, LRCCommon::FileTypeHeader))
		return LRC_ERR_FILE_SIGNATURE;

	// read file version
	fileIn.read((char*)&lrcVersion, sizeof(uint8_t));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	// version 1 had no version field in the file, there was the first byte of the file time stamp which is 8
	// so let's rely on the fact that the file version will not get to 8 anytime soon
	if (lrcVersion == 0 || lrcVersion >= 8)
		// fallback to version 1
		lrcVersion = 1;

	// read the external devices
	switch (lrcVersion)
	{
	case 1:
		// LRC version 1
		// seek back to byte 4 (there is no version field in LRC version 1)
		fileIn.seekg(4, ifstream::beg);
		// skip recording date
		fileIn.ignore(sizeof(int64_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		// prepare objects for the external devices
		uint8_t extDevCount1;
		fileIn.read((char*)&extDevCount1, sizeof(uint8_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		for (uint8_t i = 0; i < extDevCount1; i++)
			externalDevices.insert(pair<uint8_t, IExternalDevice*>(i, new BasicExternalDevice("legacy device", sizeof(short))));
		break;

	case 2:
		// LRC version 2
		// skip recording date
		fileIn.ignore(sizeof(int64_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		// read the external devices
		uint8_t extDevCount2;
		fileIn.read((char*)&extDevCount2, sizeof(uint8_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		for (int i = 0; i < extDevCount2; i++)
		{
			// read the external device information
			// device ID
			uint8_t id;
			fileIn.read((char*)&id, sizeof(uint8_t));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			// device name
			Expected<string> name = ReadString();
			if (!name.HasValue())
				return name.error;
			// device data size
			uint16_t dataSize;
			fileIn.read((char*)&dataSize, sizeof(uint16_t));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;

			// create a basic device
			externalDevices.insert(pair<uint8_t, IExternalDevice*>(id, new BasicExternalDevice(name.value, dataSize)));
		}
		break;

	default:
		return LRC_ERR_FILE_VERSION;
	}

	// read the streams
	fileIn.read((char*)&streamCount, sizeof(uint8_t));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	streams = new LRCCommon::StreamInfo[streamCount];
	for (int i = 0; i < streamCount; i++)
	{
		// read the stream data type and units
		fileIn.read((char*)&streams[i], 2 * sizeof(uint8_t));	// do not use sizeof(StreamInfo), there are more variables
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
	}

	// read the camera resolution
	uint16_t res[2];
	fileIn.read((char*)&res, sizeof(res));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	cameraParams.width = res[0];
	cameraParams.height = res[1];

	// read flags indicating optional fields
	fileIn.read((char*)&headerFlags, sizeof(uint32_t));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;

#pragma region Reading dynamic header information
	// read the dynamic header information
	// most of the information are strings, read them in a loop
	LRCCommon::FileHeaderInfoFlags stringFlags1[] =
	{
		LRCCommon::FileHeaderInfoFlags::RecordingName,
		LRCCommon::FileHeaderInfoFlags::RecordingLocation,
		LRCCommon::FileHeaderInfoFlags::RecordingClient,
		LRCCommon::FileHeaderInfoFlags::OperatorName,
		LRCCommon::FileHeaderInfoFlags::OperatorContact,
		LRCCommon::FileHeaderInfoFlags::OperatorQualification,
		LRCCommon::FileHeaderInfoFlags::CameraName,
		LRCCommon::FileHeaderInfoFlags::CameraAperture,
		LRCCommon::FileHeaderInfoFlags::CameraFilter,
	};
	for (LRCCommon::FileHeaderInfoFlags flag : stringFlags1)
		if (headerFlags & flag)
		{
			Expected<string> str = ReadString();
			if (!str.HasValue())
				return str.error;
			dynamicHeaderInfo[flag] = new string(str.value);
		}

	if (headerFlags & LRCCommon::FileHeaderInfoFlags::CameraWavelength)
	{
		uint16_t wavelength;
		fileIn.read((char*)&wavelength, sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::CameraWavelength] = new uint16_t(wavelength);
	}

	if (headerFlags & LRCCommon::FileHeaderInfoFlags::CameraIntegrationTime)
	{
		uint16_t integTime;
		fileIn.read((char*)&integTime, sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::CameraIntegrationTime] = new uint16_t(integTime);
	}
	
	if (headerFlags & LRCCommon::FileHeaderInfoFlags::CalibrationDate)
	{
		int64_t binTime;
		fileIn.read((char*)&binTime, sizeof(int64_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::CalibrationDate] = new uint64_t(DateTime::TimeFromBinary(binTime));
	}
	
	if (headerFlags & LRCCommon::FileHeaderInfoFlags::FileNote)
	{
		Expected<string> str = ReadString();
		if (!str.HasValue())
			return str.error;
		dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::FileNote] = new string(str.value);
	}
	
	if (headerFlags & LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes)
	{
		uint16_t noteLength;
		fileIn.read((char*)&noteLength, sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes] = new uint16_t(noteLength);
	}

	LRCCommon::FileHeaderInfoFlags stringFlags2[] =
	{
		LRCCommon::FileHeaderInfoFlags::CheckedDevices,
		LRCCommon::FileHeaderInfoFlags::OperationalConditions,
		LRCCommon::FileHeaderInfoFlags::EnvironmentAttenuation,
		LRCCommon::FileHeaderInfoFlags::EnvironmentConditions,
		LRCCommon::FileHeaderInfoFlags::AllWindowFilterInfo,
		LRCCommon::FileHeaderInfoFlags::SpecialConditions,
		LRCCommon::FileHeaderInfoFlags::MaxAndMeasuredLoad,
	};
	for (LRCCommon::FileHeaderInfoFlags flag : stringFlags2)
		if (headerFlags & flag)
		{
			Expected<string> str = ReadString();
			if (!str.HasValue())
				return str.error;
			dynamicHeaderInfo[flag] = new string(str.value);
		}

	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
#pragma endregion

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Reads data of current frame header.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCReader::ReadFrameHeader()
{
	// read the frame time stamp
	int64_t binTime;
	fileIn.read((char*)&binTime, sizeof(int64_t));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	frameTimeStamp = DateTime::TimeFromBinary(binTime);

	// if the frame may have a note
	if (headerFlags & LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes)
	{
		// get the note length
		uint16_t noteLength = *(uint16_t*)dynamicHeaderInfo[LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes];
		if (noteLength > 0)
		{
			// read the frame note
			Expected<string> note = ReadString(noteLength);
			if (!note.HasValue())
				return note.error;
			frameNote = note.value;
		}
	}

	// read the data of external devices
	switch (lrcVersion)
	{
	case 1:
		// LRC version 1
		for (uint8_t i = 0; i < externalDevices.size(); i++)
		{
			char devData[sizeof(int16_t)];
			fileIn.read(devData, sizeof(int16_t));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			externalDevices.at(i)->SetBinaryData(devData);
		}
		break;

	case 2:
		// LRC version 2
		for (uint8_t i = 0; i < externalDevices.size(); i++)
		{
			// read the device ID
			uint8_t id;
			fileIn.read((char*)&id, sizeof(uint8_t));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			// get the device by ID
			IExternalDevice* device = externalDevices.at(id);
			// read the device data
			uint16_t devDataSize = device->GetDataSize();
			char* devData = new char[devDataSize];
			fileIn.read(devData, devDataSize);
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			device->SetBinaryData(devData);
			delete[] devData;
		}
		break;

	default:
		return LRC_ERR_FILE_VERSION;
	}

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Reads data of selected stream header.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCReader::ReadStreamHeader()
{
	// read the data temperature range
	float dr[2];
	fileIn.read((char*)dr, sizeof(dr));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	range.min = dr[0];
	range.max = dr[1];

	// read the camera temperature range
	float cr[2];
	fileIn.read((char*)cr, sizeof(cr));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	cameraParams.tempRangeMinK = cr[0];
	cameraParams.tempRangeMaxK = cr[1];

	// read the object parameters
	fileIn.read((char*)&objectParams, 8 * sizeof(float) + sizeof(bool));	// do not use sizeof(ObjectParameters), there are some additional constants
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
	// calculate tau if necessary
	if (!objectParams.isTauSet)
		objectParams.CalculateTau();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Reads the current frame data.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCReader::GetFrame()
{
	int err;

	// read the frame header
	err = ReadFrameHeader();
	if (LRC_FAILED(err))
		return err;

	// seek to the selected stream
	for (int i = 0; i < selectedStreamIndex; i++)
		fileIn.seekg(streamSizes[i], ifstream::cur);

	// read the stream header
	err = ReadStreamHeader();
	if (LRC_FAILED(err))
		return err;

	// read the temperature data
	int pixelCount = cameraParams.width * cameraParams.height;
	switch (streams[selectedStreamIndex].dataType)
	{
	case LRCCommon::FileDataType::UInt16Float:
		{
		// read float saved as uint16
		fileIn.read((char*)uint16Data, pixelCount * sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		// calculate the stretch from uint16 range to the actual temperature range
		double stretchFactor = range.GetLength() / UINT16_MAX;
		// decode from uint16 to actual temperatures
		for (int i = 0; i < pixelCount; i++)
			floatData[i] = (float)(uint16Data[i] * stretchFactor + range.min);
		}
		break;

	case LRCCommon::FileDataType::Float:
		// read float as float
		fileIn.read((char*)floatData, pixelCount * sizeof(float));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		// theoretically, if uint16 data have been saved as float (doesn't make much sense, but it's possible),
		// we will read them as floats, not converted back to uint16
		break;

	case LRCCommon::FileDataType::UInt16Raw:
		// read uint16 as uint16
		fileIn.read((char*)uint16Data, pixelCount * sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		break;
	}

	// seek to the next frame
	for (int i = selectedStreamIndex + 1; i < streamCount; i++)
		fileIn.seekg(streamSizes[i], ifstream::cur);

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Reads the time stamp of the next frame and returns the file position to the beginning of that frame.
/// </summary>
/// <param name="timeStamp">Returns the time stamp of the next frame, or if the function fails.</param>
/// <returns>Returns an error code.</returns>
int LRCReader::GetNextFrameTimeStamp(uint64_t* timeStamp)
{
	// reset the time stamp (it should be zero in case of read error)
	*timeStamp = 0;

	// read the time stamp
	uint64_t binTimeStamp;
	fileIn.read((char*)&binTimeStamp, sizeof(uint64_t));
	if (fileIn.fail())
	{
		if (fileIn.eof())
			return LRC_ERR_REC_END;
		else
			return LRC_ERR_FILE_READ;
	}

	// seek back to the beginning of the frame
	fileIn.seekg(-(int)sizeof(uint64_t), ifstream::cur);

	// convert the binary data to DateTime
	*timeStamp = DateTime::TimeFromBinary(binTimeStamp);
	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Seek before first frame.
/// </summary>
void LRCReader::RewindToBeginning()
{
	fileIn.seekg(fileHeaderSize);
}

/// <summary>
/// Seek before frame of given index.
/// </summary>
void LRCReader::SeekToPositionOfFrame(int frameIndex)
{
	fileIn.seekg(fileHeaderSize + frameIndex * frameSize);
}

/// <summary>
/// Seek before previous frame.
/// </summary>
void LRCReader::StepBackward()
{
	fileIn.seekg(-2 * frameSize, ifstream::cur);
}

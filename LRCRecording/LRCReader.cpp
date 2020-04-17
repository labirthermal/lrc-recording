#include "stdafx.h"
#include "LRCReader.h"

/// <summary>
/// Creates the LRC reader.
/// </summary>
/// <param name="fileName">The input file name.</param>
LRCReader::LRCReader(string fileName)
	: PlayableSource(fileName)
{
	int err;

	// open the input file
	fileIn.open(fileName, ifstream::binary);

	//DynamicHeaderInfo = new SortedDictionary<FileHeaderInfo, object>();

	// read file header
	err = ReadFileHeader();
	if (LRC_FAILED(err))
		throw err;

	// allocate memory for the temperature data
	AllocateData();
	// calculate header and frame sizes
	CalculateSizes();

	// determine file size
	struct stat fileStat;
	if (stat(fileName.c_str(), &fileStat) != 0)
		throw LRC_ERR_DET_FILE_SIZE;
	// calculate the number of frames in the file
	frameCount = (int)((fileStat.st_size - fileHeaderSize) / frameSize);

	// get the time stamp of the first frame
	err = ReadFrameHeader();
	if (LRC_FAILED(err))
		throw err;
	firstFrameTimeStamp = GetFrameTimeStamp();
	// determine recording duration
	if (frameCount == 1)
		duration = 0;
	else
	{
		// seek to the last frame, do not read it
		SeekToPositionOfFrame(frameCount - 1);
		// calculate the duration as the difference of time stamps
		duration = GetNextFrameTimeStamp() - firstFrameTimeStamp;
	}

	// read the first frame (must rewind after determining the duration)
	err = Rewind();
	if (LRC_FAILED(err))
		throw err;
}

/// <summary>
/// Closes the LRC reader.
/// </summary>
LRCReader::~LRCReader()
{
	// stop playback
	Pause();

	// close the input file
	if (fileIn.is_open())
		fileIn.close();

	// release memory
	if (streamHeaderSizes)
		delete streamHeaderSizes;
	if (streamSizes)
		delete streamSizes;
	if (streams)
		delete streams;
	if (uint16buff)
		delete uint16buff;

	// dispose external devices
	for (map<uint8_t, IExternalDevice*>::iterator it = externalDevices.begin(); it != externalDevices.end(); it++)
		delete it->second;
}

/// <summary>
/// Allocates the temperature data array according to the camera parameters.
/// </summary>
void LRCReader::AllocateData()
{
	PlayableSource::AllocateData();

	// allocate buffer for reading unit16 data
	if (uint16buff)
		delete uint16buff;

	uint16buff = new uint16_t[cameraParams.width * cameraParams.height];
}

/// <summary>
/// Calculates sizes of headers and block from current settings.
/// </summary>
void LRCReader::CalculateSizes()
{
	fileHeaderSize = fileIn.tellg();

	// frame time stamp
	frameHeaderSize = sizeof(int64_t);
	/* frame note not supported
	// frame note
	if (DynamicHeaderInfo.ContainsKey(FileHeaderInfo.FrameNoteLengthBytes))
		FrameHeaderSize += (UInt16)DynamicHeaderInfo[FileHeaderInfo.FrameNoteLengthBytes];
	*/

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
		// determine data type size
		int dataTypeSize = streams[i].dataType == LRCCommon::FileDataType::UInt16 ? sizeof(uint16_t) : sizeof(float);

		// calculate stream header size
		streamHeaderSizes[i] =
			2 * sizeof(float) + // stream header data range
			2 * sizeof(float) + // stream header camera range
			8 * sizeof(float) + sizeof(uint8_t); // stream header object parameters

		// calculate stream size
		streamSizes[i] = streamHeaderSizes[i] + dataTypeSize * GetWidth() * GetHeight();
	}

	// calculate the whole frame size
	frameSize = frameHeaderSize;
	for (int i = 0; i < streamCount; i++)
		frameSize += streamSizes[i];
}

/// <summary>
/// Reads data of file header.
/// </summary>
/// <returns>Returns an error coed.</returns>
int LRCReader::ReadFileHeader()
{
	// check the file signature
	char signature[4];
	fileIn.read(signature, 4);
	if (strcmp(signature, LRCCommon::FileTypeHeader))
		return LRC_ERR_FILE_SIGNATURE;

	// read file version
	fileIn.read((char*)&lrcVersion, sizeof(uint8_t));
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
		// prepare objects for the external devices
		uint8_t extDevCount1;
		fileIn.read((char*)&extDevCount1, sizeof(uint8_t));
		for (uint8_t i = 0; i < extDevCount1; i++)
			externalDevices.insert(pair<uint8_t, IExternalDevice*>(i, new BasicExternalDevice("legacy device", sizeof(short))));
		break;

	case 2:
		// LRC version 2
		// skip recording date
		fileIn.ignore(sizeof(int64_t));
		// read the external devices
		uint8_t extDevCount2;
		fileIn.read((char*)&extDevCount2, sizeof(uint8_t));
		for (int i = 0; i < extDevCount2; i++)
		{
			// read the external device information
			// device ID
			uint8_t id;
			fileIn.read((char*)&id, sizeof(uint8_t));
			// device name
			char nameLength;
			fileIn.read(&nameLength, sizeof(char));
			char* nameChr = new char[nameLength + 1];
			fileIn.read(nameChr, nameLength);
			string name(nameChr, nameLength);
			delete nameChr;
			// device data size
			uint16_t dataSize;
			fileIn.read((char*)&dataSize, sizeof(uint16_t));

			// create a basic device
			externalDevices.insert(pair<uint8_t, IExternalDevice*>(id, new BasicExternalDevice(name, dataSize)));
		}
		break;

	default:
		return LRC_ERR_FILE_VERSION;
	}

	// read the streams
	fileIn.read((char*)&streamCount, sizeof(uint8_t));
	streams = new LRCCommon::StreamInfo[streamCount];
	for (int i = 0; i < streamCount; i++)
	{
		uint8_t dataType, units;
		fileIn.read((char*)&dataType, sizeof(uint8_t));
		fileIn.read((char*)&units, sizeof(uint8_t));

		streams[i].dataType = (LRCCommon::FileDataType)dataType;
		streams[i].units = (TemperatureSourceUnits)units;
	}

	// read the camera resolution
	fileIn.read((char*)&cameraParams.width, sizeof(uint16_t));
	fileIn.read((char*)&cameraParams.height, sizeof(uint16_t));

	// read flags indicating optional fields
	uint32_t headerFlags;
	fileIn.read((char*)&headerFlags, sizeof(uint32_t));
	// optional fields are not supported
	if (headerFlags > 0)
		return LRC_ERR_OPTIONAL_DATA;

	/*
	foreach(var flag in HeaderFlags.ToValues())
	{
		if (LRCFileData.StringFlags.Contains(flag))
			DynamicHeaderInfo[flag] = fileIn.ReadString();
		else if (LRCFileData.UInt16Flags.Contains(flag))
			DynamicHeaderInfo[flag] = fileIn.ReadUInt16();
		else if (LRCFileData.DateTimeFlags.Contains(flag))
			DynamicHeaderInfo[flag] = DateTime.FromBinary(fileIn.ReadInt64());
	}
	*/

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Reads data of current frame header.
/// </summary>
/// <returns>Returns an error coed.</returns>
int LRCReader::ReadFrameHeader()
{
	// read the frame time stamp
	int64_t binTime;
	fileIn.read((char*)&binTime, sizeof(int64_t));
	frameTimeStamp = DateTime::TimeFromBinary(binTime);

	/* frame notes not supported
	// if the frame may have a note
	if (HeaderFlags.HasFlag(FileHeaderInfo.FrameNoteLengthBytes.ToFlag()) && (UInt16)DynamicHeaderInfo[FileHeaderInfo.FrameNoteLengthBytes] > 0)
	{
		// read the frame note
		byte[] noteBytes = fileIn.ReadBytes((UInt16)DynamicHeaderInfo[FileHeaderInfo.FrameNoteLengthBytes]);
		// decode the bytes, trim trailing nulls
		FrameNote = RecorderWriter.StringEncoding.GetString(noteBytes).TrimEnd('\0');
	}
	*/

	// read the data of external devices
	switch (lrcVersion)
	{
	case 1:
		// LRC version 1
		for (size_t i = 0; i < externalDevices.size(); i++)
		{
			char devData[sizeof(int16_t)];
			fileIn.read(devData, sizeof(int16_t));
			externalDevices.at(i)->SetBinaryData(devData);
		}
		break;

	case 2:
		// LRC version 2
		for (size_t i = 0; i < externalDevices.size(); i++)
		{
			// read the device ID
			uint8_t id;
			fileIn.read((char*)&id, sizeof(uint8_t));
			// get the device by ID
			IExternalDevice* device = externalDevices.at(id);
			// read the device data
			uint16_t devDataSize = device->GetDataSize();
			char* devData = new char[devDataSize];
			fileIn.read(devData, devDataSize);
			device->SetBinaryData(devData);
			delete devData;
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
void LRCReader::ReadStreamHeader()
{
	// read the data range
	range.ResetActualRange();
	float min, max;
	fileIn.read((char*)&min, sizeof(float));
	fileIn.read((char*)&max, sizeof(float));
	range.IncludeValue(min);
	range.IncludeValue(max);

	// skip the camera temperature range
	fileIn.ignore(2 * sizeof(float));

	// read the object parameters
	fileIn.read((char*)&objectParams.emissivity, sizeof(float));
	fileIn.read((char*)&objectParams.reflectedTempK, sizeof(float));
	fileIn.read((char*)&objectParams.distance, sizeof(float));
	fileIn.read((char*)&objectParams.atmosphericTempK, sizeof(float));
	fileIn.read((char*)&objectParams.relHumidity, sizeof(float));
	fileIn.read((char*)&objectParams.tau, sizeof(float));
	fileIn.read((char*)&objectParams.windowTempK, sizeof(float));
	fileIn.read((char*)&objectParams.windowTransmiss, sizeof(float));
	fileIn.read((char*)&objectParams.isTauSet, sizeof(bool));
	// calculate tau if necessary
	if (!objectParams.isTauSet)
		objectParams.CalculateTau();
}

/// <summary>
/// Reads the current frame data.
/// </summary>
/// <returns>Returns an error coed.</returns>
int LRCReader::GetFrame()
{
	int err = ReadFrameHeader();
	if (LRC_FAILED(err))
		return err;

	// seek to the selected stream
	for (int i = 0; i < selectedStreamIndex; i++)
		fileIn.seekg(streamSizes[i], ifstream::cur);

	ReadStreamHeader();

	// read the temperature data
	int pixelCount = cameraParams.width * cameraParams.height;
	switch (streams[selectedStreamIndex].dataType)
	{
	case LRCCommon::FileDataType::UInt16:
		{
		// read uint16 data
		fileIn.read((char*)uint16buff, pixelCount * sizeof(uint16_t));
		// decode from uint16 to actual temperatures
		double rangeLength = range.GetLength();
		for (int i = 0; i < pixelCount; i++)
			data[i] = (float)(((double)uint16buff[i] / UINT16_MAX) * rangeLength + range.min);
		}
		break;

	case LRCCommon::FileDataType::Float:
		// read float data
		fileIn.read((char*)data, pixelCount * sizeof(float));
		break;
	}

	// seek to the next frame
	for (int i = selectedStreamIndex + 1; i < streamCount; i++)
		fileIn.seekg(streamSizes[i], ifstream::cur);

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Gets the time stamp of the next frame.
/// </summary>
uint64_t LRCReader::GetNextFrameTimeStamp()
{
	// check end of file
	if (fileIn.eof())
		return 0;

	// read the time stamp
	uint64_t binTimeStamp;
	fileIn.read((char*)&binTimeStamp, sizeof(uint64_t));

	// seek back to the beginning of the frame
	fileIn.seekg(-(int)sizeof(uint64_t), ifstream::cur);

	return DateTime::TimeFromBinary(binTimeStamp);
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

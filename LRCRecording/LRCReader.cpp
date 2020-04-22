#include "stdafx.h"
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

	//DynamicHeaderInfo = new SortedDictionary<FileHeaderInfo, object>();

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
	if (uint16buff)
	{
		delete[] uint16buff;
		uint16buff = NULL;
	}

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
	PlayableSource::AllocateData();

	// free the old buffer
	if (uint16buff)
		delete[] uint16buff;

	// allocate buffer for reading unit16 data
	uint16buff = new uint16_t[cameraParams.width * cameraParams.height];
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
			char nameLength;
			fileIn.read(&nameLength, sizeof(char));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			char* nameChr = new char[nameLength + 1];
			fileIn.read(nameChr, nameLength);
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;
			string name(nameChr, nameLength);
			delete[] nameChr;
			// device data size
			uint16_t dataSize;
			fileIn.read((char*)&dataSize, sizeof(uint16_t));
			if (fileIn.fail())
				return LRC_ERR_FILE_READ;

			// create a basic device
			externalDevices.insert(pair<uint8_t, IExternalDevice*>(id, new BasicExternalDevice(name, dataSize)));
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
	fileIn.read((char*)&cameraParams, 2 * sizeof(uint16_t));	// do not use sizeof(CameraParameters), there may be more variables
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;

	// read flags indicating optional fields
	uint32_t headerFlags;
	fileIn.read((char*)&headerFlags, sizeof(uint32_t));
	if (fileIn.fail())
		return LRC_ERR_FILE_READ;
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
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
	}
	*/

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
	// the camera range is not stored anywhere now

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
	case LRCCommon::FileDataType::UInt16:
		{
		// read uint16 data
		fileIn.read((char*)uint16buff, pixelCount * sizeof(uint16_t));
		if (fileIn.fail())
			return LRC_ERR_FILE_READ;
		// decode from uint16 to actual temperatures
		double rangeLength = range.GetLength();
		for (int i = 0; i < pixelCount; i++)
			data[i] = (float)(((double)uint16buff[i] / UINT16_MAX) * rangeLength + range.min);
		}
		break;

	case LRCCommon::FileDataType::Float:
		// read float data
		fileIn.read((char*)data, pixelCount * sizeof(float));
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

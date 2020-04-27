#include "stdafx.h"
#include "LRCWriter.h"


/// <summary>
/// Creates new LRCWriter.
/// </summary>
/// <param name="streams">List of streams.</param>
/// <param name="streamCount">Number of streams.</param>
/// <param name="externalDevices">List of external devices.</param>
/// <param name="extDevCount">Number of external devices.</param>
LRCWriter::LRCWriter(LRCCommon::StreamInfo* streams, int streamCount, IExternalDevice** externalDevices, int extDevCount)
{
	// check counts
	if (streamCount == 0)
		throw LRC_ERR_NO_STREAM;
	if (streamCount > UINT8_MAX)
		throw LRC_ERR_TOO_MANY_STREAMS;
	if (externalDevices && extDevCount > UINT8_MAX)
		throw LRC_ERR_TOO_MANY_DEVICES;

	// copy streams
	this->streams = new LRCCommon::StreamInfo[streamCount];
	memcpy(this->streams, streams, streamCount * sizeof(LRCCommon::StreamInfo));
	this->streamCount = streamCount;

	// copy external devices
	this->externalDevices = new IExternalDevice*[extDevCount];
	memcpy(this->externalDevices, externalDevices, extDevCount * sizeof(IExternalDevice*));
	this->extDevCount = extDevCount;

//	DynamicHeaderInfo = new SortedDictionary<FileHeaderInfo, object>();
}

/// <summary>
/// Disposes the LRCWriter.
/// </summary>
LRCWriter::~LRCWriter()
{
	// free writing buffers
	if (floatBuff != NULL)
		delete[] floatBuff;
	if (uint16Buff != NULL)
		delete[] uint16Buff;

	// free stream size information
	if (streamSizes)
		delete[] streamSizes;
	if (streamHeaderSizes)
		delete[] streamHeaderSizes;

	// free streams and external devices
	if (streams)
		delete[] streams;
	if (externalDevices)
		delete[] externalDevices;
}

/// <summary>
/// Opens a file for recording. Appends a suffix to the file name if appropriate.
/// </summary>
/// <param name="fileName">Output file name.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::OpenFileWithSuffix(string fileName)
{
	int err;
	
	// open the file
	err = RecorderWriter::OpenFileWithSuffix(fileName);
	if (LRC_FAILED(err))
		return err;

	// write file header
	err = WriteFileHeader();
	if (LRC_FAILED(err))
		return err;

	// calculate header and stream sizes
	CalculateSizes();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Calculate block sizes from current setting.
/// </summary>
void LRCWriter::CalculateSizes()
{
	// get file header size from stream position
	fileHeaderSize = (long)fileOut.tellp();

	// calculate frame header size
	frameHeaderSize = sizeof(uint64_t);	// frame datetime
//	if (DynamicHeaderInfo.ContainsKey(FileHeaderInfo.FrameNoteLength))
//		frameHeaderSize += Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]);	// frame note

	// calculate the size of external device IDs and data
	int sumDataSize = 0;
	for (int i = 0; i < extDevCount; i++)
		sumDataSize += externalDevices[i]->GetDataSize();
	frameHeaderSize += sumDataSize + sizeof(uint8_t) * extDevCount;	// data of external devices

	streamHeaderSizes = new long[streamCount];
	streamSizes = new long[streamCount];
	long streamSizesSum = 0;
	for (int i = 0; i < streamCount; i++)
	{
		// determine data element size
		int dataTypeSize = LRCCommon::FileDataTypeSize(streams[i].dataType);

		// calculate stream header size
		streamHeaderSizes[i] =
			2 * sizeof(float) +	// stream header data range
			2 * sizeof(float) +	// stream header camera range
			8 * sizeof(float) + sizeof(uint8_t);	// stream header object params

		// calculate stream size
		streamSizes[i] =
			streamHeaderSizes[i] +	// stream header
			cameraParams.width * cameraParams.height * dataTypeSize;	// data

		// sum the stream sizes
		streamSizesSum += streamSizes[i];

		// allocate writing buffer if necessary
		if (streams[i].dataType == LRCCommon::FileDataType::Float && cameraParams.dataType == CameraDataType::UInt16 && floatBuff == NULL)
			floatBuff = new float[cameraParams.width];
		if (streams[i].dataType == LRCCommon::FileDataType::UInt16Float && cameraParams.dataType == CameraDataType::Float && uint16Buff == NULL)
			uint16Buff = new uint16_t[cameraParams.width];
	}

	// calculate frame size
	frameSize = frameHeaderSize + streamSizesSum;
}

/// <summary>
/// Writes data to first stream of next frame.
/// </summary>
/// <param name="data">Data array of frame.</param>
/// <param name="streamId">Id of stream that will be written.</param>
/// <param name="rect">Rectangle where parameter data should be written.</param>
/// <param name="dataRange">Range of data.</param>
/// <param name="dateTime">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteFrame(void* data, uint8_t streamId, Int32Rect rect, Range dataRange, uint64_t dateTime)
{
	int err;

	// overwrite frame header
	err = WriteFrameHeader(dateTime, nextFrameNote);
	if (LRC_FAILED(err))
		return err;
	// note was written in the frame header
	nextFrameNote = "";

	// seek to correct stream, write dummy stream headers
	Range defaultRange = Range();
	uint8_t i;
	for (i = 0; i < streamId; i++)
	{
		// write dummy header
		err = WriteStreamHeader(i, defaultRange);
		if (LRC_FAILED(err))
			return err;

		// skip data part of stream
		fileOut.seekp(streamSizes[i] - streamHeaderSizes[i], ofstream::cur);
	}

	// overwrite stream header
	err = WriteStreamHeader(streamId, dataRange);
	if (LRC_FAILED(err))
		return err;

	// overwrite stream data
	switch (cameraParams.dataType)
	{
	case CameraDataType::Float:
		err = WriteFrameData((float*)data, streamId, rect, dataRange);
		break;

	case CameraDataType::UInt16:
		err = WriteFrameData((uint16_t*)data, streamId, rect, dataRange);
		break;

	default:
		err = LRC_ERR_FRAME_DATA_TYPE;
		break;
	}
	if (LRC_FAILED(err))
		return err;

	// seek to next frame (WriteFrame expects stream position at start of next frame)
	for (i++; i < streamCount; i++)
	{
		// write dummy header
		err = WriteStreamHeader(i, defaultRange);
		if (LRC_FAILED(err))
			return err;

		// skip data part of stream
		fileOut.seekp(streamSizes[i] - streamHeaderSizes[i], ofstream::cur);

		if (streamSizes[i] - streamHeaderSizes[i] > 0)
		{
			// reserve space for data (for later updating)
			fileOut.seekp(-1, ofstream::cur);
			fileOut.put(0);
			if (fileOut.fail())
				return LRC_ERR_FILE_WRITE;
		}
	}

	return LRC_ERR_SUCCESS;
}

#pragma region Write methods

/// <summary>
/// Writes file header into currently open file.
/// </summary>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteFileHeader()
{
	// static part of file header
	fileOut.write(LRCCommon::FileTypeHeader, 4 * sizeof(char));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// LRC version
	fileOut.write((char*)&LRCVersion, sizeof(uint8_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// recording start time stamp
	int64_t nowBin = DateTime::TimeToBinary(DateTime::Now());
	fileOut.write((char*)&nowBin, sizeof(int64_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// external devices
	fileOut.write((char*)&extDevCount, sizeof(uint8_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;
	for (uint8_t i = 0; i < extDevCount; i++)
	{
		// device ID
		fileOut.write((char*)&i, sizeof(uint8_t));
		if (fileOut.fail())
			return LRC_ERR_FILE_WRITE;
		// device name
		string name = externalDevices[i]->GetName();
		char length = name.length();
		fileOut.write(&length, sizeof(char));
		if (fileOut.fail())
			return LRC_ERR_FILE_WRITE;
		fileOut.write(name.c_str(), length);	// should be UTF-8
		if (fileOut.fail())
			return LRC_ERR_FILE_WRITE;
		// device data size
		uint16_t dataSize = externalDevices[i]->GetDataSize();
		fileOut.write((char*)&dataSize, sizeof(uint16_t));
		if (fileOut.fail())
			return LRC_ERR_FILE_WRITE;
	}

	// number of streams in this file
	fileOut.write((char*)&streamCount, sizeof(uint8_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;
	for (int i = 0; i < streamCount; i++)
	{
		// write stream data type and units
		fileOut.write((char*)&streams[i], 2 * sizeof(uint8_t));	// do not use sizeof(StreamInfo), there are more variables
		if (fileOut.fail())
			return LRC_ERR_FILE_WRITE;
	}

	// write resolution
	uint16_t res[2] { cameraParams.width, cameraParams.height };
	fileOut.write((char*)&res, sizeof(res));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;
	// header flags
	//FileHeaderInfoFlags flags = FileHeaderInfoFlags.None;
	//foreach(var value in DynamicHeaderInfo.Keys)
	//	flags |= value.ToFlag();
	uint32_t flags = 0;
	fileOut.write((char*)&flags, sizeof(uint32_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	//// dynamic part of header
	//foreach(var pair in DynamicHeaderInfo)
	//{
	//	if (LRCFileData.StringFlags.Contains(pair.Key))
	//		fileOut.Write((string)pair.Value);
	//	else if (LRCFileData.UInt16Flags.Contains(pair.Key))
	//		fileOut.Write(Convert.ToUInt16(pair.Value));
	//	else if (LRCFileData.DateTimeFlags.Contains(pair.Key))
	//		fileOut.Write(((DateTime)pair.Value).ToBinary());
	//	else
	//		throw new NotSupportedException("Dynamic header flag " + pair.Key + " is not supported.");
	//}
	//if (fileOut.fail())
	//	return LRC_ERR_FILE_WRITE;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes frame header into currently open file.
/// </summary>
/// <param name="dateTime">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
/// <param name="note">Note for this frame.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteFrameHeader(uint64_t dateTime, string note)
{
	// frame time stamp
	int64_t nowBin = DateTime::TimeToBinary(dateTime);
	fileOut.write((char*)&nowBin, sizeof(int64_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;
	//if (DynamicHeaderInfo.ContainsKey(FileHeaderInfo.FrameNoteLength) && Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]) > 0)
	//	WriteString(note ? ? string.Empty, Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]));// write frame note if present

	// write external device values
	for (int i = 0; i < extDevCount; i++)
	{
		int err = WriteExternalDevice(i, externalDevices[i]);
		if (LRC_FAILED(err))
			return err;
	}

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes an external device into currently open file.
/// </summary>
/// <param name="devID">The device ID.</param>
/// <param name="dev">The external device.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteExternalDevice(uint8_t devID, IExternalDevice* dev)
{
	// write the device ID
	fileOut.write((char*)&devID, sizeof(uint8_t));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// write the device data
	fileOut.write(dev->GetBinaryData(), dev->GetDataSize());
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes stream header into currently open file.
/// </summary>
/// <param name="dataRange">Range of frame data.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteStreamHeader(uint8_t streamId, Range dataRange)
{
	// write data temperature range
	float dr[2] { (float)dataRange.min, (float)dataRange.max };
	fileOut.write((char*)&dr, sizeof(dr));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// write camera temperature range
	float cr[2] { cameraParams.tempRangeMinK, cameraParams.tempRangeMaxK };
	fileOut.write((char*)&cr, sizeof(cr));
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	// write object parameters
	ObjectParameters* objectParams = &streams[streamId].objectParams;
	fileOut.write((char*)objectParams, 8 * sizeof(float) + sizeof(bool));	// do not use sizeof(ObjectParameters), there are some additional constants
	if (fileOut.fail())
		return LRC_ERR_FILE_WRITE;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes float temperature data to the file.
/// </summary>
/// <param name="data">The temperature data (type float) to write.</param>
/// <param name="streamId">Id of stream that will be written.</param>
/// <param name="dataRect">Rectangle where to write this temperature array.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteFrameData(float* data, uint8_t streamId, Int32Rect dataRect, Range dataRange)
{
	int err;

	// get the size of one data element
	int valueSize = LRCCommon::FileDataTypeSize(streams[streamId].dataType);

	// write rows before rect
	err = WriteZeros(valueSize * cameraParams.width * dataRect.y);
	if (LRC_FAILED(err))
		return err;

	for (int i = 0; i < dataRect.height; i++)
	{
		// write columns before rect
		err = WriteZeros(valueSize * dataRect.x);
		if (LRC_FAILED(err))
			return err;

		// write data inside rect
		switch (streams[streamId].dataType)
		{
		case LRCCommon::FileDataType::UInt16Float:
			{
			// float saved as uint16
			// calculate the stretch from the temperature range to uint16 range
			if (dataRange.GetLength() <= 0)
				return LRC_ERR_INVALID_VALUE;
			double stretchFactor = UINT16_MAX / dataRange.GetLength();
			// stretch the current data range over uint16 range
			for (int j = 0; j < dataRect.width; j++)
				uint16Buff[j] = (uint16_t)((data[j] - dataRange.min) * stretchFactor);
			// write the buffer
			fileOut.write((char*)uint16Buff, dataRect.width * sizeof(uint16_t));
			if (fileOut.fail())
				return LRC_ERR_FILE_WRITE;
			data += dataRect.width;
			}
			break;

		case LRCCommon::FileDataType::Float:
			// float saved as float
			fileOut.write((char*)data, dataRect.width * sizeof(float));
			if (fileOut.fail())
				return LRC_ERR_FILE_WRITE;
			data += dataRect.width;
			break;

		case LRCCommon::FileDataType::UInt16Raw:
			// float cannot be saved as uint16 raw
			return LRC_ERR_FRAME_DATA_TYPE;
		}

		// write columns after rect
		err = WriteZeros(valueSize * (cameraParams.width - dataRect.width - dataRect.x));
		if (LRC_FAILED(err))
			return err;
	}

	// write rows after rect
	err = WriteZeros(valueSize * cameraParams.width * (cameraParams.height - dataRect.height - dataRect.y));
	if (LRC_FAILED(err))
		return err;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes uint16 raw data to the file.
/// </summary>
/// <param name="data">The temperature data (type uint16) to write.</param>
/// <param name="streamId">Id of stream that will be written.</param>
/// <param name="dataRect">Rectangle where to write this temperature array.</param>
/// <returns>Returns an error code.</returns>
int LRCWriter::WriteFrameData(uint16_t* data, uint8_t streamId, Int32Rect dataRect, Range dataRange)
{
	int err;

	// get the size of one data element
	int valueSize = LRCCommon::FileDataTypeSize(streams[streamId].dataType);

	// write rows before rect
	err = WriteZeros(valueSize * cameraParams.width * dataRect.y);
	if (LRC_FAILED(err))
		return err;

	for (int i = 0; i < dataRect.height; i++)
	{
		// write columns before rect
		err = WriteZeros(valueSize * dataRect.x);
		if (LRC_FAILED(err))
			return err;

		// write data inside rect
		switch (streams[streamId].dataType)
		{
		case LRCCommon::FileDataType::UInt16Float:
			// uint16 cannot be saved as "uint16 converted from float"
			return LRC_ERR_FRAME_DATA_TYPE;

		case LRCCommon::FileDataType::Float:
			// uint16 saved as float (doesn't make much sense, but make it possible)
			// copy uint16 data to a float buffer
			for (int j = 0; j < dataRect.width; j++)
				floatBuff[j] = data[j];
			// write the float buffer
			fileOut.write((char*)floatBuff, dataRect.width * sizeof(float));
			if (fileOut.fail())
				return LRC_ERR_FILE_WRITE;
			data += dataRect.width;
			break;

		case LRCCommon::FileDataType::UInt16Raw:
			// uint16 saved as uint16
			fileOut.write((char*)data, dataRect.width * sizeof(uint16_t));
			if (fileOut.fail())
				return LRC_ERR_FILE_WRITE;
			data += dataRect.width;
			break;
		}

		// write columns after rect
		err = WriteZeros(valueSize * (cameraParams.width - dataRect.width - dataRect.x));
		if (LRC_FAILED(err))
			return err;
	}

	// write rows after rect
	err = WriteZeros(valueSize * cameraParams.width * (cameraParams.height - dataRect.height - dataRect.y));
	if (LRC_FAILED(err))
		return err;

	return LRC_ERR_SUCCESS;
}

#pragma endregion

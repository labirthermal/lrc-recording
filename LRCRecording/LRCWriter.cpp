#include "stdafx.h"
#include "LRCWriter.h"


/// <summary>
/// Creates new LRCWriter.
/// </summary>
/// <param name="streams">List of streams.</param>
/// <param name="streamCount">Number of streams.</param>
/// <param name="externalDevices">List of external devices.</param>
/// <param name="extDevCount">Number of external devices.</param>
template <typename T>
LRCWriter<T>::LRCWriter(StreamInfo* streams, int streamCount, IExternalDevice** externalDevices, int extDevCount)
{
	// check counts
	if (streamCount == 0)
		throw LRC_ERR_NO_STREAM;
	if (streamCount > UINT8_MAX)
		throw LRC_ERR_TOO_MANY_STREAMS;
	if (externalDevices && extDevCount > UINT8_MAX)
		throw LRC_ERR_TOO_MANY_DEVICES;

	// copy streams
	this->streams = new StreamInfo[streamCount];
	memcpy(this->streams, streams, streamCount * sizeof(StreamInfo));
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
template <typename T>
LRCWriter<T>::~LRCWriter()
{
	if (streamSizes)
		delete streamSizes;
	if (streamHeaderSizes)
		delete streamHeaderSizes;

	if (streams)
		delete streams;
	if (externalDevices)
		delete externalDevices;
}

/// <summary>
/// Opens a file for recording. Appends a suffix to the file name if appropriate.
/// </summary>
/// <param name="fileName">Output file name.</param>
/// <returns>Returns an error code.</returns>
template <typename T>
int LRCWriter<T>::OpenFileWithSuffix(string fileName)
{
	int err = RecorderWriter::OpenFileWithSuffix(fileName);
	if (err != LRC_ERR_SUCCESS)
		return err;

	WriteFileHeader();

	CalculateSizes();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Calculate block sizes from current setting.
/// </summary>
template <typename T>
void LRCWriter<T>::CalculateSizes()
{
	// get file header size from stream position
	fileHeaderSize = fileOut.tellp();

	// calculate frame header size
	frameHeaderSize = sizeof(uint64_t);	// frame datetime
//	if (DynamicHeaderInfo.ContainsKey(FileHeaderInfo.FrameNoteLength))
//		frameHeaderSize += Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]);	// frame note
	frameHeaderSize += sizeof(uint16_t) * extDevCount;	// external device data

	streamHeaderSizes = new long[streamCount];
	streamSizes = new long[streamCount];
	long streamSizesSum = 0;
	for (int i = 0; i < streamCount; i++)
	{
		int dataTypeSize = streams[i].dataType == StreamInfo::FileDataType::UInt16 ? sizeof(uint16_t) : sizeof(float);

		// calculate stream header size
		streamHeaderSizes[i] =
			2 * sizeof(float) +	// stream header data range
			2 * sizeof(float) +	// stream header camera range
			8 * sizeof(float) + sizeof(uint8_t);	// stream header object params

		// calculate stream size
		streamSizes[i] =
			streamHeaderSizes[i] +	// stream header
			width * height * dataTypeSize;	// data

		// sum the stream sizes
		streamSizesSum += streamSizes[i];
	}

	// calculate frame size
	frameSize = frameHeaderSize + streamSizesSum;
}

/// <summary>
/// Converts time to binary UInt64 representation compatible with C#.
/// </summary>
/// <param name="time">Time in milliseconds since 1.1.1970.</param>
/// <returns>Returns the time as binary UInt64.</returns>
template <typename T>
uint64_t LRCWriter<T>::TimeToBinary(uint64_t timeMs)
{
	const uint64_t TicksPerMillisecond = 10000;
	const uint64_t UnixEpoch = 621355968000000000;
	const int64_t TicksCeiling = 0x4000000000000000;
	const uint64_t LocalMask = 0x8000000000000000;

	// convert time to ticks, offset to UNIX base time
	uint64_t ticks = timeMs * TicksPerMillisecond + UnixEpoch;

	if (ticks < 0)
		ticks += TicksCeiling;

	// set time kind as local
	return ticks | (int64_t)LocalMask;
}

/// <summary>
/// Writes data to first stream of next frame.
/// </summary>
/// <param name="data">Data array of frame.</param>
/// <param name="streamId">Id of stream that will be written.</param>
/// <param name="rect">Rectangle where parameter data should be written.</param>
/// <param name="dataRange">Range of data.</param>
/// <param name="dateTime">Frame timestamp in milliseconds since 1.1.1970.</param>
template <typename T>
void LRCWriter<T>::WriteFrame(T* data, uint8_t streamId, Int32Rect rect, Range dataRange, uint64_t dateTime)
{
	// overwrite frame header
	WriteFrameHeader(dateTime, nextFrameNote);
	nextFrameNote = "";

	Range defaultRange = Range();

	// seek to correct stream, write dummy stream headers
	uint8_t i;
	for (i = 0; i < streamId; i++)
	{
		// write dummy header
		WriteStreamHeader(i, defaultRange);

		// skip data part of stream
		fileOut.seekp(streamSizes[i] - streamHeaderSizes[i], ofstream::cur);
	}

	// if saving raw uint16 data
	if (is_same<T, uint16_t>::value)
	{
		// set the data range to whole uint16 range (to save the data as they are, no range stretching)
		dataRange.min = 0;
		dataRange.max = UINT16_MAX;
	}

	// overwrite stream header
	WriteStreamHeader(streamId, dataRange);

	// overwrite stream data
	WriteTemperatureData(data, rect, dataRange);

	// seek to next frame (WriteFrame expects stream position at start of next frame)
	for (i++; i < streamCount; i++)
	{
		// write dummy header
		WriteStreamHeader(i, defaultRange);

		// skip data part of stream
		fileOut.seekp(streamSizes[i] - streamHeaderSizes[i], ofstream::cur);

		if (streamSizes[i] - streamHeaderSizes[i] > 0)
		{
			// reserve space for data (for later updating)
			fileOut.seekp(-1, ofstream::cur);
			WriteZeros(1);
		}
	}
}

#pragma region Write methods

/// <summary>
/// Writes file header into currently open file.
/// </summary>
template <typename T>
void LRCWriter<T>::WriteFileHeader()
{
	// static part of file header
	fileOut.write(FileTypeHeader, 4 * sizeof(char));
	// recording start datetime
	uint64_t nowBin = TimeToBinary(GetTimeMs());
	fileOut.write((char*)&nowBin, sizeof(uint64_t));
	fileOut.write((char*)&extDevCount, sizeof(uint8_t));	// external devices count
	fileOut.write((char*)&streamCount, sizeof(uint8_t));	// number of streams in this file
	for (int i = 0; i < streamCount; i++)
	{
		// write stream data type
		uint8_t dataType = streams[i].dataType;
		fileOut.write((char*)&dataType, sizeof(uint8_t));
		// write stream units
		uint8_t units = streams[i].units;
		fileOut.write((char*)&units, sizeof(uint8_t));
	}
	// write resolution
	fileOut.write((char*)&width, sizeof(uint16_t));
	fileOut.write((char*)&height, sizeof(uint16_t));
	// header flags
	//FileHeaderInfoFlags flags = FileHeaderInfoFlags.None;
	//foreach(var value in DynamicHeaderInfo.Keys)
	//	flags |= value.ToFlag();
	uint32_t flags = 0;
	fileOut.write((char*)&flags, sizeof(uint32_t));

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
}

/// <summary>
/// Writes frame header into currently open file.
/// </summary>
/// <param name="note">Note for this frame.</param>
template <typename T>
void LRCWriter<T>::WriteFrameHeader(uint64_t dateTime, string note)
{
	// frame datetime
	uint64_t nowBin = TimeToBinary(dateTime);
	fileOut.write((char*)&nowBin, sizeof(uint64_t));
	//if (DynamicHeaderInfo.ContainsKey(FileHeaderInfo.FrameNoteLength) && Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]) > 0)
	//	WriteString(note ? ? string.Empty, Convert.ToUInt16(DynamicHeaderInfo[FileHeaderInfo.FrameNoteLength]));// write frame note if present

	// write external device values
	for (int i = 0; i < extDevCount; i++)
	{
		uint16_t extDevValue = externalDevices[i]->GetCurrentValue();
		fileOut.write((char*)&extDevValue, sizeof(uint16_t));
	}
}

/// <summary>
/// Writes stream header into currently open file.
/// </summary>
/// <param name="dataRange">Range of frame data.</param>
template <typename T>
void LRCWriter<T>::WriteStreamHeader(uint8_t streamId, Range dataRange)
{
	// write data temp range
	float dataRangeMin = dataRange.min;
	float dataRangeMax = dataRange.max;
	fileOut.write((char*)&dataRangeMin, sizeof(float));
	fileOut.write((char*)&dataRangeMax, sizeof(float));

	// write camera temp range
	//float camRangeMin = cameraParams.TempRangeMinK;
	//float camRangeMax = cameraParams.TempRangeMaxK;
	float camRangeMin = 0;
	float camRangeMax = 100;
	fileOut.write((char*)&camRangeMin, sizeof(float));
	fileOut.write((char*)&camRangeMax, sizeof(float));

	// write object parameters
	ObjectParameters* objectParams = &streams[streamId].objectParams;
	fileOut.write((char*)&objectParams->emissivity, sizeof(float));
	fileOut.write((char*)&objectParams->reflectedTempK, sizeof(float));
	fileOut.write((char*)&objectParams->distance, sizeof(float));
	fileOut.write((char*)&objectParams->atmosphericTempK, sizeof(float));
	fileOut.write((char*)&objectParams->relHumidity, sizeof(float));
	fileOut.write((char*)&objectParams->tau, sizeof(float));
	fileOut.write((char*)&objectParams->windowTempK, sizeof(float));
	fileOut.write((char*)&objectParams->windowTransmiss, sizeof(float));
	fileOut.write((char*)&objectParams->isTauSet, sizeof(bool));
}

/// <summary>
/// Writes float temperature data to the file.
/// </summary>
/// <param name="data">The temperature data (type float) to write.</param>
/// <param name="dataRect">Rectangle where to write this temperature array.</param>
template <>
void LRCWriter<float>::WriteTemperatureData(float* data, Int32Rect dataRect, Range dataRange)
{
	int valueSize = streams[0].dataType == StreamInfo::FileDataType::Float ? sizeof(float) : sizeof(uint16_t);

	// write rows before rect
	WriteZeros(valueSize * width * dataRect.y);

	float dataRangeLength = dataRange.GetLength();
	for (int i = 0; i < dataRect.height; i++)
	{
		// write columns before rect
		WriteZeros(valueSize * dataRect.x);

		// write data inside rect
		for (int j = 0; j < dataRect.width; j++)
		{
			switch (streams[0].dataType)
			{
			case StreamInfo::FileDataType::UInt16:
			{
				// float saved as uint16, the current data range is stretched over uint16 range
				uint16_t u = 0;
				if (dataRangeLength > 0)
					u = (uint16_t)((*data - dataRange.min) / dataRangeLength * UINT16_MAX);
				data++;
				fileOut.write((char*)&u, sizeof(uint16_t));
			}
			break;

			case StreamInfo::FileDataType::Float:
			{
				// float saved as float
				float f = *data;
				data++;
				fileOut.write((char*)&f, sizeof(float));
			}
			break;
			}
		}

		// write columns after rect
		WriteZeros(valueSize * (width - dataRect.width - dataRect.x));
	}

	// write rows after rect
	WriteZeros(valueSize * width * (height - dataRect.height - dataRect.y));
}

/// <summary>
/// Writes uint16 temperature data to the file.
/// </summary>
/// <param name="data">The temperature data (type uint16) to write.</param>
/// <param name="dataRect">Rectangle where to write this temperature array.</param>
template <>
void LRCWriter<uint16_t>::WriteTemperatureData(uint16_t* data, Int32Rect dataRect, Range dataRange)
{
	int valueSize = streams[0].dataType == StreamInfo::FileDataType::Float ? sizeof(float) : sizeof(uint16_t);

	// write rows before rect
	WriteZeros(valueSize * width * dataRect.y);

	for (int i = 0; i < dataRect.height; i++)
	{
		// write columns before rect
		WriteZeros(valueSize * dataRect.x);

		// write data inside rect
		for (int j = 0; j < dataRect.width; j++)
		{
			switch (streams[0].dataType)
			{
			case StreamInfo::FileDataType::UInt16:
			{
				// uint16 saved as uint16 (without any range stretching)
				uint16_t u = *data;
				data++;
				fileOut.write((char*)&u, sizeof(uint16_t));
			}
			break;

			case StreamInfo::FileDataType::Float:
			{
				// uint16 saved as float
				float f = *data;
				data++;
				fileOut.write((char*)&f, sizeof(float));
			}
			break;
			}
		}

		// write columns after rect
		WriteZeros(valueSize * (width - dataRect.width - dataRect.x));
	}

	// write rows after rect
	WriteZeros(valueSize * width * (height - dataRect.height - dataRect.y));
}

#pragma endregion

// explicit instantiation
template class LRCWriter<float>;
template class LRCWriter<uint16_t>;

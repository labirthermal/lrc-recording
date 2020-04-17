// Test.cpp : Defines the entry point for the console application.
// For testing the LRCWriter
//

#include "stdafx.h"
#include "LRCWriter.h"
#include "LRCReader.h"

const int Width = 160;
const int Height = 120;
const string FileName = "recording.lrc";

// the data type to test
#define DATA_TYPE_FLOAT
//#define DATA_TYPE_UINT16


/// <summary>
/// Generates random data of type float.
/// </summary>
/// <param name="buff">The preallocated destination buffer.</param>
/// <param name="count">The number of elements in the buffer.</param>
void GenerateData(float* buff, int count)
{
	const float Min = 20;
	const float Max = 35;
	const float Range = Max - Min;

	for (int i = 0; i < count; i++)
	{
		// generate a random number from 0 to 1
		float f = (float)rand() / (float)RAND_MAX;
		// convert to the desired interval
		buff[i] = Min + f * Range;
	}
}

/// <summary>
/// Generates random data of type uint16.
/// </summary>
/// <param name="buff">The preallocated destination buffer.</param>
/// <param name="count">The number of elements in the buffer.</param>
void GenerateData(uint16_t* buff, int count)
{
	const uint16_t Min = 20000;
	const uint16_t Max = 30000;
	const uint16_t Range = Max - Min;

	for (int i = 0; i < count; i++)
	{
		// generate a random number from 0 to 1
		float f = (float)rand() / (float)RAND_MAX;
		// convert to the desired interval
		buff[i] = (uint16_t)(Min + f * Range);
	}
}

/// <summary>
/// Records one image.
/// </summary>
void RecordImage()
{
	// create the stream info
	LRCCommon::StreamInfo stream;
	stream.dataType = LRCCommon::FileDataType::UInt16;	// output type
	stream.objectParams = ObjectParameters();

	// generate some data
#ifdef DATA_TYPE_FLOAT
	// temperatures in degrees Celsius
	stream.units = TemperatureSourceUnits::Celsius;
	// temperatures stored as float
	FrameDataType dataType = FrameDataType::Float;
	// generate data
	float data[Width * Height];
	GenerateData(data, Width * Height);
#endif
#ifdef DATA_TYPE_UINT16
	// raw intensities
	stream.units = TemperatureSourceUnits::Intensity;
	// intensities stored as uint16
	FrameDataType dataType = FrameDataType::UInt16;
	// generate data
	uint16_t data[Width * Height];
	GenerateData(data, Width * Height);
#endif

	// create the writer
	LRCWriter writer(&stream, 1, NULL, 0);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::None);
	writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Number);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Timestamp);

	// record one image
	int err = writer.RecordSingleFrame(data, dataType, Width, Height, FileName);
	if (LRC_FAILED(err))
		printf("Recording image failed with error %d.", err);
}

/// <summary>
/// Records several images as a sequence.
/// </summary>
/// <returns>Returns the name of the recorded file.</returns>
string RecordSequence()
{
	int err;

	// create the stream info
	LRCCommon::StreamInfo stream;
	stream.dataType = LRCCommon::FileDataType::UInt16;	// output type
	stream.objectParams = ObjectParameters();

	// create buffer for data
#ifdef DATA_TYPE_FLOAT
	// temperatures in degrees Celsius
	stream.units = TemperatureSourceUnits::Celsius;
	// temperatures stored as float
	FrameDataType dataType = FrameDataType::Float;
	float data[Width * Height];
#endif
#ifdef DATA_TYPE_UINT16
	// raw intensities
	stream.units = TemperatureSourceUnits::Intensity;
	// intensities stored as uint16
	FrameDataType dataType = FrameDataType::UInt16;
	uint16_t data[Width * Height];
#endif

	// create an external device
	BasicExternalDevice* extDevices[1];
	extDevices[0] = new BasicExternalDevice(u8"Test device", 4);
	char extDevData[4] { 0, 10, 20, 30 };
	extDevices[0]->SetBinaryData(extDevData);

	// create the writer
	LRCWriter writer(&stream, 1, (IExternalDevice**)extDevices, 1);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::None);
	writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Number);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Timestamp);

	// start recording
	err = writer.StartRecordingManually(FileName, Width, Height);
	if (LRC_FAILED(err))
	{
		printf("Recording sequence failed with error code %d.", err);
		return "";
	}

	// record several frames
	for (int i = 0; i < 5; i++)
	{
		// generate some data
		GenerateData(data, Width * Height);
		// modify external device data
		extDevData[0]++;
		extDevices[0]->SetBinaryData(extDevData);
		
		// record a frame
		err = writer.RecordFrame(data, dataType);
		if (LRC_FAILED(err))
		{
			printf("Recording frame failed with error code %d.", err);
			writer.Close();
			return "";
		}

		// wait before the next frame
		Sleep(200);
	}

	// get the recording file name
	string recordingName = writer.GetActualFileName();

	// finish recording
	writer.Close();

	return recordingName;
}

/// <summary>
/// Called when the reader reads a new frame.
/// </summary>
/// <param name="data">Frame data.</param>
/// <param name="range">Frame data range.</param>
/// <param name="timeStamp">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
/// <param name="userData">User data object. Set when registering the callback.</param>
void NewFrameCallback(float* data, Range* range, uint64_t timeStamp, void* userData)
{
	printf("Frame read: min %f, max %f\n", range->min, range->max);
}

/// <summary>
/// Application entry point.
/// Tests recording to LRC format.
/// </summary>
int main()
{
	// save an image
	printf("Saving image...\n");
	RecordImage();

	// record a sequence
	printf("Recording sequence...\n");
	string recordingName = RecordSequence();

	// open the recorded sequence
	printf("Opening recorded sequence...\n");
	LRCReader reader(recordingName);
	// register callback for processing frames
	reader.RegisterNewDataCallback(&NewFrameCallback, NULL);
	// start playback
	reader.Play();
	// wait for playback to finish
	while (reader.IsPlaying())
		Sleep(100);
	// finished
	printf("Sequence playback finished.\n");

	// manual playback: reader.NextFrame and reader.GetNextFrameTimeStamp
	// access current frame data: reader.GetTemperatureData, reader.GetDataRange, reader.GetFrameTimeStamp

    return 0;
}

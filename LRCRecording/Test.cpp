// Test.cpp : Defines the entry point for the console application.
// For testing the LRCWriter
//

#include "stdafx.h"
#include "LRCWriter.h"

const int Width = 160;
const int Height = 120;
const string FileName = "recording.lrc";

// the data type to test
typedef float TData;
//typedef uint16_t TData;


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
	StreamInfo stream;
	stream.dataType = StreamInfo::FileDataType::UInt16;	// output type
	stream.units = StreamInfo::TemperatureSourceUnits::Celsius;
	stream.objectParams = ObjectParameters();

	// generate some data
	TData data[Width * Height];
	GenerateData(data, Width * Height);

	// create the writer
	LRCWriter<TData> writer(&stream, 1, NULL, 0);
	//writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::None);
	writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::Number);
	//writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::Timestamp);

	// record one image
	int err = writer.RecordSingleFrame(data, Width, Height, FileName);
	if (LRC_FAILED(err))
		printf("Recording image failed with error %d.", err);
}

/// <summary>
/// Records several images as a sequence.
/// </summary>
void RecordSequence()
{
	int err;

	// create the stream info
	StreamInfo stream;
	stream.dataType = StreamInfo::FileDataType::UInt16;	// output type
	stream.units = StreamInfo::TemperatureSourceUnits::Celsius;
	stream.objectParams = ObjectParameters();

	// create buffer for data
	TData data[Width * Height];

	// create the writer
	LRCWriter<TData> writer(&stream, 1, NULL, 0);
	//writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::None);
	writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::Number);
	//writer.SetRecordingSuffix(RecorderWriter<TData>::RecordingSuffix::Timestamp);

	// start recording
	err = writer.StartRecordingManually(FileName, Width, Height);
	if (LRC_FAILED(err))
	{
		printf("Recording sequence failed with error code %d.", err);
		return;
	}

	// record several frames
	for (int i = 0; i < 5; i++)
	{
		// generate some data
		GenerateData(data, Width * Height);
		
		// record a frame
		err = writer.RecordFrame(data);
		if (LRC_FAILED(err))
		{
			printf("Recording frame failed with error code %d.", err);
			writer.Close();
			return;
		}

		// wait before the next frame
		Sleep(200);
	}

	// finish recording
	writer.Close();
}

/// <summary>
/// Application entry point.
/// Tests recording to LRC format.
/// </summary>
int main()
{
	RecordImage();

	RecordSequence();

    return 0;
}

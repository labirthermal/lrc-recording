// Test.cpp : Defines the entry point for the console application.
// For testing the LRCWriter
//

#include "stdafx.h"
#include "LRCWriter.h"
#include "LRCReader.h"

const uint16_t Width = 160;
const uint16_t Height = 120;
const string FileName = "recording.lrc";

// the camera (source) data type to test
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
	// create the camera parameters
	CameraParameters cameraParams;
	cameraParams.width = Width;
	cameraParams.height = Height;

	// create the stream info
	LRCCommon::StreamInfo stream;
	stream.objectParams = ObjectParameters();

#ifdef DATA_TYPE_FLOAT
	// camera source data type
	cameraParams.dataType = CameraDataType::Float;
	// file target data type
	stream.dataType = LRCCommon::FileDataType::UInt16Float;
	// temperatures in degrees Celsius
	stream.units = TemperatureSourceUnits::Celsius;
	// allocate memory for data
	float data[Width * Height];
#endif
#ifdef DATA_TYPE_UINT16
	// camera source data type
	cameraParams.dataType = CameraDataType::UInt16;
	// set the camera range as the range of uint16
	cameraParams.tempRangeMinK = 0;
	cameraParams.tempRangeMaxK = UINT16_MAX;
	// file target data type
	stream.dataType = LRCCommon::FileDataType::UInt16Raw;
	// raw intensities
	stream.units = TemperatureSourceUnits::Intensity;
	// allocate memory for data
	uint16_t data[Width * Height];
#endif

	// generate some data
	GenerateData(data, Width * Height);

	// create the writer
	LRCWriter writer(&stream, 1, NULL, 0);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::None);
	writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Number);
	//writer.SetRecordingSuffix(RecorderWriter::RecordingSuffix::Timestamp);

	// record one image
	int err = writer.RecordSingleFrame(data, cameraParams, FileName);
	if (LRC_FAILED(err))
		printf("Recording image failed with error %d.\n", err);
}

/// <summary>
/// Records several images as a sequence.
/// </summary>
/// <returns>Returns the name of the recorded file.</returns>
string RecordSequence()
{
	int err;

	// create the camera parameters
	CameraParameters cameraParams;
	cameraParams.width = Width;
	cameraParams.height = Height;

	// create the stream info
	LRCCommon::StreamInfo stream;
	stream.objectParams = ObjectParameters();

	// create buffer for data
#ifdef DATA_TYPE_FLOAT
	// camera source data type
	cameraParams.dataType = CameraDataType::Float;
	// file target data type
	stream.dataType = LRCCommon::FileDataType::UInt16Float;
	// temperatures in degrees Celsius
	stream.units = TemperatureSourceUnits::Celsius;
	// allocate memory for data
	float data[Width * Height];
#endif
#ifdef DATA_TYPE_UINT16
	// camera source data type
	cameraParams.dataType = CameraDataType::UInt16;
	// set the camera range as the range of uint16
	cameraParams.tempRangeMinK = 0;
	cameraParams.tempRangeMaxK = UINT16_MAX;
	// file target data type
	stream.dataType = LRCCommon::FileDataType::UInt16Raw;
	// raw intensities
	stream.units = TemperatureSourceUnits::Intensity;
	// allocate memory for data
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
	err = writer.StartRecordingManually(FileName, cameraParams);
	if (LRC_FAILED(err))
	{
		printf("Recording sequence failed with error code %d.\n", err);
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
		err = writer.RecordFrame(data);
		if (LRC_FAILED(err))
		{
			printf("Recording frame failed with error code %d.\n", err);
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
void NewFrameCallback(void* data, Range* range, uint64_t timeStamp, void* userData)
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

	// if the recording name is empty, no sequence was recorded
	if (recordingName.empty())
	{
		printf("No sequence recorded.\n");
		return 1;
	}

	// play back the recorded sequence
	printf("Opening recorded sequence...\n");
	LRCReader reader;
	// register callback for processing frames
	reader.RegisterNewDataCallback(&NewFrameCallback, NULL);
	// open the recording
	reader.Open(recordingName);
	// determine data type
	// the first frame has already been read by Open, perhaps register the NewFrameCallback later and re-read the first frame
	printf("Recording data type is ");
	switch (reader.GetSelectedStreamDataType())
	{
	case CameraDataType::Float:
		printf("float.\n");
		break;

	case CameraDataType::UInt16:
		printf("uint16.\n");
		break;
	}
	// start playback
	reader.Play();
	// wait for playback to finish
	while (reader.IsPlaying())
		Sleep(100);
	// check playback result
	int err = reader.GetPlaybackThreadResult();
	if (LRC_FAILED(err) && err != LRC_ERR_REC_END)
		printf("Playback failed with error code %d.\n", err);
	// finished
	reader.Close();
	printf("Sequence playback finished.\n");

	// manual playback: reader.NextFrame and reader.GetNextFrameTimeStamp
	// the functions return LRC_ERR_REC_END when trying to read after the last frame
	// access current frame data: reader.GetTemperatureData, reader.GetDataRange, reader.GetFrameTimeStamp

    return 0;
}

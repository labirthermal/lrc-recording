#include "pch.h"
#include "LRCRecordingWrapper.h"
#include "LRCReader.h"

/// <summary>
/// The LRC reader.
/// </summary>
LRCReader* reader = NULL;

/// <summary>
/// The camera parameters.
/// </summary>
CameraParameters cameraParams;

/// <summary>
/// The object parameters.
/// </summary>
ObjectParameters objectParams;

/// <summary>
/// The temperature data range.
/// </summary>
Range dataRange;

/// <summary>
/// The temperature data as floats. Used when the data are recorded as raw uint16 (not float stored as uint16).
/// </summary>
float* dataFloat = NULL;



//
// Opens a LRC recording.
//
LRCRECORDING_API_C int Open(const char* fileName)
{
	// close previous reader if any
	Close();

	reader = new LRCReader();
	return reader->Open(fileName);

	// if the temperatures are stored as uint16, allocate memory for converting them to floats
	if (reader->GetSelectedStreamDataType() == CameraDataType::UInt16)
		dataFloat = new float[reader->GetWidth() * reader->GetHeight()];

	UpdateTemperatureData();
}

//
// Closes the currently open recording.
//
LRCRECORDING_API_C void Close()
{
	if (reader != NULL)
	{
		reader->Close();
		reader = NULL;
	}

	if (dataFloat != NULL)
	{
		delete[] dataFloat;
		dataFloat = NULL;
	}
}

//
// Gets the frame width.
//
LRCRECORDING_API_C int GetWidth()
{
	return reader->GetWidth();
}

//
// Gets the frame height.
//
LRCRECORDING_API_C int GetHeight()
{
	return reader->GetHeight();
}

//
// Gets the number of frames in the recording.
//
LRCRECORDING_API_C int GetFrameCount()
{
	return reader->GetFrameCount();
}

//
// Gets the duration of the recording in milliseconds.
//
LRCRECORDING_API_C uint64_t GetDuration()
{
	return reader->GetDuration();
}

//
// Gets the length of the recording note.
//
LRCRECORDING_API_C int GetRecordingNoteLength()
{
	// if the recording has a note
	if (reader->GetHeaderFlags() & LRCCommon::FileHeaderInfoFlags::FileNote)
		return ((string*)reader->GetDynamicHeaderInfo()[LRCCommon::FileHeaderInfoFlags::FileNote])->length() + 1;	// +1 for null terminator
	else
		return 0;
}

//
/// Gets the text note associated with the recording.
//
LRCRECORDING_API_C void GetRecordingNote(char* buff, int length)
{
	// check we have a buffer
	if (buff == NULL || length <= 0)
		return;

	// if the recording has a note
	if (reader->GetHeaderFlags() & LRCCommon::FileHeaderInfoFlags::FileNote)
	{
		// copy the note to the output buffer
		string* note = (string*)reader->GetDynamicHeaderInfo()[LRCCommon::FileHeaderInfoFlags::FileNote];
		strcpy_s(buff, length, note->c_str());
	}
	else
		// no note
		buff[0] = '\0';
}

//
// Gets the camera parameters.
//
LRCRECORDING_API_C struct CameraParameters* GetCameraParameters()
{
	cameraParams = reader->GetCameraParameters();
	return &cameraParams;
}

//
// Gets the object parameters.
//
LRCRECORDING_API_C struct ObjectParameters* GetObjectParameters()
{
	objectParams = reader->GetObjectParameters();
	return &objectParams;
}

//
// Gets the index of the current frame.
//
LRCRECORDING_API_C int GetFrameIndex()
{
	return reader->GetCurrentFrameIndex();
}

//
// Updates the temperature data when a new frame is read.
//
void UpdateTemperatureData()
{
	objectParams = reader->GetObjectParameters();
	dataRange = reader->GetDataRange();

	if (reader->GetSelectedStreamDataType() == CameraDataType::UInt16)
	{
		// get the uint16 data
		uint16_t* dataUInt = (uint16_t*)reader->GetTemperatureData();
		int n = reader->GetWidth() * reader->GetHeight();

		// get the data range
		Range range = reader->GetDataRange();

		// convert the data
		for (int i = 0; i < n; i++)
			dataFloat[i] = dataUInt[i];
	}
}

//
// Gets the pointer to the temperature data.The data are a 2D matrix arranged in a row - major format.
//
LRCRECORDING_API_C float* GetTemperatureData()
{
	switch (reader->GetSelectedStreamDataType())
	{
	case CameraDataType::Float:
		return (float*)reader->GetTemperatureData();

	case CameraDataType::UInt16:
		return dataFloat;

	case CameraDataType::Unknown:
		return NULL;
	}
}

//
// Gets the temperature range of the current frame.
//
LRCRECORDING_API_C struct Range* GetDataRange()
{
	dataRange = reader->GetDataRange();
	return &dataRange;
}

//
// Gets the timestamp of the current frame.
// The timestamp is the number of milliseconds since 1.1.1970 UTC.
//
LRCRECORDING_API_C uint64_t GetFrameTimestamp()
{
	return reader->GetFrameTimeStamp();
}

//
// Gets the time in milliseconds of the current frame relative to the first frame.
//
LRCRECORDING_API_C uint64_t GetTimeFromBeginning()
{
	return reader->GetFrameTimeFromBeginning();
}

//
// Gets the maximum length of the frame note. Not all frames have notes.
//
LRCRECORDING_API_C int GetFrameNoteLength()
{
	// if frames may have notes
	if (reader->GetHeaderFlags() & LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes)
		return *(uint16_t*)reader->GetDynamicHeaderInfo()[LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes] + 1;	// +1 for null terminator
	else
		return 0;
}

//
// Gets the text note associated with the current frame.
//
LRCRECORDING_API_C void GetFrameNote(char* buff, int length)
{
	// check we have a buffer
	if (buff == NULL || length <= 0)
		return;

	// if the frame has a note
	if (reader->GetHeaderFlags() & LRCCommon::FileHeaderInfoFlags::FrameNoteLengthBytes)
	{
		string note = reader->GetFrameNote();
		strcpy_s(buff, length, note.c_str());
	}
	else
		// no note
		buff[0] = '\0';
}

//
// Seeks to a frame at the given index.
//
LRCRECORDING_API_C void Seek(int frameIndex)
{
	reader->SeekToFrame(frameIndex);
	UpdateTemperatureData();
}

//
// Rewinds to the beggining of the recording.
//
LRCRECORDING_API_C void Rewind()
{
	reader->Rewind();
	UpdateTemperatureData();
}

//
// Moves to the next frame. Does nothing if the current frame is already at the end of recording.
//
LRCRECORDING_API_C void NextFrame()
{
	reader->NextFrame();
	UpdateTemperatureData();
}

//
// Moves to the previous frame. Does nothing if the current frame is already the first one.
//
LRCRECORDING_API_C void PreviousFrame()
{
	reader->PreviousFrame();
	UpdateTemperatureData();
}

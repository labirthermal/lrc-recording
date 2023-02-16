#pragma once

#include "PlayableSource.h"
#include "LRCCommon.h"
#include "DateTime.h"

/// <summary>
/// Reads LabIR recording files.
/// </summary>
class LRCReader : public PlayableSource
{
private:
	/// <summary>
	/// The name of the source file.
	/// </summary>
	string sourceFileName;

	/// <summary>
	/// The input file.
	/// </summary>
	ifstream fileIn;

	/// <summary>
	/// Memory for float frame data.
	/// </summary>
	float* floatData = NULL;

	/// <summary>
	/// Memory for uint16 frame data.
	/// </summary>
	uint16_t* uint16Data = NULL;

	/// <summary>
	/// The note to the current frame.
	/// </summary>
	string frameNote;

	/// <summary>
	/// File header size in bytes.
	/// </summary>
	long fileHeaderSize;

	/// <summary>
	/// Frame header size in bytes.
	/// </summary>
	long frameHeaderSize;

	/// <summary>
	/// Frame size in bytes.
	/// </summary>
	long frameSize;

	/// <summary>
	/// The number of streams in the recording.
	/// </summary>
	uint8_t streamCount;

	/// <summary>
	/// Stream header sizes in bytes.
	/// </summary>
	long* streamHeaderSizes = NULL;

	/// <summary>
	/// Stream sizes in bytes.
	/// </summary>
	long* streamSizes = NULL;

	/// <summary>
	/// Information about streams.
	/// </summary>
	LRCCommon::StreamInfo* streams = NULL;

	/*
	/// <summary>
	/// Values of dynamic part of header.
	/// </summary>
	SortedDictionary<FileHeaderInfo, object> DynamicHeaderInfo{ get; set; }

	/// <summary>
	/// Flags of dynamic header content.
	/// </summary>
	FileHeaderInfoFlags HeaderFlags{ get; set; }
	*/

	/// <summary>
	/// The external devices in the LRC file. The key is the device ID.
	/// </summary>
	map<uint8_t, IExternalDevice*> externalDevices;

	/// <summary>
	/// The version of the LRC file.
	/// </summary>
	uint8_t lrcVersion;



public:
	/// <summary>
	/// Creates the LRC reader.
	/// </summary>
	LRCReader();

	/// <summary>
	/// Closes the LRC reader.
	/// </summary>
	~LRCReader();

	/// <summary>
	/// Opens a LRC file and reads the first frame.
	/// </summary>
	/// <param name="fileName">The input file name.</param>
	/// <returns>Returns an error code.</returns>
	int Open(string fileName);

	/// <summary>
	/// Closes the currently open file.
	/// </summary>
	void Close();

protected:
	/// <summary>
	/// Allocates the temperature data array according to the camera parameters.
	/// </summary>
	void AllocateData() override;

	/// <summary>
	/// Sets which data to output (which data type).
	/// </summary>
	void SetOutputData();

public:
	/// <summary>
	/// Gets the name of the source file.
	/// </summary>
	string GetSourceFileName() { return sourceFileName; }

	/// <summary>
	/// Gets the note to the current frame.
	/// </summary>
	// frame notes not supported
	//string GetFrameNote() { return frameNote; }

	/// <summary>
	/// Gets the external devices in the LRC file. Map key is the device ID.
	/// </summary>
	map<uint8_t, IExternalDevice*> GetExternalDevices() { return externalDevices; }

	/// <summary>
	/// Gets the camera data type of the currently selected stream.
	/// </summary>
	CameraDataType GetSelectedStreamDataType();

	/// <summary>
	/// Sets the selected stream index.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int SetSelectedStreamIndex(uint8_t value) override;

	/// <summary>
	/// Gets whether this reader can seek to an arbitrary frame.
	/// </summary>
	bool CanSeek() override { return true; }

	/// <summary>
	/// Gets whether this reader can step to the previous frame.
	/// </summary>
	bool CanStepBackward() override { return true; }

private:
	/// <summary>
	/// Calculates sizes of headers and block from current settings.
	/// </summary>
	void CalculateSizes();

	/// <summary>
	/// Reads data of file header.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int ReadFileHeader();

	/// <summary>
	/// Reads data of current frame header.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int ReadFrameHeader();

	/// <summary>
	/// Reads data of selected stream header.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int ReadStreamHeader();

public:
	/// <summary>
	/// Reads the current frame data.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int GetFrame() override;

	/// <summary>
	/// Reads the time stamp of the next frame and returns the file position to the beginning of that frame.
	/// </summary>
	/// <param name="timeStamp">Returns the time stamp of the next frame, or if the function fails.</param>
	/// <returns>Returns an error code.</returns>
	int GetNextFrameTimeStamp(uint64_t* timeStamp) override;

protected:
	/// <summary>
	/// Seek before first frame.
	/// </summary>
	void RewindToBeginning() override;

	/// <summary>
	/// Seek before frame of given index.
	/// </summary>
	void SeekToPositionOfFrame(int frameIndex) override;

	/// <summary>
	/// Seek before previous frame.
	/// </summary>
	void StepBackward() override;
};

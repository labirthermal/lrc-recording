// Port of LRCWriter from LabIR 1.5.14
// Simplified by removing frame updates

#pragma once

#include "pch.h"
#include "RecorderWriter.h"
#include "LRCCommon.h"

/// <summary>
/// Class for creating LabIR specific sequence files.
/// </summary>
class LRCRECORDING_API LRCWriter : public RecorderWriter
{
private:
	/// <summary>
	/// The LRC file version.
	/// </summary>
	const uint8_t LRCVersion = 2;

	/// <summary>
	/// The size of the file header.
	/// </summary>
	long fileHeaderSize;

	/// <summary>
	/// The size of one frame.
	/// </summary>
	long frameSize;

	/// <summary>
	/// Buffer for writing float data to a file.
	/// </summary>
	float* floatBuff = NULL;

	/// <summary>
	/// Buffer for writing uint16 data to a file.
	/// </summary>
	uint16_t* uint16Buff = NULL;

	/// <summary>
	/// List of streams in this file with their temperature sources and unit types.
	/// </summary>
	LRCCommon::StreamInfo* streams = NULL;
	uint8_t streamCount;

	/// <summary>
	/// List of external devices.
	/// Every time new frame is written by RecordFrame every external device will be sampled and written into frame.
	/// Not sampled by UpdateFrame.
	/// </summary>
	IExternalDevice** externalDevices = NULL;
	uint8_t extDevCount;

	/*
	/// <summary>
	/// Values of dynamic part of file header.
	/// </summary>
	SortedDictionary<FileHeaderInfo, object> DynamicHeaderInfo{ get; set; }
	*/

	/*
public:
	/// <summary>
	/// Indexer for setting values of dynamic part of file header.
	/// </summary>
	/// <param name="index">Index of file header info being set/unset.</param>
	object this[FileHeaderInfo index]
	{
		set
		{
			// if value is null, unset previous value
			if (value == null)
			{
				DynamicHeaderInfo.Remove(index);
				return;
			}

		// skip string values if string is null or empty
		if (LRCFileData.StringFlags.Contains(index))
			if (string.IsNullOrEmpty(value as string))
				return;

		// skip UInt16 values if 0
		if (LRCFileData.UInt16Flags.Contains(index))
			if ((UInt16)value == 0)
				return;

		// store value
		DynamicHeaderInfo[index] = value;
		}
	}
	*/

private:
	/// <summary>
	/// Note for next frame.
	/// </summary>
	string nextFrameNote;

public:
	/// <summary>
	/// Gets or sets the note for the next frame.
	/// </summary>
	string GetNextFrameNote() { return nextFrameNote; }
	void SetNextFrameNote(string value) { nextFrameNote = value; }

protected:
	/// <summary>
	/// Size of file header in bytes.
	/// </summary>
	long GetFileHeaderSize() override { return fileHeaderSize; };

	/// <summary>
	/// Size of frame in bytes.
	/// </summary>
	long GetFrameSize() override { return frameSize; };

private:
	/// <summary>
	/// Size of frame header in bytes.
	/// </summary>
	long frameHeaderSize;

	/// <summary>
	/// Size of streams in bytes.
	/// </summary>
	long* streamSizes = NULL;

	/// <summary>
	/// Size of stream headers in bytes.
	/// </summary>
	long* streamHeaderSizes = NULL;

public:
	/// <summary>
	/// Gets default extension of this recorder type.
	/// </summary>
	const string DefaultExtension = ".lrc";



public:
	/// <summary>
	/// Creates new LRCWriter.
	/// </summary>
	/// <param name="streams">List of streams.</param>
	/// <param name="streamCount">Number of streams.</param>
	/// <param name="externalDevices">List of pointers to external devices.</param>
	/// <param name="extDevCount">Number of external devices.</param>
	LRCWriter(LRCCommon::StreamInfo* streams, int streamCount, IExternalDevice** externalDevices = NULL, int extDevCount = 0);

	/// <summary>
	/// Disposes the LRCWriter.
	/// </summary>
	~LRCWriter();

protected:
	/// <summary>
	/// Opens a file for recording. Appends a suffix to the file name if appropriate.
	/// </summary>
	/// <param name="fileName">Output file name.</param>
	/// <returns>Returns an error code.</returns>
	int OpenFileWithSuffix(string fileName) override;

public:
	/// <summary>
	/// Sets the object parameters for the given stream.
	/// </summary>
	/// <param name="objectParams">The object parameters to set.</param>
	/// <param name="streamId">The stream for which to set the parameters.</param>
	/// <returns>Returns an error code.</returns>
	int SetObjectParams(ObjectParameters objectParams, uint8_t streamId);

	/// <summary>
	/// Closes the file being recorded.
	/// </summary>
	void Close() override;

private:
	/// <summary>
	/// Calculate block sizes from current setting.
	/// </summary>
	void CalculateSizes();

protected:
	/// <summary>
	/// Writes data to first stream of next frame.
	/// </summary>
	/// <param name="data">Data array of frame.</param>
	/// <param name="streamId">Id of stream that will be written.</param>
	/// <param name="rect">Rectangle where parameter data should be written.</param>
	/// <param name="dataRange">Range of data.</param>
	/// <param name="dateTime">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
	/// <returns>Returns an error code.</returns>
	int WriteFrame(void* data, uint8_t streamId, Int32Rect rect, Range dataRange, uint64_t dateTime) override;

#pragma region Write methods

private:
	/// <summary>
	/// Writes file header into currently open file.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int WriteFileHeader();

	/// <summary>
	/// Writes frame header into currently open file.
	/// </summary>
	/// <param name="dateTime">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
	/// <param name="note">Note for this frame.</param>
	/// <returns>Returns an error code.</returns>
	int WriteFrameHeader(uint64_t dateTime, string note);

	/// <summary>
	/// Writes an external device into currently open file.
	/// </summary>
	/// <param name="devID">The device ID.</param>
	/// <param name="dev">The external device.</param>
	/// <returns>Returns an error code.</returns>
	int WriteExternalDevice(uint8_t devID, IExternalDevice* dev);

	/// <summary>
	/// Writes stream header into currently open file.
	/// </summary>
	/// <param name="dataRange">Range of frame data.</param>
	/// <returns>Returns an error code.</returns>
	int WriteStreamHeader(uint8_t streamId, Range dataRange);

	/// <summary>
	/// Writes float temperature data to the file.
	/// </summary>
	/// <param name="data">The temperature data (in degrees Celsius) to write.</param>
	/// <param name="streamId">Id of stream that will be written.</param>
	/// <param name="dataRect">Rectangle where to write this temperature array.</param>
	/// <returns>Returns an error code.</returns>
	int WriteFrameData(float* data, uint8_t streamId, Int32Rect dataRect, Range dataRange);

	/// <summary>
	/// Writes uint16 raw data to the file.
	/// </summary>
	/// <param name="data">The temperature data (type uint16) to write.</param>
	/// <param name="streamId">Id of stream that will be written.</param>
	/// <param name="dataRect">Rectangle where to write this temperature array.</param>
	/// <returns>Returns an error code.</returns>
	int WriteFrameData(uint16_t* data, uint8_t streamId, Int32Rect dataRect, Range dataRange);

#pragma endregion
	
};

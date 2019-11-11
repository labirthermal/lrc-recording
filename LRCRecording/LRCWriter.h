// Port of LRCWriter from LabIR 1.5.14
// Simplified by removing frame updates

#pragma once

#include "RecorderWriter.h"

/// <summary>
/// Class for creating LabIR specific sequence files.
/// </summary>
class LRCWriter : public RecorderWriter
{
private:
	/// <summary>
	/// LRCReader file signature.
	/// Remember to write it including the terminating null.
	/// </summary>
	const string FileTypeHeader = "LRC";

	long fileHeaderSize;

	long frameSize;

	/// <summary>
	/// List of streams in this file with their temperature sources and unit types.
	/// </summary>
	StreamInfo* streams = NULL;
	uint8_t streamCount;

	/// <summary>
	/// List of external devices.
	/// Every time new frame is writen by RecordFrame every external device will be sampled and writen into frame.
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
	LRCWriter(StreamInfo* streams, int streamCount, IExternalDevice** externalDevices = NULL, int extDevCount = 0);

	/// <summary>
	/// Disposes the LRCWriter.
	/// </summary>
	~LRCWriter();

protected:
	/// <summary>
	/// Opens new file and writes file header.
	/// </summary>
	void OpenFileWithSuffix(string fileName) override;

private:
	/// <summary>
	/// Calculate block sizes from current setting.
	/// </summary>
	void CalculateSizes();

	/// <summary>
	/// Converts time to binary UInt64 representation compatible with C#.
	/// </summary>
	/// <param name="time">Time in milliseconds since 1.1.1970.</param>
	/// <returns>Returns the time as binary UInt64.</returns>
	uint64_t TimeToBinary(uint64_t time);

protected:
	/// <summary>
	/// Writes data to first stream of next frame.
	/// </summary>
	/// <param name="data">Data array of frame.</param>
	/// <param name="streamId">Id of stream that will be writen.</param>
	/// <param name="rect">Rectangle where parameter data should be writen.</param>
	/// <param name="dataRange">Range of data.</param>
	/// <param name="dateTime">Frame timestamp in milliseconds since 1.1.1970.</param>
	void WriteFrame(float* data, uint8_t streamId, Int32Rect rect, Range dataRange, uint64_t dateTime) override;

#pragma region Write methods

private:
	/// <summary>
	/// Writes file header into currently open file.
	/// </summary>
	void WriteFileHeader();

	/// <summary>
	/// Writes frame header into currently open file.
	/// </summary>
	/// <param name="note">Note for this frame.</param>
	void WriteFrameHeader(time_t dateTime, string note);

	/// <summary>
	/// Writes stream header into currently open file.
	/// </summary>
	/// <param name="dataRange">Range of frame data.</param>
	void WriteStreamHeader(uint8_t streamId, Range dataRange);

	/// <summary>
	/// Writes temperature data to the file.
	/// </summary>
	/// <param name="data">The temperature data (in degrees Celsius) to write.</param>
	/// <param name="dataRect">Rectangle where to write this temperature array.</param>
	void WriteTemperatureData(float* data, Int32Rect dataRect, Range dataRange);

#pragma endregion
	
};

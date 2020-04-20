// Port of RecorderWriter from LabIR 1.5.14
// Simplified for only manual recording

#pragma once

#include "LRCErrorCodes.h"
#include "DataStructures.h"
#include "DateTime.h"

using namespace std;

/// <summary>
/// Writes temperature data to a file.
/// </summary>
class RecorderWriter
{
public:
	/// <summary>
	/// The suffix to append to the recording file name.
	/// </summary>
	enum RecordingSuffix { None, Number, Timestamp };

private:
	/// <summary>
	/// The maximal allowed file size.
	/// </summary>
	long maxFileSize =  1024 * 1024 * 1024;	// 1 GB

	/// <summary>
	/// Indicates whether to split large files.
	/// </summary>
	bool doSplitLargeFiles = true;

protected:
	/// <summary>
	/// The output file stream.
	/// </summary>
	ofstream fileOut;

	/// <summary>
	/// The actual recording file name, possibly with number or time stamp.
	/// </summary>
	string actualFileName = "";

	/// <summary>
	/// The duration of the recording so far. Measured in milliseconds.
	/// </summary>
	uint64_t duration = 0;
	
	/// <summary>
	/// Index of next recorded frame.
	/// </summary>
	int nextFrameIndex = 0;

	/// <summary>
	/// The number of frames recorded so far.
	/// </summary>
	int frameCount = 0;

	/// <summary>
	/// The number of frames recorded.
	/// </summary>
	int recordedFrameCount = 0;

private:
	/// <summary>
	/// The time when the recording started. Measured as UTC in milliseconds since 1.1.1970.
	/// </summary>
	uint64_t recordingStartTime = 0;

protected:
	/// <summary>
	/// Maximum width and height of frames.
	/// </summary>
	uint16_t width = 0, height = 0;

public:
	/// <summary>
	/// Tells whether a number should be appended to the output file name
	/// so that an existing file is not overwritten.
	/// </summary>
	bool rewriteFiles = false;

	/// <summary>
	/// The style of the suffix to append to the recording file name.
	/// </summary>
	RecordingSuffix recordingSuffixStyle = RecordingSuffix::Number;

private:
	/// <summary>
	/// The original file name without suffix.
	/// </summary>
	string baseFileName = "";

	/// <summary>
	/// The base file name (without a number appended) for which the file numbering is established.
	/// </summary>
	string fileNumberingName = "";

	/// <summary>
	/// The file numbering counter, the value is the next number to append to the base file name.
	/// </summary>
	int fileNumberingNumber = 0;

	/// <summary>
	/// The separator between recording file name and number.
	/// </summary>
	const string FileNumberSeparator = "_";

	/// <summary>
	/// The format of the file number.
	/// </summary>
	const string FileNumberFormat = FileNumberSeparator + "%03d";

	/// <summary>
	/// The format of the time stamp appended to the file name, including the initial separator.
	/// </summary>
	const string TimestampFormat = " %Y-%m-%d %H-%M-%S";



public:
	/// <summary>
	/// Creates the sequence recorder.
	/// </summary>
	RecorderWriter();

	/// <summary>
	/// Disposes the recorder.
	/// </summary>
	~RecorderWriter();


#pragma region Manual recording methods

public:
	/// <summary>
	/// Records a single frame.
	/// </summary>
	/// <param name="data">The frame data.</param>
	/// <param name="dataType">The frame data type.</param>
	/// <param name="width">The frame width.</param>
	/// <param name="height">The frame height.</param>
	/// <param name="fileName">The recording file name.</param>
	/// <returns>Returns an error code.</returns>
	int RecordSingleFrame(void* data, FrameDataType dataType, uint16_t width, uint16_t height, string fileName);

	/// <summary>
	/// Opens a file for manual recording.
	/// </summary>
	/// <remarks>
	/// Use methods RecordFrame and UpdateFrame for manual recording.
	/// Call Close or Dispose method when recording is done.
	/// </remarks>
	/// <param name="fileName">Output file name.</param>
	/// <param name="width">Width of frames.</param>
	/// <param name="height">Height of frames.</param>
	/// <returns>Returns an error code.</returns>
	int StartRecordingManually(string fileName, uint16_t width, uint16_t height);

	/// <summary>
	/// Records given data into the given stream of current position in file.
	/// Use this method if writing whole new frame of completely rewriting existing frame (including header).
	/// </summary>
	/// <param name="data">Frame data.</param>
	/// <param name="dataType">The frame data type.</param>
	/// <param name="streamId">Stream ID.</param>
	/// <param name="dataRect">Rectangle where data should be written. If null, entire temperature resolution is expected. Parameter data should have save size as this rectangle.</param>
	/// <param name="range">Range of data values. If null, the range will be calculated.</param>
	/// <param name="frameTimestamp">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
	/// <returns>Returns an error code.</returns>
	int RecordFrame(void* data, FrameDataType dataType, char streamId = 0, Int32Rect dataRect = Int32Rect(), Range range = Range(), uint64_t frameTimestamp = 0);

	/// <summary>
	/// Closes the file being recorded.
	/// </summary>
	virtual void Close();

protected:
	/// <summary>
	/// Initialises the recorder.
	/// </summary>
	virtual void InitialiseRecording();
	
	/// <summary>
	/// Writes data to next frame.
	/// </summary>
	/// <remarks>
	/// Before calling this method stream position points to start of this frame.
	/// After calling this method stream position should point to start of next frame.
	///	Consider this if there are multiple streams.
	/// </remarks>
	/// <param name="data">Data array of frame.</param>
	/// <param name="dataType">The frame data type.</param>
	/// <param name="streamId">Id of stream that will be written.</param>
	/// <param name="rect">Rectangle where parameter data should be written.</param>
	/// <param name="dataRange">Range of data.</param>
	/// <param name="dateTime">Frame UTC time stamp in milliseconds since 1.1.1970.</param>
	/// <returns>Returns an error code.</returns>
	virtual int WriteFrame(void* data, FrameDataType dataType, uint8_t streamId, Int32Rect rect, Range dataRange, uint64_t dateTime) = 0;

	/// <summary>
	/// Size of file header in bytes.
	/// </summary>
	virtual long GetFileHeaderSize() = 0;

	/// <summary>
	/// Size of frame in bytes.
	/// </summary>
	virtual long GetFrameSize() = 0;

#pragma endregion

#pragma region Properties

public:
	/// <summary>
	/// Gets the maximum frame width.
	/// </summary>
	int GetWidth() { return width; }

	/// <summary>
	/// Gets the maximum frame height.
	/// </summary>
	int GetHeight() { return height; }

	/// <summary>
	/// Gets or sets the maximum recording file size before splitting into multiple files.
	/// </summary>
	long GetMaxFileSize() { return maxFileSize; }
	void SetMaxFileSize(long value) { maxFileSize = value; }

	/// <summary>
	/// Gets or sets whether to split large recordings into multiple files.
	/// </summary>
	bool GetDoSplitLargeFiles() { return doSplitLargeFiles; }
	void SetDoSplitLargeFiles(bool value) { doSplitLargeFiles = value; }

	/// <summary>
	/// Tells whether a number should be appended to the output file name
	/// so that an existing file is not overwritten.
	/// </summary>
	bool GetRewriteFiles() { return rewriteFiles; }
	void SetRewriteFiles(bool value) { rewriteFiles = value; }

	/// <summary>
	/// Gets or sets the style of the suffix to append to the recording file name.
	/// </summary>
	RecordingSuffix GetRecordingSuffix() { return recordingSuffixStyle; }
	void SetRecordingSuffix(RecordingSuffix value) { recordingSuffixStyle = value; }

	/// <summary>
	/// Gets the number of frames recorded so far.
	/// </summary>
	int GetFrameCount() { return frameCount; }

	/// <summary>
	/// Gets the duration of the recording so far. Measured in milliseconds.
	/// </summary>
	uint64_t GetDuration() { return duration; }

	/// <summary>
	/// Gets the actual output file name (a number is probably appended).
	/// </summary>
	string GetActualFileName() { return actualFileName; }

#pragma endregion

#pragma region Suffixing file names

private:
	/// <summary>
	/// Escapes special characters in a text for a regular expression.
	/// </summary>
	/// <param name="text">The text to escape.</param>
	/// <returns>Returns the text with escaped characters.</returns>
	string regex_escape(string text);

	/// <summary>
	/// Finds the next number for the recording file.
	/// </summary>
	/// <param name="fileName">The base file name without the number appended.</param>
	void FindNextRecordingNumber(string fileName);

	/// <summary>
	/// Suffixes the file name.
	/// </summary>
	/// <param name="fileName">The base file name.</param>
	/// <param name="suffixedName">Returns the suffixed file name.</param>
	/// <returns>Returns an error code.</returns>
	int SuffixFileName(string fileName, string* suffixedName);

	/// <summary>
	/// Appends a number to a file name so that such a file does not yet exist. Increments the file number counter.
	/// </summary>
	/// <param name="fileName">The base file name.</param>
	/// <returns>Returns the numbered file name.</returns>
	string AppendNumberToFileName(string fileName);

	/// <summary>
	/// Appends the current time stamp to the file name.
	/// </summary>
	/// <param name="fileName">The base file name.</param>
	/// <returns>Returns the file name with the time stamp.</returns>
	string AppendTimestampToFileName(string fileName);

#pragma endregion

protected:
	/// <summary>
	/// Determines whether a file exists.
	/// </summary>
	/// <param name="fileName">The file name.</param>
	/// <returns>Returns true if the file exists, false otherwise.</returns>
	bool FileExists(string fileName);

	/// <summary>
	/// Ensures the existence of directory tree for the given file name.
	/// </summary>
	/// <param name="fileName">The file name.</param>
	/// <returns>Returns an error code.</returns>
	int EnsureDirectory(string fileName);

	/// <summary>
	/// Opens a file for recording.
	/// </summary>
	/// <param name="fileName">The file name.</param>
	/// <returns>Returns an error code.</returns>
	int OpenFile(string fileName);

	/// <summary>
	/// Opens a file for recording. Appends a suffix to the file name if appropriate.
	/// </summary>
	/// <param name="fileName">Output file name.</param>
	/// <returns>Returns an error code.</returns>
	virtual int OpenFileWithSuffix(string fileName);

private:
	/// <summary>
	/// Checks whether there is enough space for another frame in the file.
	/// Should the file exceed the allowed size, a new file will be created.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int CheckFileSize();

protected:
	/// <summary>
	/// Updates the range of the data.
	/// </summary>
	/// <param name="data">The data.</param>
	/// <param name="length">The length of data.</param>
	/// <param name="range">The range to update.</param>
	static void UpdateDataRange(const float* data, int length, Range* range);
	/// <summary>
	/// Updates the range of the data.
	/// </summary>
	/// <param name="data">The data.</param>
	/// <param name="length">The length of data.</param>
	/// <param name="range">The range to update.</param>
	static void UpdateDataRange(const uint16_t* data, int length, Range* range);

private:
	/// <summary>
	/// Checks if written rect fit into frame and data is at least as large as rect.
	/// </summary>
	/// <param name="dataWidth">Data width.</param>
	/// <param name="dataHeight">Data height.</param>
	/// <param name="rect">Rect where data will be written.</param>
	/// <returns>True if rect fit into frame and data is at least as large as rect. False otherwise.</returns>
	bool CheckRectSize(uint16_t dataWidth, uint16_t dataHeight, Int32Rect rect);


#pragma region Auxiliary writing methods

protected:
	/// <summary>
	/// Writes zeros to the file.
	/// </summary>
	/// <param name="count">The number of bytes to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteZeros(int count);

	/// <summary>
	/// Writes a string to the file. If the total length is specified,
	/// the string is padded with zeros to reach the desired length.
	/// </summary>
	/// <param name="str">The string to write.</param>
	/// <param name="totalLength">The total number of bytes to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteString(string str, int totalLength = 0);

	/// <summary>
	/// Writes a 2-byte unsigned integer to the file in big endian.
	/// </summary>
	/// <param name="x">The value to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteUInt16BE(uint16_t x);

	/// <summary>
	/// Writes a 4-byte unsigned integer to the file in big endian.
	/// </summary>
	/// <param name="x">The value to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteUInt32BE(uint32_t x);

	/// <summary>
	/// Writes raw radiometric data to the file.
	/// </summary>
	/// <param name="data">The raw radiometric data to write.</param>
	/// <param name="count">The number of elements to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteRawData(uint16_t* data, int count);

	/// <summary>
	/// Writes raw radiometric data to the file in big endian.
	/// </summary>
	/// <param name="data">The data to write.</param>
	/// <param name="count">The number of elements to write.</param>
	/// <returns>Returns an error code.</returns>
	int WriteRawDataBE(uint16_t* data, int count);

#pragma endregion

};

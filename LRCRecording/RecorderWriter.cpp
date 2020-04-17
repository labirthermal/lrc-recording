#include "stdafx.h"
#include "RecorderWriter.h"

using namespace std;

/// <summary>
/// Creates the sequence recorder.
/// </summary>
RecorderWriter::RecorderWriter()
{
}

/// <summary>
/// Disposes the recorder.
/// </summary>
RecorderWriter::~RecorderWriter()
{
	Close();
}


#pragma region Manual recording methods

/// <summary>
/// Records a single frame.
/// </summary>
/// <param name="data">The frame data.</param>
/// <param name="dataType">The frame data type.</param>
/// <param name="width">The frame width.</param>
/// <param name="height">The frame height.</param>
/// <param name="fileName">The recording file name.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::RecordSingleFrame(void* data, FrameDataType dataType, int width, int height, string fileName)
{
	int err;

	// open the file
	err = StartRecordingManually(fileName, width, height);
	if (LRC_FAILED(err))
		return err;

	// record the frame
	err = RecordFrame(data, dataType);
	if (LRC_FAILED(err))
		return err;
	
	// done
	Close();

	return LRC_ERR_SUCCESS;
}

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
int RecorderWriter::StartRecordingManually(string fileName, int width, int height)
{
	// check recording is not already running
	if (fileOut.is_open())
		return LRC_ERR_REC_RUNNING;

	// reset recording files
	frameCount = 0;
	nextFrameIndex = 0;

	// store the resolution
	this->width = width;
	this->height = height;

	// open the output file
	int err = OpenFileWithSuffix(fileName);
	if (LRC_FAILED(err))
		return err;

	// initialise recording
	InitialiseRecording();

	return LRC_ERR_SUCCESS;
}

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
int RecorderWriter::RecordFrame(void* data, FrameDataType dataType, char streamId, Int32Rect dataRect, Range range, uint64_t frameTimestamp)
{
	int err;

	// if no data rectangle is specified, make it across the whole frame
	if (dataRect.IsEmpty())
		dataRect = Int32Rect(0, 0, width, height);
	// if no time stamp is specified, record the current time
	if (!frameTimestamp)
		frameTimestamp = DateTime::Now();

	//if (!CheckRectSize(data, dataRect))
	//	return LRC_ERR_GENERAL; // "Data rectangle doesn't fit into frame size or data is smaller than data rectangle.";

	// check a recording file is open
	if (!fileOut.is_open())
		return LRC_ERR_REC_NOT_OPEN;

	// store the starting time
	if (recordingStartTime == 0)
		recordingStartTime = frameTimestamp;

	// decide whether we are writing at the end of the file, i.e., adding a new frame
	bool isAtEnd = nextFrameIndex == frameCount;

	// split the file if it is too big
	if (isAtEnd && doSplitLargeFiles)
	{
		err = CheckFileSize();
		if (LRC_FAILED(err))
			return err;
	}

	// if no range was specified, find the data range
	if (range.IsEmpty())
	{
		switch (dataType)
		{
		case FrameDataType::Float:
			UpdateDataRange((float*)data, dataRect.width * dataRect.height, &range);
			break;

		case FrameDataType::UInt16:
			UpdateDataRange((uint16_t*)data, dataRect.width * dataRect.height, &range);
			break;

		default:
			return LRC_ERR_FRAME_DATA_TYPE;
		}
	}

	// record the frame
	err = WriteFrame(data, dataType, streamId, dataRect, range, frameTimestamp);
	if (LRC_FAILED(err))
		return err;

	if (isAtEnd)
	{
		// update the statistics
		frameCount++;
		duration = frameTimestamp - recordingStartTime;
	}
	nextFrameIndex++;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Closes the file being recorded.
/// </summary>
void RecorderWriter::Close()
{
	// close the output file
	if (fileOut.is_open())
	{
		fileOut.close();
		actualFileName = "";
	}
}

/// <summary>
/// Initialises the recorder.
/// </summary>
void RecorderWriter::InitialiseRecording()
{
	// recording started
	recordedFrameCount = 0;
	frameCount = 0;
	recordingStartTime = 0; // wait for the first frame
	duration = 0;

	// frameCount and recordedFrameCount are incremented at different times. Are both really needed?
	// frameCount is incremented in RecordFrame, recordedFrameCount is incremented when a frame is recorded or added to the queue
}

#pragma endregion


/// <summary>
/// Escapes special characters in a text for a regular expression.
/// https://stackoverflow.com/a/1253004
/// </summary>
/// <param name="text">The text to escape.</param>
/// <returns>Returns the text with escaped characters.</returns>
string RecorderWriter::regex_escape(string text)
{
	const regex esc("[.^$|()\\[\\]{}*+?\\\\]");
	const string rep("\\\\&");
	return regex_replace(text, esc, rep, regex_constants::format_sed);
}

/// <summary>
/// Finds the next number for the recording file.
/// </summary>
/// <param name="fileName">The base file name without the number appended.</param>
void RecorderWriter::FindNextRecordingNumber(string fileName)
{
	// split the file name to parts
	int lastSlash = fileName.find_last_of("\\/");
	int lastDot = fileName.find_last_of(".");
	// the dot must be in the file name, i.e., after the last slash
	if (lastDot < lastSlash)
		lastDot = string::npos;

	// get the directory name
	string dirName = ".";	// initialise to the current directory
	if (lastSlash != string::npos)
		dirName = fileName.substr(0, lastSlash);

	// get the file name without extension
	int nameStart = lastSlash == string::npos ? 0 : lastSlash + 1;
	int nameLength = lastDot == string::npos ? string::npos : lastDot - nameStart;
	string name = fileName.substr(nameStart, nameLength);

	// get the file extension
	string ext = "";
	if (lastDot != string::npos)
		ext = fileName.substr(lastDot);

	// match file names with number suffix
	regex pattern(regex_escape(name + FileNumberSeparator) + "(\\d+)" + regex_escape(ext));

	// the maximum file number
	int max = 0;

	// go through the directory
	DIR* dir = opendir(dirName.c_str());
	if (dir)
	{
		dirent* x;
		while (x = readdir(dir))
		{
			// if the item is a file
			if (x->d_type == DT_REG)
			{
				// if the regular expression matches
				cmatch fileMatch;
				if (regex_match(x->d_name, fileMatch, pattern))
				{
					// get the matched file number
					int num = stoi(fileMatch[1].str());

					// look for maximum
					if (num > max)
						max = num;
				}
			}
		}

		closedir(dir);
	}

	// set up the file numbering
	fileNumberingName = fileName;
	fileNumberingNumber = max + 1;
}

/// <summary>
/// Suffixes the file name.
/// </summary>
/// <param name="fileName">The base file name.</param>
/// <param name="suffixedName">Returns the suffixed file name.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::SuffixFileName(string fileName, string* suffixedName)
{
	// store the base file name
	baseFileName = fileName;

	switch (recordingSuffixStyle)
	{
	case RecordingSuffix::None:
		*suffixedName = fileName;
		break;

	case RecordingSuffix::Number:
		*suffixedName = AppendNumberToFileName(fileName);
		break;

	case RecordingSuffix::Timestamp:
		*suffixedName = AppendTimestampToFileName(fileName);
		break;

	default:
		return LRC_ERR_UNKNOWN_SUFFIX;
	}

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Appends a number to a file name so that such a file does not yet exist. Increments the file number counter.
/// </summary>
/// <param name="fileName">The base file name.</param>
/// <returns>Returns the numbered file name.</returns>
string RecorderWriter::AppendNumberToFileName(string fileName)
{
	// make sure the file numbering is established
	if (fileNumberingName != fileName)
		// establish file numbering
		FindNextRecordingNumber(fileName);

	// find the file extension
	int lastDot = fileName.find_last_of('.');
	if (lastDot < 0)
		lastDot = fileName.length();

	// make the numbered file name
	char formatted[16];
	snprintf(formatted, sizeof(formatted), FileNumberFormat.c_str(), fileNumberingNumber);
	string result = fileName;
	result.insert(lastDot, formatted);
	// increment the file number
	fileNumberingNumber++;

	// make sure the file does not exist
	// (someone could have created a file with the same number since the last FindNextRecordingNumber)
	while (FileExists(result))   // could be just "if" but make it "while" to be absolutely sure
	{
		// re-establish file numbering
		FindNextRecordingNumber(fileName);

		// make the numbered file name
		snprintf(formatted, sizeof(formatted), FileNumberFormat.c_str(), fileNumberingNumber);
		result = fileName;
		result.insert(lastDot, formatted);
		// increment the file number
		fileNumberingNumber++;
	}

	return result;
}

/// <summary>
/// Appends the current time stamp to the file name.
/// </summary>
/// <param name="fileName">The base file name.</param>
/// <returns>Returns the file name with the time stamp.</returns>
string RecorderWriter::AppendTimestampToFileName(string fileName)
{
	// find the file extension
	int lastDot = fileName.find_last_of('.');
	if (lastDot < 0)
		lastDot = fileName.length();

	// construct the time stamp
	time_t timeSec = DateTime::Now() / 1000;
	tm* timeLoc = localtime(&timeSec);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), TimestampFormat.c_str(), timeLoc);
	// insert the time stamp
	string result = fileName;
	result.insert(lastDot, timestamp);

	// make sure the file does not exist
	int num = 2;
	while (FileExists(result))
	{
		// insert the time stamp with an additional counter
		char timestampNum[32];
		snprintf(timestampNum, sizeof(timestampNum), "%s %d", timestamp, num);
		result = fileName;
		result.insert(lastDot, timestampNum);
		num++;
	}

	return result;
}

/// <summary>
/// Determines whether a file exists.
/// </summary>
/// <param name="fileName">The file name.</param>
/// <returns>Returns true if the file exists, false otherwise.</returns>
bool RecorderWriter::FileExists(string fileName)
{
	return ifstream(fileName).good();
}

/// <summary>
/// Ensures the existence of directory tree for the given file name.
/// Inspired by https://stackoverflow.com/a/12904145
/// </summary>
/// <param name="fileName">The file name.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::EnsureDirectory(string fileName)
{
	size_t pos = 0;
	// if the path includes Windows drive letter, skip it, it's not a directory
	if (fileName.length() > 1 && fileName[1] == ':')
		pos = 3;
	// look for directory separator chars
	while ((pos = fileName.find_first_of("\\/", pos)) != string::npos)
	{
		// get one directory from the path
		string dir = fileName.substr(0, pos++);
		// if the path starts with / or \, the first dir part is empty
		if (dir.size() > 0)
		{
			// create the directory
#ifdef WIN32
			int mdret = _mkdir(dir.c_str());
#else
			mode_t mode = 0755;
			int mdret = mkdir(dir.c_str(), mode);
#endif
			// check errors
			if (mdret && errno != EEXIST)
				return LRC_ERR_CREATE_DIR;
		}
	}

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Opens a file for recording.
/// </summary>
/// <param name="fileName">The file name.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::OpenFile(string fileName)
{
	// make sure the directory exists
	int err = EnsureDirectory(fileName);
	if (LRC_FAILED(err))
		return err;

	// open the output file and its reader/writer
	fileOut.open(fileName, ofstream::binary);
	actualFileName = fileName;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Opens a file for recording. Appends a suffix to the file name if appropriate.
/// </summary>
/// <param name="fileName">Output file name.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::OpenFileWithSuffix(string fileName)
{
	int err;

	// append a suffix to avoid overwriting existing files
	string suffixedName;
	err = SuffixFileName(fileName, &suffixedName);
	if (LRC_FAILED(err))
		return err;

	// double check the file does not exist
	if (FileExists(suffixedName))
		return LRC_ERR_FILE_EXISTS;
	
	// open the file
	err = OpenFile(suffixedName);
	if (LRC_FAILED(err))
		return err;

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Checks whether there is enough space for another frame in the file.
/// Should the file exceed the allowed size, a new file will be created.
/// </summary>
/// <returns>Returns an error code.</returns>
int RecorderWriter::CheckFileSize()
{
	// would writing another frame exceed the allowed file size?
	if ((long)fileOut.tellp() + GetFrameSize() > maxFileSize)
	{
		// close the current file
		Close();

		// suspend overwriting files
		bool wasRewrite = GetRewriteFiles();
		SetRewriteFiles(false);

		// open the new file
		int err = OpenFileWithSuffix(baseFileName);
		if (LRC_FAILED(err))
			return err;

		// restore overwriting setting
		SetRewriteFiles(wasRewrite);
	}

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Updates the range of the data.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="length">The length of data.</param>
/// <param name="range">The range to update.</param>
void RecorderWriter::UpdateDataRange(const float* data, int length, Range* range)
{
	for (int i = 0; i < length; i++)
		range->IncludeValue(data[i]);
}

/// <summary>
/// Updates the range of the data.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="length">The length of data.</param>
/// <param name="range">The range to update.</param>
void RecorderWriter::UpdateDataRange(const uint16_t* data, int length, Range* range)
{
	for (int i = 0; i < length; i++)
		range->IncludeValue(data[i]);
}

/// <summary>
/// Checks if written rect fit into frame and data is at least as large as rect.
/// </summary>
/// <param name="dataWidth">Data width.</param>
/// <param name="dataHeight">Data height.</param>
/// <param name="rect">Rect where data will be written.</param>
/// <returns>True if rect fit into frame and data is at least as large as rect. False otherwise.</returns>
bool RecorderWriter::CheckRectSize(int dataWidth, int dataHeight, Int32Rect rect)
{
	return rect.x >= 0	// rect is inside frame
		&& rect.y >= 0	// rect is inside frame
		&& rect.x + rect.width <= this->width	// rect fits inside frame
		&& rect.y + rect.height <= this->height	// rect fits inside frame
		&& dataWidth >= rect.width		// data cover entire rect
		&& dataHeight >= rect.height;	// data cover entire rect
}


#pragma region Auxiliary writing methods

/// <summary>
/// Writes zeros to the file.
/// </summary>
/// <param name="count">The number of bytes to write.</param>
void RecorderWriter::WriteZeros(int count)
{
	for (int i = 0; i < count; i++)
		fileOut.put(0);
}

/// <summary>
/// Writes a string to the file. If the total length is specified,
/// the string is padded with zeros to reach the desired length.
/// </summary>
/// <param name="str">The string to write.</param>
/// <param name="totalLength">The total number of bytes to write.</param>
/// <returns>Returns an error code.</returns>
int RecorderWriter::WriteString(string str, int totalLength)
{
	int byteCount = str.length();
	
	// check string length
	if (totalLength > 0 && byteCount > totalLength)
		return LRC_ERR_STR_TOO_LONG;

	// write the string
	fileOut << str;

	// write the padding
	int paddingBytes = totalLength - byteCount;
	if (paddingBytes > 0)
		WriteZeros(paddingBytes);

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Writes a 2-byte unsigned integer to the file in big endian.
/// </summary>
/// <param name="x">The value to write.</param>
void RecorderWriter::WriteUInt16BE(uint16_t x)
{
	fileOut.put(x >> 8);
	fileOut.put(x & 0xFF);
}

/// <summary>
/// Writes a 4-byte unsigned integer to the file in big endian.
/// </summary>
/// <param name="x">The value to write.</param>
void RecorderWriter::WriteUInt32BE(uint32_t x)
{
	fileOut.put(x >> 24);
	fileOut.put((x >> 16) & 0xFF);
	fileOut.put((x >> 8) & 0xFF);
	fileOut.put(x & 0xFF);
}

/// <summary>
/// Writes raw radiometric data to the file.
/// </summary>
/// <param name="data">The raw radiometric data to write.</param>
/// <param name="count">The number of elements to write.</param>
void RecorderWriter::WriteRawData(uint16_t* data, int count)
{
	fileOut.write((char*)data, count * sizeof(uint16_t));
}

/// <summary>
/// Writes raw radiometric data to the file in big endian.
/// </summary>
/// <param name="data">The data to write.</param>
/// <param name="count">The number of elements to write.</param>
void RecorderWriter::WriteRawDataBE(uint16_t* data, int count)
{
	for (int i = 0; i < count; i++)
		WriteUInt16BE(data[i]);
}

#pragma endregion

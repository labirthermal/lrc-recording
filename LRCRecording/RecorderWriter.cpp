#include "stdafx.h"
#include "RecorderWriter.h"

using namespace std;

/// <summary>
/// Creates the sequence recorder.
/// </summary>
template <typename T>
RecorderWriter<T>::RecorderWriter()
{
}

/// <summary>
/// Disposes the recorder.
/// </summary>
template <typename T>
RecorderWriter<T>::~RecorderWriter()
{
	Close();
}


#pragma region Manual recording methods

/// <summary>
/// Records a single frame.
/// </summary>
/// <param name="data">The frame data.</param>
/// <param name="width">The frame width.</param>
/// <param name="height">The frame height.</param>
/// <param name="fileName">The recording file name.</param>
template <typename T>
void RecorderWriter<T>::RecordSingleFrame(T* data, int width, int height, string fileName)
{
	// open the file
	StartRecordingManually(fileName, width, height);
	// record the frame
	RecordFrame(data);
	// done
	Close();
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
template <typename T>
void RecorderWriter<T>::StartRecordingManually(string fileName, int width, int height)
{
	if (fileOut.is_open())
		throw std::runtime_error("Recording already running.");

	// reset recording files
	frameCount = 0;
	nextFrameIndex = 0;

	// store the resolution
	this->width = width;
	this->height = height;

	// open the output file
	OpenFileWithSuffix(fileName);

	// initialise recording
	InitialiseRecording();
}

/// <summary>
/// Records given data into the given stream of current position in file.
/// Use this method if writing whole new frame of completely rewriting existing frame (including header).
/// </summary>
/// <param name="data">Frame data.</param>
/// <param name="streamId">Stream ID.</param>
/// <param name="dataRect">Rectangle where data should be written. If null, entire temperature resolution is expected. Parameter data should have save size as this rectangle.</param>
/// <param name="range">Range of data values. If null, the range will be calculated.</param>
/// <param name="frameTimestamp">Frame timestamp in milliseconds since 1.1.1970.</param>
template <typename T>
void RecorderWriter<T>::RecordFrame(T* data, char streamId, Int32Rect dataRect, Range range, uint64_t frameTimestamp)
{
	// if no data rectangle is specified, make it across the whole frame
	if (dataRect.IsEmpty())
		dataRect = Int32Rect(0, 0, width, height);
	// if no timestamp is specified, record the current time
	if (!frameTimestamp)
		frameTimestamp = GetTimeMs();

	//if (!CheckRectSize(data, dataRect))
	//	throw "Data rectangle doesn't fit into frame size or data is smaller than data rectangle.";

	if (!fileOut.is_open())
		throw std::runtime_error("Recording file is not opened.");

	// store the starting time
	if (recordingStartTime == 0)
		recordingStartTime = frameTimestamp;

	// decide whether we are writing at the end of the file, i.e., adding a new frame
	bool isAtEnd = nextFrameIndex == frameCount;

	// split the file if it is too big
	if (isAtEnd && doSplitLargeFiles)
		CheckFileSize();

	// update the data range
	if (range.IsEmpty())
		FindDataRange(data, dataRect.width * dataRect.height, &range);

	// record the frame
	WriteFrame(data, streamId, dataRect, range, frameTimestamp);

	if (isAtEnd)
	{
		// update the statistics
		frameCount++;
		duration = frameTimestamp - recordingStartTime;
	}
	nextFrameIndex++;
}

/// <summary>
/// Closes the file being recorded.
/// </summary>
template <typename T>
void RecorderWriter<T>::Close()
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
template <typename T>
void RecorderWriter<T>::InitialiseRecording()
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
template <typename T>
string RecorderWriter<T>::regex_escape(string text)
{
	const regex esc("[.^$|()\\[\\]{}*+?\\\\]");
	const string rep("\\\\&");
	return regex_replace(text, esc, rep, regex_constants::format_sed);
}

/// <summary>
/// Finds the next number for the recording file.
/// </summary>
/// <param name="fileName">The base file name without the number appended.</param>
template <typename T>
void RecorderWriter<T>::FindNextRecordingNumber(string fileName)
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
/// <returns>Returns the suffixed file name.</returns>
template <typename T>
string RecorderWriter<T>::SuffixFileName(string fileName)
{
	switch (GetRecordingSuffix())
	{
	case RecordingSuffix::None:
		return fileName;

	case RecordingSuffix::Number:
		return AppendNumberToFileName(fileName);

	case RecordingSuffix::Timestamp:
		return AppendTimestampToFileName(fileName);

	default:
		throw std::runtime_error("Recording suffix style is not implemented.");
	}
}

/// <summary>
/// Appends a number to a file name so that such a file does not yet exist. Increments the file number counter.
/// </summary>
/// <param name="fileName">The base file name.</param>
/// <returns>Returns the numbered file name.</returns>
template <typename T>
string RecorderWriter<T>::AppendNumberToFileName(string fileName)
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
/// Appends the current timestamp to the file name.
/// </summary>
/// <param name="fileName">The base file name.</param>
/// <returns>Returns the file name with the timestamp.</returns>
template <typename T>
string RecorderWriter<T>::AppendTimestampToFileName(string fileName)
{
	// find the file extension
	int lastDot = fileName.find_last_of('.');
	if (lastDot < 0)
		lastDot = fileName.length();

	// construct the timestamp
	time_t timeSec = GetTimeMs() / 1000;
	tm* timeLoc = localtime(&timeSec);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), TimestampFormat.c_str(), timeLoc);
	// insert the timestamp
	string result = fileName;
	result.insert(lastDot, timestamp);

	// make sure the file does not exist
	int num = 2;
	while (FileExists(result))
	{
		// insert the timestamp with an additional counter
		char timestampNum[32];
		snprintf(timestampNum, sizeof(timestampNum), "%s %d", timestamp, num);
		result = fileName;
		result.insert(lastDot, timestampNum);
		num++;
	}

	return result;
}

/// <summary>
/// Gets the current local time in milliseconds since 1.1.1970.
/// </summary>
/// <returns>Returns the current local time in milliseconds.</returns>
template <typename T>
uint64_t RecorderWriter<T>::GetTimeMs()
{
	long long milliseconds;

#ifdef WIN32
	timeb t;
	ftime(&t);
	milliseconds = (long long)t.time * 1000L + t.millitm;
#else
	timeval t;
	gettimeofday(&t, NULL);
	milliseconds = (long long)t.tv_sec * 1000L + t.tv_usec / 1000;
#endif

	return milliseconds;
}

/// <summary>
/// Determines whether a file exists.
/// </summary>
/// <param name="fileName">The file name.</param>
/// <returns>Returns true if the file exists, false otherwise.</returns>
template <typename T>
bool RecorderWriter<T>::FileExists(string fileName)
{
	return ifstream(fileName).good();
}

/// <summary>
/// Ensures the existence of directory tree for the given file name.
/// Inspired by https://stackoverflow.com/a/12904145
/// </summary>
/// <param name="fileName">The file name.</param>
template <typename T>
void RecorderWriter<T>::EnsureDirectory(string fileName)
{
	size_t pos = 0;
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
				throw std::runtime_error("Cannot create directory tree.");
		}
	}
}

/// <summary>
/// Opens a file for recording.
/// </summary>
/// <param name="fileName">The file name.</param>
template <typename T>
void RecorderWriter<T>::OpenFile(string fileName)
{
	// make sure the directory exists
	EnsureDirectory(fileName);

	// open the output file and its reader/writer
	fileOut.open(fileName, ofstream::binary);
	actualFileName = fileName;
}

/// <summary>
/// Opens a file for recording. Appends a suffix to the file name if appropriate.
/// </summary>
/// <param name="fileName">Output file name.</param>
template <typename T>
void RecorderWriter<T>::OpenFileWithSuffix(string fileName)
{
	// append a suffix to avoid overwriting existing files
	fileName = SuffixFileName(fileName);

	// double check the file does not exist
	if (FileExists(fileName))
		throw std::runtime_error("The file \"" + fileName + "\" already exists.");
	
	// open the file
	OpenFile(fileName);
}

/// <summary>
/// Checks whether there is enough space for another frame in the file.
/// Should the file exceed the allowed size, a new file will be created.
/// </summary>
template <typename T>
void RecorderWriter<T>::CheckFileSize()
{
	// would writing another frame exceed the allowed file size?
	if ((long)fileOut.tellp() + GetFrameSize() > MaxFileSize)
	{
		// store output file name before closing it
		string fileName = GetActualFileName();

		// close the current file
		Close();

		// get unnumbered file name
		if (!GetRewriteFiles())
		{
			// find the number delimiters
			int underscore = fileName.find_last_of('_');
			int dot = fileName.find_last_of('.');
			// strip the number from the file name
			if (underscore > 0 && dot > underscore)
				fileName = fileName.erase(underscore, dot - underscore);
		}

		// suspend overwriting files
		bool wasRewrite = GetRewriteFiles();
		SetRewriteFiles(false);
		// open the new file
		OpenFileWithSuffix(fileName);
		// initialise recording to the new file
		InitialiseRecording();
		// restore overwriting setting
		SetRewriteFiles(wasRewrite);
	}
}

/// <summary>
/// Finds the range of the data.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="length">The length of data.</param>
/// <param name="range">The range to store the result.</param>
template <typename T>
void RecorderWriter<T>::FindDataRange(const T* data, int length, Range* range)
{
	range->ResetActualRange();

	UpdateDataRange(data, length, range);
}

/// <summary>
/// Updates the range of the data.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="length">The length of data.</param>
/// <param name="range">The range to update.</param>
template <typename T>
void RecorderWriter<T>::UpdateDataRange(const T* data, int length, Range* range)
{
	for (int i = 0; i < length; i++)
	{
		range->IncludeValue(*data);
		data++;
	}
}

/// <summary>
/// Checks if written rect fit into frame and data is at least as large as rect.
/// </summary>
/// <param name="dataWidth">Data width.</param>
/// <param name="dataHeight">Data height.</param>
/// <param name="rect">Rect where data will be written.</param>
/// <returns>True if rect fit into frame and data is at least as large as rect. False otherwise.</returns>
template <typename T>
bool RecorderWriter<T>::CheckRectSize(int dataWidth, int dataHeight, Int32Rect rect)
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
template <typename T>
void RecorderWriter<T>::WriteZeros(int count)
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
template <typename T>
void RecorderWriter<T>::WriteString(string str, int totalLength)
{
	int byteCount = str.length();
	
	// check string length
	if (totalLength > 0 && byteCount > totalLength)
		throw std::runtime_error("The string is too long. It does not fit the desired length.");

	// write the string
	fileOut << str;

	// write the padding
	int paddingBytes = totalLength - byteCount;
	if (paddingBytes > 0)
		WriteZeros(paddingBytes);
}

/// <summary>
/// Writes a 2-byte unsigned integer to the file in big endian.
/// </summary>
/// <param name="x">The value to write.</param>
template <typename T>
void RecorderWriter<T>::WriteUInt16BE(uint16_t x)
{
	fileOut.put(x >> 8);
	fileOut.put(x & 0xFF);
}

/// <summary>
/// Writes a 4-byte unsigned integer to the file in big endian.
/// </summary>
/// <param name="x">The value to write.</param>
template <typename T>
void RecorderWriter<T>::WriteUInt32BE(uint32_t x)
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
template <typename T>
void RecorderWriter<T>::WriteRawData(uint16_t* data, int count)
{
	fileOut.write((char*)data, count * sizeof(data[0]));
}

/// <summary>
/// Writes raw radiometric data to the file in big endian.
/// </summary>
/// <param name="data">The data to write.</param>
/// <param name="count">The number of elements to write.</param>
template <typename T>
void RecorderWriter<T>::WriteRawDataBE(uint16_t* data, int count)
{
	for (int i = 0; i < count; i++)
		WriteUInt16BE(data[i]);
}

#pragma endregion

// explicit instantiation
template class RecorderWriter<float>;
template class RecorderWriter<uint16_t>;

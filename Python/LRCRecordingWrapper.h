#pragma once

#include "DataStructures.h"

#define LRCRECORDING_API_C extern __declspec(dllimport)
#define uint64_t unsigned long long

/// <summary>
/// Opens a LRC recording.
/// </summary>
/// <param name="fileName">Recording file name.</param>
/// <returns>Returns 0 on success.</returns>
LRCRECORDING_API_C int Open(const char* fileName);

/// <summary>
/// Closes the currently open recording.
/// </summary>
LRCRECORDING_API_C void Close();

/// <summary>
/// Gets the frame width.
/// </summary>
LRCRECORDING_API_C int GetWidth();

/// <summary>
/// Gets the frame height.
/// </summary>
LRCRECORDING_API_C int GetHeight();

/// <summary>
/// Gets the number of frames in the recording.
/// </summary>
LRCRECORDING_API_C int GetFrameCount();

/// <summary>
/// Gets the duration of the recording in milliseconds.
/// </summary>
LRCRECORDING_API_C uint64_t GetDuration();

/// <summary>
/// Gets the length of the recording note.
/// </summary>
/// <returns>Returns the length in bytes, including the null terminating character.</returns>
LRCRECORDING_API_C int GetRecordingNoteLength();

/// <summary>
/// Gets the text note associated with the recording.
/// </summary>
/// <param name="buff">A buffer to receive the text.</param>
/// <param name="length">The buffer length.</param>
LRCRECORDING_API_C void GetRecordingNote(char* buff, int length);

/// <summary>
/// Gets the object parameters.
/// </summary>
/// <returns>Returns the object parameters.</returns>
/// <remarks>MATLAB doesn't support struct as a return type so it must be a pointer.</remarks>
LRCRECORDING_API_C struct ObjectParameters* GetObjectParameters();

/// <summary>
/// Gets the index of the current frame.
/// </summary>
LRCRECORDING_API_C int GetFrameIndex();

/// <summary>
/// Gets the pointer to the temperature data. The data are a 2D matrix arranged in a row-major format.
/// </summary>
/// <remarks>You need to call this function only once. Then just call NextFrame and the data will be updated.</remarks>
LRCRECORDING_API_C float* GetTemperatureData();

/// <summary>
/// Gets the temperature range of the current frame.
/// </summary>
/// <returns>Returns the temperature range.</returns>
/// <remarks>MATLAB doesn't support struct as a return type so it must be a pointer.</remarks>
LRCRECORDING_API_C struct Range* GetDataRange();

/// <summary>
/// Gets the timestamp of the current frame.
/// The timestamp is the number of milliseconds since 1.1.1970 UTC.
/// </summary>
LRCRECORDING_API_C uint64_t GetFrameTimestamp();

/// <summary>
/// Gets the time in milliseconds of the current frame relative to the first frame.
/// </summary>
LRCRECORDING_API_C uint64_t GetTimeFromBeginning();

/// <summary>
/// Gets the maximum length of the frame note. Not all frames have notes.
/// </summary>
/// <returns>Returns the length in bytes, including the null terminating character.</returns>
LRCRECORDING_API_C int GetFrameNoteLength();

/// <summary>
/// Gets the text note associated with the current frame.
/// </summary>
/// <param name="buff">A buffer to receive the text.</param>
/// <param name="length">The buffer length.</param>
LRCRECORDING_API_C void GetFrameNote(char* buff, int length);

/// <summary>
/// Seeks to a frame at the given index.
/// </summary>
/// <param name="frameIndex">The frame index.</param>
LRCRECORDING_API_C void Seek(int frameIndex);

/// <summary>
/// Rewinds to the beggining of the recording.
/// </summary>
LRCRECORDING_API_C void Rewind();

/// <summary>
/// Moves to the next frame. Does nothing if the current frame is already at the end of recording.
/// </summary>
LRCRECORDING_API_C void NextFrame();

/// <summary>
/// Moves to the previous frame. Does nothing if the current frame is already the first one.
/// </summary>
LRCRECORDING_API_C void PreviousFrame();

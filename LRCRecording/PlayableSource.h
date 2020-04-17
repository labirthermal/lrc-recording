#pragma once

#include "OfflineSource.h"

/// <summary>
/// Temperature source capable of data playback.
/// </summary>
class PlayableSource : public OfflineSource
{
protected:
	/// <summary>
	/// The name of the source file.
	/// </summary>
	string sourceFileName;

	/// <summary>
	/// The number of frames available for playback.
	/// </summary>
	int frameCount;

	/// <summary>
	/// Index of the current frame.
	/// </summary>
	int currentFrameIndex;

	/// <summary>
	/// The time stamp of the first frame.
	/// </summary>
	uint64_t firstFrameTimeStamp;

	/// <summary>
	/// The duration of the frames available to play.
	/// </summary>
	uint64_t duration;

private:
	/// <summary>
	/// Indicates whether the playable source is currently playing.
	/// </summary>
	bool isPlaying;

	/// <summary>
	/// Time stamp when the next frame should be played.
	/// </summary>
	chrono::steady_clock::time_point nextFramePlayTime;

	/// <summary>
	/// A mutex for synchronising manual and automatic playback.
	/// </summary>
	mutex playbackMutex;

	/// <summary>
	/// A thread for playing the frames.
	/// </summary>
	thread playbackThread;

	/// <summary>
	/// The error code from the playback thread.
	/// </summary>
	int playbackThreadResult;

	/// <summary>
	/// A mutex for the scheduling lock for the scheduling condition variable.
	/// </summary>
	mutex schedulingMutex;

	/// <summary>
	/// A condition variable used in the playback thread for waiting for the next frame.
	/// </summary>
	condition_variable schedulingCondVar;



public:
	/// <summary>
	/// Initialises a playable source.
	/// </summary>
	/// <param name="sourceFileName">The name of the source file.</param>
	PlayableSource(string sourceFileName);

	/// <summary>
	/// Disposes the playable source.
	/// </summary>
	~PlayableSource();

	/// <summary>
	/// Gets the flag indicating whether the temperature source can move to the previous frame.
	/// </summary>
	virtual bool CanStepBackward() = 0;

	/// <summary>
	/// Gets the flag indication whether the temperature source can seek to any position in the file.
	/// </summary>
	virtual bool CanSeek() = 0;

	/// <summary>
	/// Gets the name of the source file.
	/// </summary>
	string GetSourceFileName() { return sourceFileName; }

	/// <summary>
	/// Gets the number of frames available for playback.
	/// </summary>
	int GetFrameCount() { return frameCount; }

	/// <summary>
	/// Gets the total duration of the frames available to play.
	/// </summary>
	uint64_t GetDuration() { return duration; }

	/// <summary>
	/// Gets the frame time relative to the first frame.
	/// </summary>
	uint64_t GetFrameTimeFromBeginning() { return frameTimeStamp - firstFrameTimeStamp; }

	/// <summary>
	/// Gets the zero-based index of the current frame.
	/// During GetFrame method this index means what frame index should be retrieved.
	/// </summary>
	int GetCurrentFrameIndex() { return currentFrameIndex; }

	/// <summary>
	/// Gets the current frame number. Frame numbers are counted from one.
	/// </summary>
	int GetCurrentFrameNumberFromOne() { return currentFrameIndex + 1; }

	/// <summary>
	/// Gets whether the playable source is currently playing.
	/// </summary>
	bool IsPlaying() { return isPlaying; }

protected:
	/// <summary>
	/// Gets a frame from the source (camera, file).
	/// </summary>
	/// <returns>Returns an error code.</returns>
	virtual int GetFrame() = 0;

public:
	/// <summary>
	/// Starts the playback. Can also be used to change the playback speed.
	/// </summary>
	void Play();

	/// <summary>
	/// Pauses the playback.
	/// </summary>
	void Pause();

	/// <summary>
	/// Gets the playback thread error code.
	/// </summary>
	int GetPlaybackThreadResult() { return playbackThreadResult; }

	/// <summary>
	/// Advances to the next frame and reads it.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int NextFrame();

	/// <summary>
	/// Rewinds to the beginning and reads the first frame.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int Rewind();

	/// <summary>
	/// Rewinds to the previous frame and reads it.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int PreviousFrame();

	/// <summary>
	/// Seeks to the given frame and reads it.
	/// </summary>
	/// <param name="frameIndex">The zero-based frame index.</param>
	/// <returns>Returns an error code.</returns>
	int SeekToFrame(int frameIndex);

protected:
	/// <summary>
	/// Moves to the beginning of the first frame.
	/// </summary>
	virtual void RewindToBeginning() = 0;

	/// <summary>
	/// Moves to the beginning of the previous frame.
	/// </summary>
	virtual void StepBackward() = 0;

	/// <summary>
	/// Seeks to the given frame.
	/// </summary>
	/// <param name="frameIndex">The zero-based frame index.</param>
	virtual void SeekToPositionOfFrame(int frameIndex) = 0;

public:
	/// <summary>
	/// Reads the time stamp of the next frame and returns the file position to the beginning of that frame.
	/// </summary>
	/// <returns>Returns the time stamp of the next frame, or zero if there are no more frames available.</returns>
	virtual uint64_t GetNextFrameTimeStamp() = 0;

private:
	/// <summary>
	/// Plays the playable source. Should be run in a thread.
	/// </summary>
	void PlaybackThread();

	/// <summary>
	/// Sets up a timer to read the next frame to play the video.
	/// </summary>
	void ScheduleNextFrame();
};


#include "stdafx.h"
#include "PlayableSource.h"

/// <summary>
/// Initialises a playable source.
/// </summary>
/// <param name="sourceFileName">The name of the source file.</param>
PlayableSource::PlayableSource(string sourceFileName)
{
	// initialise members
	frameCount = currentFrameIndex = 0;
	firstFrameTimeStamp = 0;
	duration = 0;
	isPlaying = false;

	// store the source file name
	this->sourceFileName = sourceFileName;
}

/// <summary>
/// Disposes the playable source.
/// </summary>
PlayableSource::~PlayableSource()
{
	// stop playback
	Pause();
}

/// <summary>
/// Starts the playback. Can also be used to change the playback speed.
/// </summary>
void PlayableSource::Play()
{
	// enable playback
	isPlaying = true;

	// schedule reading a frame right now
	nextFramePlayTime = chrono::steady_clock::now();

	// run the playback thread
	playbackThread = thread(&PlayableSource::PlaybackThread, this);
}

/// <summary>
/// Pauses the playback.
/// </summary>
void PlayableSource::Pause()
{
	// disable playback
	isPlaying = false;
	// do not join the thread, it could take too long for slow frame rates
	if (playbackThread.get_id() != thread::id::id())
		playbackThread.detach();
}

/// <summary>
/// Advances to the next frame and reads it.
/// </summary>
void PlayableSource::NextFrame()
{
	playbackMutex.lock();
	
	// do not read beyond the end
	bool isNewFrame = false;
	if (currentFrameIndex < frameCount - 1)
	{
		isNewFrame = true;

		// read a frame
		currentFrameIndex++;
		GetFrame();
	}

	playbackMutex.unlock();

	// inform the base class that new data have been received
	if (isNewFrame)
		DataReceived();
}

/// <summary>
/// Rewinds to the beginning and reads the first frame.
/// </summary>
void PlayableSource::Rewind()
{
	playbackMutex.lock();

	// rewind
	RewindToBeginning();

	// read the first frame
	currentFrameIndex = 0;
	GetFrame();
	
	playbackMutex.unlock();

	// inform the base class that new data have been received
	DataReceived();
}

/// <summary>
/// Rewinds to the previous frame and reads it.
/// </summary>
void PlayableSource::PreviousFrame()
{
	// make sure we can step backward
	if (!CanStepBackward())
		throw runtime_error("This PlayableSource does not support stepping backwards.");

	playbackMutex.lock();

	// do not go before the beginning
	bool isNewFrame = false;
	if (currentFrameIndex > 0)
	{
		isNewFrame = true;

		// rewind
		StepBackward();

		// read the frame
		currentFrameIndex--;
		GetFrame();
	}
	
	playbackMutex.unlock();

	// inform the base class that new data have been received
	if (isNewFrame)
		DataReceived();
}

/// <summary>
/// Seeks to the given frame and reads it.
/// </summary>
/// <param name="frameIndex">The zero-based frame index.</param>
void PlayableSource::SeekToFrame(int frameIndex)
{
	// make sure we can seek
	if (!CanSeek())
		throw runtime_error("This PlayableSource does not support seeking.");

	// check bounds
	if (frameIndex < 0 || frameIndex >= frameCount)
		throw runtime_error("Frame index out of range.");
	// NextFrame and PreviousFrame consume errors because it's quite normal to attempt
	// to read one frame over the bound. But here the exact index should be correct.

	playbackMutex.lock();

	// seek to the frame
	SeekToPositionOfFrame(frameIndex);

	// read the frame
	currentFrameIndex = frameIndex;
	GetFrame();
	
	playbackMutex.unlock();

	// inform the base class that new data have been received
	DataReceived();
}

/// <summary>
/// Plays the playable source. Should be run in a thread.
/// </summary>
void PlayableSource::PlaybackThread()
{
	// while playback is on
	while (isPlaying)
	{
		playbackMutex.lock();

		// read a frame
		currentFrameIndex++;
		GetFrame();

		playbackMutex.unlock();

		// inform the base class that new data have been received
		DataReceived();

		// wait until the next frame
		ScheduleNextFrame();
		this_thread::sleep_until(nextFramePlayTime);
	}
}

/// <summary>
/// Sets up a timer to read the next frame to play the video.
/// </summary>
void PlayableSource::ScheduleNextFrame()
{
	playbackMutex.lock();

	// check there are frames available
	if (currentFrameIndex == frameCount - 1)
		// no, we are at the end, stop playing
		Pause();
	else
	{
		// calculate the time to the next frame
		uint64_t timeToNextFrame = GetNextFrameTimeStamp() - frameTimeStamp;
		
		// store the time when the next frame should be played
		nextFramePlayTime += chrono::milliseconds(timeToNextFrame);

		// get the current time
		chrono::steady_clock::time_point now = chrono::steady_clock::now();
		// if the next frame should have already been played, schedule it for now
		// (for example when the disk gets slow, frames would be scheduled in the past,
		// then the disk gets fast and several frames would be played rapidly without delay)
		if (nextFramePlayTime < now)
			nextFramePlayTime = now;
	}
	
	playbackMutex.unlock();
}

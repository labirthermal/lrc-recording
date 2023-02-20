#include "pch.h"
#include "PlayableSource.h"

/// <summary>
/// Initialises a playable source.
/// </summary>
PlayableSource::PlayableSource()
{
	// initialise members
	frameCount = currentFrameIndex = 0;
	firstFrameTimeStamp = 0;
	duration = 0;
	isPlaying = false;
	playbackThreadResult = LRC_ERR_SUCCESS;
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
	// wake up the playback thread
	schedulingCondVar.notify_all();

	// if the playback thread exists and it is not this thread
	if (playbackThread.joinable() && playbackThread.get_id() != this_thread::get_id())
		playbackThread.join();
}

/// <summary>
/// Advances to the next frame and reads it.
/// </summary>
/// <returns>Returns an error code.</returns>
int PlayableSource::NextFrame()
{
	playbackMutex.lock();
	
	// do not read beyond the end
	int err;
	if (currentFrameIndex < frameCount - 1)
	{
		// read a frame
		currentFrameIndex++;
		err = GetFrame();
	}
	else
		// end of recording
		err = LRC_ERR_REC_END;

	playbackMutex.unlock();

	// check result
	if (LRC_FAILED(err))
		return err;

	// inform the base class that new data have been received
	DataReceived();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Rewinds to the beginning and reads the first frame.
/// </summary>
/// <returns>Returns an error code.</returns>
int PlayableSource::Rewind()
{
	playbackMutex.lock();

	// rewind
	RewindToBeginning();

	// read the first frame
	currentFrameIndex = 0;
	int err = GetFrame();

	playbackMutex.unlock();

	// check GetFrame result
	if (LRC_FAILED(err))
		return err;

	// inform the base class that new data have been received
	DataReceived();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Rewinds to the previous frame and reads it.
/// </summary>
/// <returns>Returns an error code.</returns>
int PlayableSource::PreviousFrame()
{
	// make sure we can step backward
	if (!CanStepBackward())
		return LRC_ERR_CANNOT_STEP_BACK;

	playbackMutex.lock();

	// do not go before the beginning
	bool isNewFrame = false;
	int err = LRC_ERR_SUCCESS;
	if (currentFrameIndex > 0)
	{
		isNewFrame = true;

		// rewind
		StepBackward();

		// read the frame
		currentFrameIndex--;
		err = GetFrame();
	}
	
	playbackMutex.unlock();

	// check GetFrame result
	if (LRC_FAILED(err))
		return err;

	// inform the base class that new data have been received
	if (isNewFrame)
		DataReceived();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Seeks to the given frame and reads it.
/// </summary>
/// <param name="frameIndex">The zero-based frame index.</param>
/// <returns>Returns an error code.</returns>
int PlayableSource::SeekToFrame(int frameIndex)
{
	// make sure we can seek
	if (!CanSeek())
		return LRC_ERR_CANNOT_SEEK;

	// check bounds
	if (frameIndex < 0 || frameIndex >= frameCount)
		return LRC_ERR_FRAME_INDEX_OUT;
	// NextFrame and PreviousFrame consume errors because it's quite normal to attempt
	// to read one frame over the bound. But here the exact index should be correct.

	playbackMutex.lock();

	// seek to the frame
	SeekToPositionOfFrame(frameIndex);

	// read the frame
	currentFrameIndex = frameIndex;
	int err = GetFrame();
	
	playbackMutex.unlock();

	// check GetFrame result
	if (LRC_FAILED(err))
		return err;

	// inform the base class that new data have been received
	DataReceived();

	return LRC_ERR_SUCCESS;
}

/// <summary>
/// Plays the playable source. Should be run in a thread.
/// </summary>
void PlayableSource::PlaybackThread()
{
	int err;

	// reset error code
	playbackThreadResult = LRC_ERR_SUCCESS;

	// create a lock for the condition variable
	unique_lock<mutex> schedulingLock(schedulingMutex);

	// while playback is on
	while (isPlaying)
	{
		// play a frame
		err = NextFrame();
		if (LRC_FAILED(err))
		{
			// error, stop the playback
			Pause();
			playbackThreadResult = err;
			return;
		}

		// schedule the next frame
		err = ScheduleNextFrame();
		if (LRC_FAILED(err))
		{
			// end of recording or error, stop the playback
			Pause();
			playbackThreadResult = err;
			return;
		}

		// wait until the next frame
		schedulingCondVar.wait_until(schedulingLock, nextFramePlayTime,
			// filter out spurious wake ups
			[&] { return chrono::steady_clock::now() >= nextFramePlayTime; });
	}

	// do not set error code to success here, we might got here from some error so leave the result as is
}

/// <summary>
/// Sets up a timer to read the next frame to play the video.
/// </summary>
/// <returns>Returns an error code.</returns>
int PlayableSource::ScheduleNextFrame()
{
	playbackMutex.lock();

	// get the next frame time stamp
	uint64_t nextFrameTimeStamp;
	int err = GetNextFrameTimeStamp(&nextFrameTimeStamp);
	if (LRC_FAILED(err))
	{
		// end of recording or error
		playbackMutex.unlock();
		return err;
	}
	// calculate the time to the next frame (measured from the previous frame, not from now)
	uint64_t timeToNextFrame = nextFrameTimeStamp - frameTimeStamp;
		
	// store the time when the next frame should be played
	nextFramePlayTime += chrono::milliseconds(timeToNextFrame);

	// get the current time
	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	// if the next frame should have already been played, schedule it for now
	// (for example when the disk gets slow, frames would be scheduled in the past,
	// then the disk gets fast and several frames would be played rapidly without delay)
	if (nextFramePlayTime < now)
		nextFramePlayTime = now;
	
	playbackMutex.unlock();
	return LRC_ERR_SUCCESS;
}

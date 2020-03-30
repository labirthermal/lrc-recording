#pragma once

// No error, operation successful
#define LRC_ERR_SUCCESS			0


// Checks whether the given error code indicates success
#define LRC_SUCCEEDED(e)	((e) == LRC_ERR_SUCCESS)

// Checks whether the given error code indicates an error
#define LRC_FAILED(e)		((e) != LRC_ERR_SUCCESS)


// General error, when there is no more specific information
#define LRC_ERR_GENERAL			1

// Cannot start recording, it is already running
#define LRC_ERR_REC_RUNNING		2

// Cannot record frame, recording file is not open
#define LRC_ERR_REC_NOT_OPEN	3

// Unknown recording file suffix (number, time stamp)
#define LRC_ERR_UNKNOWN_SUFFIX	4

// Cannot create directory tree for recording file
#define LRC_ERR_CREATE_DIR		5

// The recording file already exists
#define LRC_ERR_FILE_EXISTS		6

// The string is too long
#define LRC_ERR_STR_TOO_LONG	7

// No stream was provided for LRC writer
#define LRC_ERR_NO_STREAM		8

// Too many streams provided for LRC writer
#define LRC_ERR_TOO_MANY_STREAMS	9

// Too many external devices provided for LRC Writer
#define LRC_ERR_TOO_MANY_DEVICES	10

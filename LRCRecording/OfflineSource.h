#pragma once
#include "TemperatureSource.h"

/// <summary>
/// Base class for every offline temperature source (from file).
/// </summary>
class OfflineSource : public TemperatureSource
{
public:
	/// <summary>
	/// Initialises an offline temperature source.
	/// </summary>
	OfflineSource()
	{
		// nothing to do
	}

	/// <summary>
	/// Disposes the offline source.
	/// </summary>
	~OfflineSource()
	{
		// nothing to do
	}

	/// <summary>
	/// Gets the object parameters.
	/// Used for off-line sources such as recording files.
	/// </summary>
	ObjectParameters GetObjectParameters() override
	{
		return objectParams;
	}

	/// <summary>
	/// Sets the object parameters.
	/// Used for on-line sources such as cameras.
	/// </summary>
	/// <returns>Returns an error code.</returns>
	int SetObjectParameters(ObjectParameters value) override
	{
		// cannot set object parameters of an off-line source
		return LRC_ERR_INVALID_VALUE;
	}
};


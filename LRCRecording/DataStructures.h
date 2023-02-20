// Auxiliary data structures
//

#pragma once

#include "pch.h"

using namespace std;

/// <summary>
/// Types of units for writing temperatures to file.
/// </summary>
enum LRCRECORDING_API TemperatureSourceUnits : uint8_t { None, Intensity, Celsius, Kelvin, Time, Radians };



/// <summary>
/// Camera data types. The recorder may save it to a file as a different type.
/// </summary>
enum LRCRECORDING_API CameraDataType : uint8_t { Float, UInt16, Unknown = 255 };



/// <summary>
/// Rectangle with integer coordinates.
/// </summary>
struct LRCRECORDING_API Int32Rect
{
	int x, y, width, height;

	/// <summary>
	/// Initialises an empty rectangle.
	/// </summary>
	Int32Rect()
	{
		x = y = 0;
		width = height = 0;
	}

	/// <summary>
	/// Initialises the rectangle.
	/// </summary>
	/// <param name="x">The x coordinate of the left side.</param>
	/// <param name="y">The y coordinate of the top side.</param>
	/// <param name="width">The width.</param>
	/// <param name="height">The height.</param>
	Int32Rect(int x, int y, int width, int height)
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

	/// <summary>
	/// Checks whether the rectangle is empty, i.e., both width and height are zero.
	/// </summary>
	/// <returns>Returns true if the rectangle is empty, otherwise returns false.</returns>
	bool IsEmpty()
	{
		return width == 0 && height == 0;
	}
};



/// <summary>
/// Range of values, i.e., minimum and maximum.
/// </summary>
struct LRCRECORDING_API Range
{
	/// <summary>
	/// The range bounds.
	/// </summary>
	double min, max;

	/// <summary>
	/// Initialises an empty range.
	/// </summary>
	Range()
	{
		ResetActualRange();
	}

	/// <summary>
	/// Resets the range to empty.
	/// </summary>
	void ResetActualRange()
	{
		min = DBL_MAX;
		max = -DBL_MAX;
	}

	/// <summary>
	/// Checks whether the range is empty.
	/// </summary>
	/// <returns>Returns true if the range is empty, otherwise returns false.</returns>
	bool IsEmpty()
	{
		return min > max;
	}

	/// <summary>
	/// Extends the range to include the given value.
	/// </summary>
	/// <param name="value">The value to include in the range.</param>
	void IncludeValue(double value)
	{
		if (value < min)
			min = value;
		if (value > max)
			max = value;
	}

	/// <summary>
	/// Gets the range length, i.e., max - min;
	/// </summary>
	/// <returns>Returns the range length.</returns>
	double GetLength()
	{
		return max - min;
	}
};



/// <summary>
/// Object parameters.
/// </summary>
struct LRCRECORDING_API ObjectParameters
{
	/// <summary>
	/// Object emissivity.
	/// </summary>
	float emissivity;

	/// <summary>
	/// Reflected apparent temperature (also known as ambient temperature) in Kelvin.
	/// </summary>
	float reflectedTempK;

	/// <summary>
	/// Object distance from camera.
	/// </summary>
	float distance;

	/// <summary>
	/// Atmospheric temperature in Kelvin.
	/// </summary>
	float atmosphericTempK;

	/// <summary>
	/// Relative humidity.
	/// </summary>
	float relHumidity;

	/// <summary>
	/// The atmosphere transmissivity.
	/// </summary>
	float tau;

	/// <summary>
	/// Window temperature in Kelvin.
	/// </summary>
	float windowTempK;

	/// <summary>
	/// Window transmissivity.
	/// </summary>
	float windowTransmiss;

	/// <summary>
	/// Tells whether the tau parameter is set.
	/// If not, it is computed from the atmosphere parameters.
	/// </summary>
	bool isTauSet;

#pragma region Tau calculation constants

	/// <summary>
	/// Attenuation for the atmosphere without water vapour.
	/// </summary>
	float Alpha1 = 0.006569f;
	float Alpha2 = 0.01262f;

	/// <summary>
	/// Attenuation for the water vapour.
	/// </summary>
	float Beta1 = -0.002276f;
	float Beta2 = -0.00667f;

	/// <summary>
	/// Scaling factor for attenuation.
	/// </summary>
	float X = 1.9f;

#pragma endregion

	/// <summary>
	/// Initialises new object parameters.
	/// </summary>
	ObjectParameters()
	{
		// initialise the parameters
		emissivity = 0.92f;
		distance = 2;
		reflectedTempK = atmosphericTempK = windowTempK = CelsiusToKelvin(20);
		relHumidity = 0.53f;
		windowTransmiss = 1;
		CalculateTau();
	}

	/// <summary>
	/// Converts degrees Celsius to Kelvin.
	/// </summary>
	/// <param name="celsius">The value in degrees Celsius.</param>
	/// <returns>Returns the value in Kelvin.</returns>
	float CelsiusToKelvin(float celsius)
	{
		return celsius + 273.15f;
	}

	/// <summary>
	/// Converts Kelvin to degrees Celsius.
	/// </summary>
	/// <param name="kelvin">The value in Kelvin.</param>
	/// <returns>The value in degrees Celsius.</returns>
	float KelvinToCelsius(float kelvin)
	{
		return kelvin - 273.15f;
	}

	/// <summary>
	/// Calculates the tau.
	/// </summary>
	/// <remarks>
	/// http://u88.n24.queensu.ca/exiftool/forum/index.php/topic,4898.msg27530.html#msg27530
	/// Minkina, Dudzik: Infrared Thermography: Errors and Uncertainties, Appendix A, page 171
	/// </remarks>
	void CalculateTau()
	{
		double tAtmC = KelvinToCelsius(atmosphericTempK);
		double mSqrtDist = -sqrt(distance);

		double h2o = relHumidity * exp(1.5587 + 6.939e-2*tAtmC - 2.7816e-4*tAtmC*tAtmC + 6.8455e-7*tAtmC*tAtmC*tAtmC);
		double sqrth2o = sqrt(h2o);

		double calculatedTau = X * exp(mSqrtDist * (Alpha1 + Beta1 * sqrth2o))
			+ (1 - X) * exp(mSqrtDist * (Alpha2 + Beta2 * sqrth2o));

		// tau calculated
		tau = (float)calculatedTau;
		isTauSet = false;
	}
};



/// <summary>
/// Camera parameters.
/// </summary>
struct LRCRECORDING_API CameraParameters
{
	/// <summary>
	/// The data type.
	/// </summary>
	CameraDataType dataType;

	/// <summary>
	/// The camera resolution.
	/// </summary>
	uint16_t width, height;

	/// <summary>
	/// Camera measuring range in Kelvin.
	/// </summary>
	float tempRangeMinK, tempRangeMaxK;

	/// <summary>
	/// Initialises the camera parameters.
	/// </summary>
	CameraParameters()
	{
		dataType = CameraDataType::Float;
		width = height = 0;
		// FLIR Lepton default measuring range (in high gain mode)
		tempRangeMinK = -10 + 273.15f;
		tempRangeMaxK = 140 + 273.15f;
	}
};

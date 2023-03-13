// Auxiliary data structures
//

#pragma once

/// <summary>
/// Range of values, i.e., minimum and maximum.
/// </summary>
struct Range
{
	/// <summary>
	/// The range bounds.
	/// </summary>
	double min, max;
};



/// <summary>
/// Object parameters.
/// </summary>
struct ObjectParameters
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
	float Alpha1;
	float Alpha2;

	/// <summary>
	/// Attenuation for the water vapour.
	/// </summary>
	float Beta1;
	float Beta2;

	/// <summary>
	/// Scaling factor for attenuation.
	/// </summary>
	float X;

#pragma endregion
};

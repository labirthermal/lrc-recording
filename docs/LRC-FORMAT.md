# LRC format

Version 2 released 17.4.2020.

Data is stored in little-endian byte order.

## File header
| Address | Data type | Description |
| --- | --- | --- |
| 0 | 4x uint8 | Signature, always "LRC\0" |
| 4 | uint8 | File version, currently 2 |
| 5 | int64 | File timestamp, .NET DateTime.ToBinary() |
| 13 | uint8 | Number of external devices, typically 0 |
| 14 | uint8 | Number of data streams, typically 1 |
| 15 | uint8 | Stream data type, see below |
| 16 | uint8 | Stream unit, see below |
| 17 | uint16 | Frame resolution, width |
| 19 | uint16 | Frame resolution, height |
| 21 | uint32 | Flags of dynamic part of header, typically 0 |
| 25 | special | Dynamic part of header, typically empty |

### Stream data types
| Value | Meaning |
| --- | --- |
| 0 | float saved as uint16 *) |
| 1 | float |
| 2 | uint16 |

*) When reading, the uint16 range 0&ndash;65535 must be scaled to the specified frame temperature range.

### Stream units
| Value | Meaning |
| --- | --- |
| 0 | none |
| 1 | intensity |
| 2 | degrees Celsius |
| 3 | kelvin |
| 4 | time |
| 5 | radians |

## Frame header
When the dynamic part of file header is empty, the first frame header starts at byte 25.

| Relative address | Data type | Description |
| --- | --- | --- |
| 0 | int64 | Frame timestamp, .NET DateTime.ToBinary() |
| (8) | length-prefixed UTF-8 string | Frame note, typically empty unless specified in dynamic part of header |
| (8) | special | External device data, typically empty |
| 8 | float | Frame temperature range minimum; unit according to stream |
| 12 | float | Frame temperature range maximum; unit according to stream |
| 16 | float | Camera measuring range minimum [K] |
| 20 | float | Camera measuring range maximum [K] |
| 24 | float | Emissivity [0-1] |
| 28 | float | Reflected apparent temperature [K] |
| 32 | float | Distance [m] |
| 36 | float | Atmosphere temperature [K] |
| 40 | float | Relative humidity [0-1] |
| 44 | float | Tau (atmosphere transmittance) [0-1] |
| 48 | float | IR window temperature [K] |
| 52 | float | IR window transmittance [0-1] |
| 56 | uint8 | Is tau set? Typically 0, not set, calculated from atmosphere parameters |
| 57 | stream data type | Frame data; unit according to stream |

# SetGlobalOption

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  [GlobalOption](GlobalOption.md)<br>[GetGlobalOption()](GetGlobalOption.md)|

## Overview
This function is used to set a Global option. The available Global options and possible values can be found in the documentation for the [GlobalOption](GlobalOption.md) enumeration.

## Syntax

```python
SetGlobalOption(option, value)
```

## Paramerers

|Parameter      | Description |
|---------------|---------------|
|option|Member of the GlobalOptions enumeration that specifies which option to set.|
|value |The new value to which the option will be set. See the [GlobalOption](GlobalOption.md) enumeration class for the type of value for each option.| 

## Known Issues

## Example
```Python
import mvnc.mvncapi as ncs

# Set the global logging level to verbose
ncs.SetGlobalOption(ncs.GlobalOption.LOGLEVEL, 2)
```

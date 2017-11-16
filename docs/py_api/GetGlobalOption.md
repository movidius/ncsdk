# GetGlobalOption

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  [GlobalOption](GlobalOption.md)<br>[SetGlobalOption()](SetGlobalOption.md)|

## Overview
This function is used to get a Global option. The available options can be found in the [GlobalOption](GlobalOption.md) enumeration section.

## Syntax

```python
value = GetGlobalOption(option)
```

## Parameters

|Parameter      | Description |
|---------------|---------------|
|option     |Member of the GlobalOption enumeration that specifies which option to get.|

## Return
The value for the specified option. The type of the returned value depends on the option specified. See the [GlobalOption](GlobalOption.md) enumeration for the type that will be returned for each option.

## Known Issues

## Example
```Python
from mvnc import mvncapi as mvnc

# Get current logging level Global Option
logLevel = mvnc.GetGlobalOption(mvnc.GlobalOption.LOGLEVEL)

print("The current global logging level is: ", logLevel)
```

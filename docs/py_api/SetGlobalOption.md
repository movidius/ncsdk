# SetGlobalOption

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  GlobalOption<br>GetGlobalOption()|

## Overview
This function is used to set a global option. The available Global options and possible values can be found in the documentation for the GlobalOption enumeration.

## Syntax

```python
SetGlobalOption(option, value)
```

## Paramerers

|Parameter      | Description |
|---------------|---------------|
|option|Member of the GlobalOptions enumeration which specifies which option to set.|
|value |The new value to which the option will be set.  See the GlobalOption enumeration class for the type of value for each option.| 

## Known Issues

## Example
```Python
import mvnc.mvncapi as ncs

# set the global logging level to verbose
ncs.SetGlobalOption(ncs.GlobalOption.LOGLEVEL, 2)
```

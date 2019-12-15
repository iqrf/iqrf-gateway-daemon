# Tracing component design

Tracing to files is done by shape component `shape::TraceFileService` implementing interface `shape::ITraceService` The component can run in an unlimited number of instances according configuration setting. One instance writes to one file.

## General features
Tracing is part of the source code. The trace records are annotated with the channel and the level parameters:
- **channel** - to group trace records
- **level** - to set trace details

Tracing features:
- More trace channels can be written to the file.
- The level is set per the channel.
- The file is opened at the start of the daemon.
- If the file already exists from a previous run it is overwritten.
- If the file reaches maximum size it is overwritten.
- Name of the file can be prefixed with timestamp to avoid overwrite.
- Maximum size of the file can be set.
- Maximum number of the timestamped files can be set
- Maximum age of the timestamped files can be set

## Configuration
Meaning of the configuration parameters is obvious from the configuration schema excerpt:

```json
{
    "properties": {
        "path": {
            "type": "string",
            "description": "Path to the trace files",
            "default": ""
        },
        "filename": {
            "type": "string",
            "description": "Name of the trace file",
            "default": "IqrfDaemon.txt"
        },
        "maxSize": {
            "type": "integer",
            "description": "Maximum size of the file",
            "default": 1048576
        },
        "timestampFiles": {
            "type": "boolean",
            "description": "The file is timestamped in the format YYY-MM-DD-hh-mm-ss-mss-<filename> to avoid overwrite",
            "default": false
        },
        "maxAgeMinutes": {
            "type": "integer",
            "description": "Maximum age of the timestamped files in minutes. If set to zero or not present it is not applied",
            "default": 0
        },
        "maxNumber": {
            "type": "integer",
            "description": "Maximum number of the timestamped files. If set to zero or not present it is not applied",
            "default": 0
        },
        "VerbosityLevels": {
            "type": "array",
            "description": "Array of pairs channel and verbosity level",
            "items": {
                "type": "object",
                "properties": {
                    "channel": {
                        "type": "integer",
                        "default": 0,
                        "description": "The channel to be routed to the file"
                    },
                    "level": {
                        "type": "string",
                        "default": "DBG",
                        "description": "Set level of trace"
                        "enum": [
                            "DBG",
                            "INF",
                            "WAR",
                            "ERR"
                        ]
                    }
                },
                "required": [
                    "channel",
                    "level"
                ]
            }
        },
    }
}
```
Note the channels used in the daemon are:
- 0 - default - all components
- 33 - JsCache 

If `"timestamp": true` it creates set of the files timestamped by the creation time, e.g:
- 2019-12-13-15-22-35-580-IqrfDaemon.txt
- 2019-12-13-15-23-32-129-IqrfDaemon.txt
- 2019-12-13-15-46-33-430-IqrfDaemon.txt

The files are created during (re)start of the daemon or if maximal size of the actual opened file was reached.The number of the files can be limited e.g. to 10: `"maxNumber": 10` Note there is always +1 file as this is the opened actual one.

The files can be limited by its age e.g. `"maxAgeMinutes": 1440` means the files older then one day are removed when a new file is created. So the actual file is not removed even if it is opened for more than one day. The age of the files is not taken from filesystem but directly from its name.

In the previous version v2.2 there was a formal error. The parameter used to set maximum size was named **maxSizeMB**, but the number was interpreted as a size in bytes. Thus the parameter was renamed to **maxSize** in version v2.3 and can be set in between:
- minimum 100KB
- maximum 100MB

The parameter **maxSize** is optional (as used to be maxSizeMB) and it is defaulted to 1MB.
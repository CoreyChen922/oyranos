static const char help_message[] = ""
" +------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n"
" |                                                                                                             Get various cinds of information about camera devices|\n"
" |------------------------------------------------------------------------------------------------------------------------------------------------------------------|\n"
" |           |                                input                                |                                     output                                     |\n"
" |-----------+---------------------------------------------------------------------+--------------------------------------------------------------------------------|\n"
" |           |     tag      |         value         |           Comments           |  oyConfig_s  |         tag          |    value    |           Action           |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |           |              |                       |                              |              |                      |file_name\\n |Informs the calling program |\n"
" |   required|command       |list                   |                              |::data        |device_handle         |data_blob    |that it should provide a    |\n"
" |           |              |                       |                              |              |                      |             |file as device_handle       |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |           |              |                       |Returns                       |              |                      |             |One oyConfig_s struct with  |\n"
" |           |              |                       |“manufacturer\\nmodel0\\n...”,|::data        |supported_devices_info|<string list>|all supported devices       |\n"
" |           |              |                       |instead of device_name options|              |                      |             |                            |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   optional|device_handle |<string>               |raw image file                |::backend_core|<property name>       |<string>     |List of all properties from |\n"
" |           |              |                       |                              |              |                      |             |EXIF tags (H/W Information) |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |alterantive|              |oyBlob_s               |raw image in memmory          |              |                      |             |                            |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   optional|driver_version|                       |The LibRaw version            |::data        |driver_version_string |<string>     |                            |\n"
" |           |              |                       |                              |              |driver_version_number |             |                            |\n"
" |------------------------------------------------------------------------------------------------------------------------------------------------------------------|\n"
" |                                                                                                                         Get the properties of a particular device|\n"
" |------------------------------------------------------------------------------------------------------------------------------------------------------------------|\n"
" |           |                                input                                |                                     output                                     |\n"
" |-----------+---------------------------------------------------------------------+--------------------------------------------------------------------------------|\n"
" |           |     tag      |         value         |           Comments           |  oyConfig_s  |         tag          |    value    |           Action           |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |           |              |                       |                              |              |                      |             |List of all properties, i.e.|\n"
" |   required|command       |properties             |                              |::backend_core|<property name>       |<string>     |info from EXIF & LibRaw     |\n"
" |           |              |                       |                              |              |                      |             |options                     |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   required|device_handle |<string>               |raw image file                |              |                      |             |                            |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |alterantive|              |oyBlob_s               |raw image in memmory          |              |                      |             |                            |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   optional|device_context|libraw_output_params_t*|Requires device_handle        |              |                      |             |                            |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   optional|icc_profile.add_meta|oyProfile_s      |ICC profile                   |::data        | icc_profile.add_meta |oyProfile_s  |add calibration in meta tag |\n"
" |------------------------------------------------------------------------------------------------------------------------------------------------------------------|\n"
" |                                                                                                                                      Get an extensive help mesage|\n"
" |------------------------------------------------------------------------------------------------------------------------------------------------------------------|\n"
" |           |                                input                                |                                     output                                     |\n"
" |-----------+---------------------------------------------------------------------+--------------------------------------------------------------------------------|\n"
" |           |     tag      |         value         |           Comments           |  oyConfig_s  |         tag          |    value    |           Action           |\n"
" |-----------+--------------+-----------------------+------------------------------+--------------+----------------------+-------------+----------------------------|\n"
" |   required|command       |help                   |                              |              |                      |             |Print help message to       |\n"
" |           |              |                       |                              |              |                      |             |standard error              |\n"
" +------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n"
;
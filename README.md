## LIBs

- ArduinoJson:
  - URL: https://arduinojson.org/v6/doc/
  - Instructions:
    1. Open Arduino IDE > Tools > Library Management;
    2. Search for "ArduinoJson";
    3. Install the version 6.18.3
    4. Add to the top of the .ino the include `#include <ArduinoJson.h>`

## COMMON ERRORS

- `(...) update includePath (...)`:
  - CTRL + SHIFT + P > C++ edit configs > Check if the necessary paths are included.
  - Check VS Code bottom bar: at the right side of the port (eg. COM3), there should be "Arduino" in place of "Win32" or whichever C++ existing config
  - Check if the folder .vscode has a c_cpp_properties.json in it with all the include paths
  - Add to VS Code settings.json: `"files.associations": { "*.ino": "cpp" }`

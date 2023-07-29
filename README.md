# RtAudioWrapper

## Including

To add this library to your project, simply add these two lines to your *CMakeLists.txt*:
```cmake
add_subdirectory(path/to/RtAudioWrapper)
target_link_libraries(${PROJECT_NAME} PRIVATE RtAudioWrapper::RtAudioWrapper)
```

Then include it as:
```cpp
#include <RtAudioWrapper/RtAudioWrapper.hpp>
```

## Running the tests

Simply use "tests/CMakeLists.txt" to generate a project, then run it.<br/>
If you are using VSCode and the CMake extension, this project already contains a *.vscode/settings.json* that will use the right CMakeLists.txt automatically.

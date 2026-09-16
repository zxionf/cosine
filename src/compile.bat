C:\VulkanSDK\1.4.335.0\Bin\glslc.exe shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
C:\VulkanSDK\1.4.335.0\Bin\glslc.exe shaders/simple_shader.frag -o shaders/simple_shader.frag.spv
xcopy ".\shaders" "..\build\src\shaders" /E /I /Y
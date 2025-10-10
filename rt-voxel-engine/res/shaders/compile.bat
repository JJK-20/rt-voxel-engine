start "" /d "%cd%" "%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-generation.rgen   -o ray-generation.spv
start "" /d "%cd%" "%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-closest-hit.rchit -o ray-closest-hit.spv
start "" /d "%cd%" "%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-miss.rmiss        -o ray-miss.spv

::error to log file
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-generation.rgen -o ray-generation.spv > ray-generation.log 2>&1
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-closest-hit.rchit -o ray-closest-hit.spv > ray-closest-hit.log 2>&1
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-miss.rmiss -o ray-miss.spv > ray-miss.log 2>&1

::error to console
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-generation.rgen -o ray-generation.spv
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-closest-hit.rchit -o ray-closest-hit.spv
::"%VULKAN_SDK%/bin/glslangValidator" --target-env vulkan1.2 -V ray-miss.rmiss -o ray-miss.spv

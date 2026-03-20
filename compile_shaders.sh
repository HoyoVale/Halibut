#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "正在编译着色器...（compiling shaders...）"

./Halibut/vendor/slangc/bin/linux/bin/slangc \
    assets/shaders/BasicShader.slang \
    -entry vertMain \
    -stage vertex \
    -target spirv \
    -profile glsl_450 \
    -o assets/shaders/triangle.vert.spv

./Halibut/vendor/slangc/bin/linux/bin/slangc \
    assets/shaders/BasicShader.slang \
    -entry fragMain \
    -stage fragment \
    -target spirv \
    -profile glsl_450 \
    -o assets/shaders/triangle.frag.spv

if [[ -f assets/shaders/triangle.vert.spv && -f assets/shaders/triangle.frag.spv ]]; then
    echo "着色器编译完成（successfully compiled shaders）"
else
    echo "着色器编译失败（failed compiled shaders）"
    exit 1
fi
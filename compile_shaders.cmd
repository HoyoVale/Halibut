@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
echo 正在编译着色器...（compiling shaders...）
call Halibut\vendor\slangc\bin\win\Bin\slangc.exe assets\shaders\BasicShader.slang -entry vertMain -stage vertex -target spirv -profile glsl_450 -o assets\shaders\triangle.vert.spv
call Halibut\vendor\slangc\bin\win\Bin\slangc.exe assets\shaders\BasicShader.slang -entry fragMain -stage fragment -target spirv -profile glsl_450 -o assets\shaders\triangle.frag.spv
if exist assets\shaders\triangle.vert.spv (
    if exist assets\shaders\triangle.frag.spv (
        echo 着色器编译完成（successfully compiled shaders）
    ) else (
        echo 着色器编译失败（failed compiled shaders）
    )
) else (
    echo 着色器编译失败（failed compiled shaders）
)



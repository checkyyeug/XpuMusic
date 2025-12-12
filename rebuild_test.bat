@echo off
cd /d "C:\workspace\XpuMusic\build"

echo Building sdk_impl target...
MSBuild.exe sdk_impl.vcxproj /p:Configuration=Debug /p:Platform=x64 /m

echo.
echo Building foobar2k-player target...
MSBuild.exe foobar2k-player.vcxproj /p:Configuration=Debug /p:Platform=x64 /m

echo.
echo Build completed!
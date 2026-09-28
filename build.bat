@echo off
setlocal

pushd "%~dp0" || exit /b 1

cmake --preset windows-release
if errorlevel 1 goto fail

cmake --build --preset windows-release
if errorlevel 1 goto fail

cmake -E make_directory build\windows-release\test-appdata
if errorlevel 1 goto fail

cmake -E env "APPDATA=%CD%\build\windows-release\test-appdata" ctest --test-dir build\windows-release --output-on-failure
if errorlevel 1 goto fail

cmake --install build\windows-release --config Release
if errorlevel 1 goto fail

release\shareaudio_cli.exe help
if errorlevel 1 goto fail

cmake -E rm -rf release-test
if errorlevel 1 goto fail

echo.
echo Build completed. Portable files are in: %CD%\release

popd
endlocal
exit /b 0

:fail
echo.
echo Build failed.
popd
endlocal
exit /b 1

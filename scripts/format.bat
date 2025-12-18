@echo off
setlocal

for /R src %%f in (*.cpp *.hpp) do (
    clang-format -i "%%f"
)

endlocal

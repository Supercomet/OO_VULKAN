@echo off
cd "bin"
for %%f in (*.spv) do (
	DEL %%f
)
cd "../"
pause
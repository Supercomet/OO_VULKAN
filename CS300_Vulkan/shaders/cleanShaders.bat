@echo off

for %%f in (*vert.spv *frag.spv) do (
	DEL %%f
)
pause
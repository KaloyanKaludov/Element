@echo off

set interpreter=..\bin\element_d.exe

for %%f in (*.element) do (
	echo tests from: %%f
	%interpreter% --test %%f
)

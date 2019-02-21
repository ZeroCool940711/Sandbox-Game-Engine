@echo Please examine settings below and modify if necessary
cd \bigworldtech\bigworld\tools\cat
set MF_ROOT=/bigworldtech
set BW_RES_PATH=/bigworldtech/fantasydemo/res;/bigworldtech/bigworld/res
@if exist main.py (python main.py) else (python main.pyc)
pause

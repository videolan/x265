
:: Input for this script is number_of_frames.

@echo off

set "frames=%1"

@echo ...

if not exist BasketballDrive_1920x1080_50.y4m GOTO OVER1

@echo Running encoder for BasketBallDrive: config all_I ...
@echo Basketball_all_I > dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -i BasketballDrive_1920x1080_50.y4m -c ..\..\cfg\encoder_all_I.cfg -f %frames% -b script.out > encoder_output.txt
@echo Running decoder for BasketBallDrive...
TAppDecoder.exe -b script.out -o script.yuv > decoder_output.txt
@echo Running dr_psnr for BasketBallDrive...
dr_psnr.exe -r BasketballDrive_1920x1080_50.y4m -c script.yuv -w 1920 -h 1080 -e %frames% >> dr_psnr_output.txt

@echo Running encoder for BasketBallDrive: config I_15P ...
@echo Basketball_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -i BasketballDrive_1920x1080_50.y4m -c ..\..\cfg\encoder_I_15P.cfg -f %frames% -b script.out >> encoder_output.txt
@echo Running decoder for BasketBallDrive...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for BasketBallDrive...
dr_psnr.exe -r BasketballDrive_1920x1080_50.y4m -c script.yuv -w 1920 -h 1080 -e %frames% >> dr_psnr_output.txt

@echo ...
GOTO FOURPEOPLE

:OVER1
@echo To run this script, BasketballDrive_1920x1080_50.y4m file should be available in \build\dr_psnr_script directory.
@echo ...

:FOURPEOPLE
if not exist FourPeople_1280x720_60.y4m GOTO OVER2

@echo Running encoder for FourPeople: config all_I ...
@echo Fourpeople_all_I >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -i FourPeople_1280x720_60.y4m -c ..\..\cfg\encoder_all_I.cfg -f %frames% -b script.out >> encoder_output.txt
@echo Running decoder for FourPeople...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for FourPeople...
dr_psnr.exe -r FourPeople_1280x720_60.y4m -c script.yuv -w 1280 -h 720 -e %frames% >> dr_psnr_output.txt

@echo Running encoder for FourPeople: config I_15P ...
@echo Fourpeople_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -i FourPeople_1280x720_60.y4m -c ..\..\cfg\encoder_I_15P.cfg -f %frames% -b script.out >> encoder_output.txt
@echo Running decoder for FourPeople...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for FourPeople...
dr_psnr.exe -r FourPeople_1280x720_60.y4m -c script.yuv -w 1280 -h 720 -e %frames% >> dr_psnr_output.txt

@echo ...
GOTO DONE

:OVER2
@echo To run this script, FourPeople_1280x720_60.y4m file should be available in \build\dr_psnr_script directory.
@echo ...

:DONE
@echo Done...

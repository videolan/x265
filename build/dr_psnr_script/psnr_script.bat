@echo Basketball_all_I > dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c BasketballDrive.cfg -c encoder_all_I.cfg -b script.out > encoder_output.txt
TAppDecoder.exe -b script.out -o script.yuv > decoder_output.txt
dr_psnr.exe -r D:\HEVC\sample_data\BasketballDrive_1920x1080_50\BasketballDrive_1920x1080_50.yuv -c script.yuv -w 1920 -h 1080 -e 30 >> dr_psnr_output.txt


@echo Basketball_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c BasketballDrive.cfg -c encoder_I_15P.cfg -b script.out >> encoder_output.txt
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
dr_psnr.exe -r D:\HEVC\sample_data\BasketballDrive_1920x1080_50\BasketballDrive_1920x1080_50.yuv -c script.yuv -w 1920 -h 1080 -e 30 >> dr_psnr_output.txt


@echo Fourpeople_all_I >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c FourPeople.cfg -c encoder_all_I.cfg -b script.out >> encoder_output.txt
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
dr_psnr.exe -r D:\HEVC\sample_data\FourPeople_1280x720_60\FourPeople_1280x720_60.yuv -c script.yuv -w 1280 -h 720 -e 30 >> dr_psnr_output.txt


@echo Fourpeople_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c FourPeople.cfg -c encoder_I_15P.cfg -b script.out >> encoder_output.txt
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
dr_psnr.exe -r D:\HEVC\sample_data\FourPeople_1280x720_60\FourPeople_1280x720_60.yuv -c script.yuv -w 1280 -h 720 -e 30 >> dr_psnr_output.txt

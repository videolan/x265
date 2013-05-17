1.Set the following in the config file

ScriptPath - path where the batch scripts (Profiler.bat)are saved.
DecoderApplicationPath - path where the decoder application is present (TAppDecoder.exe)
encoderApplicationPath - path where encoder application is present (x265-cli.exe)
fileCfgPath  -  Path where input files and cfg files are present. Both input testsequence file name and cfg filename should be same
psnrApplicationPath - Path where the ds_psnr application is present (dr_psnr.exe)
ffmpegPath  -  Path where ffmeg tool is installed. This is required only for testing y4m file
frames -- no:of frames to encode
testFile1= filename without the extension


2. Run performanceProfiler.bat

3. Check the results in Result.txt under the path <encoderApplicationPath >


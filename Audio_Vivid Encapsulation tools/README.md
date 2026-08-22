# 一、编译环境 Unix-like
1. 基于FFmpeg-n4.4.2版本
2. 将对应文件夹libavcodec和libavformat直接放入FFmpeg-n4.4.2源代码对应文件夹执行替换即可。
3. 建议安装x86 NASM汇编依赖
   1. sudo apt-get install yasm (Ubuntu)
   2. brew install yasm (macOS)
4. 编译步骤：参见ffmpeg编译命令
   1. 执行./configure
   2. 执行make 

# 二、Audio vivid码流转封装基础命令
## Audio vivid码流转封装到Mpeg2-TS或者从Mpeg2-TS解封装
命令行：ffmpeg -i input.av3a -c copy output.ts 

命令行：ffmpeg -i input.ts -c copy output.av3a 

## Audio vivid码流转封装到MP4或者从MP4中解封装
命令行：ffmpeg -i input.av3a -c copy output.mp4 

命令行：ffmpeg -i input.mp4 -c copy output.av3a 

## Audio vivid码流转封装到Mpeg-Dash
命令行：ffmpeg -i input.av3a -c copy -f dash output.mpd

注：input.av3a表示Audio vivid码流，注意后缀为av3a

# 三、音视频封装
## Audio vivid码流和TS视频流文件转封装到Mpeg-TS格式
命令行：ffmpeg -i input_video.ts -i input.av3a -vcodec copy -acodec copy output.ts 

## Audio vivid码流和MP4视频流文件转封装到Mp4格式
命令行：ffmpeg -i input_video.mp4 -i input.av3a -vcodec copy -acodec copy output.mp4 

## Audio vivid码流和mp4视频流文件转封装到Mpeg-Dash格式
命令行：ffmpeg -i input_video.mp4 -i input.av3a -vcodec copy -acodec copy -f dash output.mpd

注：input.av3a表示Audio vivid码流，注意后缀为**av3a**; input_video.ts/input_video.mp4表示一个没有音频流的mp4/Mpeg-TS封装格式的视频流文件。
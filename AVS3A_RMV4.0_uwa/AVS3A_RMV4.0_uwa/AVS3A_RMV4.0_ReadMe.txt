信息技术 智能媒体编码 第3部分：沉浸式音频 第一阶段（广电/流媒体Profile） (以下简称AVS3-P3)参考编解码器使用说明

*  版本：AVS3A_RMv4.0
   更新说明：1. 解决Linux、Mac跨平台编译报错；2. （未完成）通用高码率音频编码工具中删除BWE、对象元数据相关代码，AASF和AATF删除BWE、对象元数据相关代码

*  AVS3-P3参考软件包含工具及说明：
    通用全码率音频编码工具	: 支持全部码率下声道信号编码、对象信号编码、HOA信号编码
    通用高码率音频编码工具	: 支持高码率下声道信号编码
    无损音频编码工具           	: 支持声道信号编码、对象信号编码、HOA信号编码
    元数据编码工具                      : 支持元数据编码


*  通用全码率音频编解码工具使用说明：

1. 编码器
1.1 命令行选项
    avs3RM0Encoder  -codec_id 2  [options]  [bitrate]  [samplingRate]  [inFileName]  [outFileName]
1.2 参数说明：
    -codec_id 2：代表选择通用全码率音频编码工具，此选项应放在命令行最前
    inFileName：输入文件名（*.wav）
    outFileName：输出文件名（*.av3a）
    bitrate：编码速率（bps）
    samplingRate：输入信号采样率（kHz）
options说明：
    -nn_type: 神经网络模式配置，0代表基本配置，1代表低复杂度配置
    -max_band：最大编码频带宽度，SWB：超宽带，FB：全带
    -bitdepth：位深，16：16比特位深，24：24比特位深
    -meta_file：(可选)元数据二进制文件，文件格式应符合标准文档中的元数据语法
    -mono：单声道模式
    -stereo：立体声模式
    -mc channel_config：多声道模式，channel_config为多声道扬声器配置，例如MC_5_1_0为5.1格式，MC_5_1_4为5.1.4格式
    -hoa order: HOA模式，order为HOA信号的阶数，1为FOA，2、3分别对应2阶和3阶HOA
    -mix  soundBedType  soundBed_channel_config  soundBed_bitrate  num_objs  bitrate_per_obj: 混合信号模式。soundBedType为声床类型，0表示不包含声床（即纯对象信号），1表示多声道声床+对象信号。soundBed_channel_config为声床信号类型（支持立体声、5.1等）。soundBed_bitrate为声床信号编码速率。num_objs为对象信号数量。bitrate_per_obj为每个对象信号的编码速率。
1.4 示例：
示例1：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -stereo 48000 48 test.wav test.av3a
    -- 立体声编码，低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，采样率48kHz，编码速率48kbps，输入文件test.wav，输出码流文件test.av3a
示例2：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -mc MC_5_1_0 96000 48 test.wav test.av3a
    -- 多声道编码（5.1格式），低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，采样率48kHz，编码速率96kbps，输入文件test.wav，输出码流文件test.av3a
示例3：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -hoa 3 256000 48 test.wav test.av3a
    -- HOA编码（3阶），低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，采样率48kHz，编码速率256kbps，输入文件test.wav，输出码流文件test.av3a
示例4：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -mix 0 4 44000 0 48 test.wav test.av3a
    -- 混合模式编码（声床类型为0，纯对象信号，对象数量4），低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，采样率48kHz，每个对象编码速率44kbps，输入文件test.wav，输出码流文件test.av3a
示例5：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -mix 1 MC_5_1_0 192000 2 64000 0 48 test.wav test.av3a
    -- 混合编码模式（声床类型为1，声床类型5.1，对象数量2），低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，采样率48kHz，5.1声床编码速率192kbps，每个对象编码速率64kbps，输入文件test.wav，输出码流文件test.av3a
示例6：
    avs3RM0Encoder -codec_id 2 -nn_type 1 -max_band FB -bitdepth 16 -meta_file meta.bin -mix 1 MC_5_1_0 192000 2 64000 0 48 test.wav test.av3a
    -- 混合编码模式（声床类型为1，声床类型5.1，对象数量2），低复杂度配置，最大编码频带宽度FB，输入信号位深16bit，元数据文件为meta.bin，采样率48kHz，5.1声床编码速率192kbps，每个对象编码速率64kbps，输入文件test.wav，输出码流文件test.av3a

2. 解码器
2.1 命令行选项
    avs3RM0Decoder  -if  [inFileName]  -of  [outFileName]
2.2 参数说明：
    inFileName：输入文件名（*.av3a）
    outFileName：输出文件名（*.wav）
2.3 示例：
    avs3RM0Decoder -if test.av3a -of test_dec.wav
    -- 对输入码流test.av3a进行解码，得到解码音频文件test_dec.wav


*  通用高速率音频编解码工具使用说明：
   引用AVS2-P3参考软件v4.5版本

1. 编码器 (Encoder)
1.1 命令行选项 (Command line options)
avs2enc.exe   -if [infilename] -of [outfilename] [options]
选项 (Options):
-if [infilename]    ：	输入文件名（*.wav）
-of [outfilename] :	输出文件名（*.avsa）
-f <1,2>              :	输出码流格式
                                1: 存储格式(AASF); 2: 传输格式(AATF); 默认是存储格式AASF
-b X                     :	X是编码码率
-codec_id X          :	X是 {0, 1}
                                0: 通用音频编码; 1:无损音频编码; 默认是通用音频编码
-coding_profile X :	X 是 {0, 1}
                                0: 基础编码, 1: 3D 编码
-ob X                   :	当coding_profile 是1时, -ob 是必选项, X 是对象数据的编码码率
-h/-help               :	显示帮助信息
1.2 示例 (Examples)
示例1 (Example 1):  
avs2enc -f 1 -codec_id 0 -coding_profile 0 -b 64000 -if test.wav -of test.avsa  
  -  编码test.wav，使用通用编码器输出存储格式（AASF）64,000bps码率位流 
示例2 (Example 2):
avs2enc -f 1 -codec_id 0 -coding_profile 1 -b 256000 -if test.wav -of test.avsa -ob 64000
  -  3D编码test.wav. 使用通用编码器输出存储格式（AASF）256,000bps码率位流和64000bps对象码流 

2. 解码器 (Decoder)
2.1 命令行选项 (Command line options)
avs2dec.exe  -if infile.avsa -of outfile.wav [options]
选项(Options):
-if [infilename]    ：	输入文件名（*.wav）
-of [outfilename] :	输出文件名（*.avsa）
-fp [filepath]        :	输入文件路径
-h/-help              :	显示帮助信息
2.2 示例 (Examples)
示例1 (Example 1):
avs2dec.exe -if test.avsa -of output.wav 
  -  解码test.avsa，输出为output.wav
示例2 (Example 2):
avs2dec.exe -fp D:\usr\ -if test_bed_obj.avsa -of output.wav
  - 解码带对象数据的test_bed_obj.avsa文件，输出为output.wav，注意-fp是必须项，其路径下要有一个object_dec.txt文件，object_dec.txt文件中存储的是解码后的对象文件输出，每个对象文件以空格隔开，如：D:\usr\obj1_dec.wav D:\usr\obj2_dec.wav D:\usr\obj3_dec.wav


*  无损音频编解码工具使用说明：
    引用AVS2-P3参考软件v4.5版本

1. 编码器 (Encoder)
1.1 命令行选项 (Command line options)
avs2enc.exe      [-ms<+,->][-w<+,->][-of[outfilename]][-f<0,1,2>][-lpc<0...127>][-e<0,1,2>][-h/-help] [-if[infilename]]
1.2 选项(Options):
  -if[infilename]:        输入文件名（*.wav）
  -of[outfilename] :   输出文件名（*.avsl）
  -f<0,1,2>           :    输出码流格式
                                1: 存储格式(AASF); 2: 传输格式(AATF); 默认是存储格式AASF
  -ms<+,->          :     开启 ('+') 或关闭 ('-') 声道去相关编码
  -w<+,->            :     开启 ('+') 或关闭 ('-') 小波变换
  -lpc X                 :     LPC最大阶数, X取值(1, 127)
  -e X                    :    X是{0,1, 2}, 0: 算数编码, 1: Golomb-rice编码, 2: (混合编码)
  -codec_id X       :    X是 {0, 1}, 0: 通用音频编码; 1:无损音频编码; 默认是通用音频编码
  -h/-help             :     显示帮助信息
1.3 示例 (Examples) 
示例1 (Example 1):
avs2enc -f 1 -codec_id 1 -of test.avsl -if test.wav
编码test.wav，使用无损解码器输出文件格式为AASF的test.avsl文件 

2. 解码器 (Decoder)
2.1 命令行选项 (Command line options)
avs2dec.exe [options] -if infile.avsl -of outfile.wav
2.2 选项(Options):
-h or -help   :    显示帮助信息
-if infile.avsl :    输入压缩文件(*.avsl)
-of outfile.wav: 输出解压音频文件(*.wav)
-b X              :    设置输出文件位率，X是{0,1,2}，默认是1
                         0:  8位PCM数据, 1: 16位PCM数据(默认值), 2: 24位PCM数据.
-codec_id X  :    X是 {0, 1}, 0: 通用音频编码; 1:无损音频编码; 默认是通用音频编码
2.3 示例 (Examples)
示例1 (Example 1):
avs2dec -if test.avsl -of test.wav -b 1
解码test.avsl，输出为16位PCM的test.wav
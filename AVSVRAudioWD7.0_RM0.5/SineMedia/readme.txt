赛因渲染器代码编译及使用说明：
	(备注：该渲染器代码没有实现drc模块，对象渲染器objects_render只能处理一个对象的单声道音频)
编译方式1：
	1）进入“赛因渲染器代码”文件夹内
	1）直接使用Visual Studio打开工程中的.sln文件
	2）编译生成解决方案中的objects_render项目，direct_speakers_render项目，hoa_render项目，生成对应的可执行文件
编译方式2：（需要提前安装cmake和boost库）
	1）通过cmake-gui可视化工具构建，源码路径为该目录下“赛因渲染器代码”文件夹，选择boost库路径，选择构建目标路径build
	2）选择boost库路径，创建构建目标路径build
	3）进行构建，并进入build文件夹中
	4）打开VS解决方案文件(.sln)
	5）分别编译生成解决方案中的objects_render项目，direct_speakers_render项目，hoa_render项目，生成对应的可执行文件


可执行文件使用方式：
	[run_exe] [input_wav] [output_wav] [format]
	参数说明	run_exe: 可执行文件名
		input_wav: BW64格式的wav文件
		output_wav: 渲染后的音频文件
		format: 输出格式（扬声器：4+7+0，立体声：0+2+0）
	
	示例：objects_render.exe input.wav output.wav 4+7+0

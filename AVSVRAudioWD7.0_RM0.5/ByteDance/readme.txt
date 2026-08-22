字节双耳渲染器代码编译及使用说明：
	(备注：该渲染器代码没有实现drc模块)

编译方式（需要提前安装cmake）：
	1）进入“字节双耳渲染器代码”文件夹内
	2）启动powershell cd到该目录下
	3）执行指令     .\script\build_win.ps1
	4）进入dist文件夹可寻找到exe文件

可执行文件使用方式：
	./file-render.exe -i [input_wav] -o [output_wav]
	参数说明 	input_wav: BW64格式的wav文件
		output_wav: 渲染后的音频文件

我用Deepseek写了一个shell，运行在PDP-11计算机上，叫dsh，可以实现现代shell的基本功能（可不是 https://github.com/dancerj/dsh 那个dsh！）。

这只是一个玩具，甚至只有一个C文件！它非常简单，很适合用来学习。代码整体是用K&R C写的。

[屏幕录像_20260406_132741.webm](https://github.com/user-attachments/assets/7df8fb50-584a-4a72-8e46-1a83e040e3ad)

这是我的演示视频，在我的计算机上用SimH模拟器成功运行了211BSD并测试了dsh的基本功能。

根目录下的“pdp11”二进制文件是我自己编译的x64 arch上的SimH模拟器，官网下载的旧版本。注意不要用新版本的OpenSIMH！下载链接： https://simh.trailing-edge.com/ ，请自行编译。

2026.5.9:

我让Deepseek写了一个ANSI C标准的dsh_mod，可以在现代系统上运行。

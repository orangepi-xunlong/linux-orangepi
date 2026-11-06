# 简介
* A2平台 OTP烧录工具

## 运行环境
* 运行于linux应用层的可执行程序

# 使用方法
## 通用选项


## 功能
1. 查看版本信息
* 打印相关版本信息
```
# ./otp_burn_tool -i
Tool version : v0.1
IP   version : 0x30d70003
IC      uuid : 0xe5a08081 0x141b419a
```
2. 烧写UUID
* 将uuid列表以uuid.txt的形式和程序放在一起，运行时程序会加载uuid.txt并按行读取其中的uuid，使用过的uuid会写入uuid_used.txt,并且下一次烧录会主动跳过已使用的uuid。
* uuid列表由郭若言整理在：https://wiki.sophgo.com/display/HardSYS/Athena2+otp+uuid，
* uuid.txt格式要求
```
0x141b429ad9998d81
0x141b429ad9998d82
0x141b429ad9998d83
0x141b429ad9998d84
0x141b429ad9998d85
0x141b429ad9998d86
0x141b429ad9998d87
0x141b429ad9998d88
0x141b429ad9998d89
....
```
* 正常流程（输入Y/y）
```
# ./otp_burn_tool --program_uuid
uuid : 0xd9998d8e 0x141b429a, OK?
y
IC uuid : 0xe5a08081 0x141b419a
```
* 解析出的uuid不对，输入Y/y之外的其它字符，退出烧录
```
# ./otp_burn_tool --program_uuid
uuid : 0xd9998d8e 0x141b429a, OK?
n
```

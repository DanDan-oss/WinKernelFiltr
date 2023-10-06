# WinKernelFiltr

```bat
# Windbg串口调试
bcdedit /debug on
bcdedit /dbgsettings serial baudrate:115200 debugport:1
# ＂C:\ProgramFiles(x86)\WindowsKits\l0\Debuggers\x64\Wmdbgexe＂ -b -k com:pipe,port=\\.\pipe\com_1,resets=0
```

```bat
# VS基于网络调试 hostip是调试机IP, port是被调试机端口
bcdedit /debug on
bcdedit /dbgsettings net hostip:192.168.0.101 port:50010
Key=3szhhq9o4d1nx.2sp7obqwnhir8.3jpuijkknd80i.24t55pk7n0n3d
```




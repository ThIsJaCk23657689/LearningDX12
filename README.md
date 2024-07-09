# Learnging DirectX 12
nothing to say.... I just want to learning DirectX 12 and try to understand how it works.

## Prerequisites
You need to enable the Windows developer mode to allow the symbolic link creation. Here is how to do it:
1. Open powershell as administrator
2. Run the following command:
```bash
$ reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock" /t REG_DWORD /f /v "AllowDevelopmentWithoutDevLicense" /d "1"
```

## Get Started
```bash
$ git clone --recursive https://github.com/ThIsJaCk23657689/LearningDX12.git
```

```bash
$ cd LearningDX12
$ cmake -S . -B out/debug -D CMAKE_BUILD_TYPE=Debug -G Ninja
$ cmake --build out/debug
```
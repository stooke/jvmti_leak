Linux:

Call with

```
LEAK_MMAP=1 LEAK_MALLOC=1 java -agentpath:/path/to/leaker.so <program> <args>
```


Windows:

Agent DLL:

```
set JDK_HOME=C:/work/jdk/build/fastdebug/images/jdk
cl /Zi -LD -I %JDK_HOME%/include -I %JDK_HOME%/include/win32 leaker.cpp
%JDK_HOME%\bin\java -agentpath:leaker.dll -version
```

Executable:

```
cl /Zi -I -I %JDK_HOME%/include -I %JDK_HOME%/include/win32 -fsanitize=address leaker.cpp
.\leaker.exe
```

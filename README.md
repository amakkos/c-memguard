# c-memguard
Memory overflow, leakage detector for C

[MemGuard](https://github.com/amakkos/memguard) is a memory overflow, leakage detector.

That tool was made to get help to detect and eliminate memory issues during run-time for long-run big project - when copiler warnings do not help. 
MemGuard may avoid you from crash by allocating slightly greater block than you asked. But is not tuned that way. 
The main design goal is save time on using debugger on core dump.  
> Does not support static allocation like char[12].  
> If you still want to debug those code lines, you can change it to malloc.

## Features
- Allocates more memory, than requested. That may avoid crash. 
- Adds a ulong pattern at the end of allocation. That is verified after write. 
- Writes log/stderr to save all findings
- Prints summary report. That may be scheduled for each hour or at exit.


## How do I build and run this?
There are just 2 files here to simplify to add to your project: 
- include memguard.h as the last include to make sure it over defines memory functions like malloc/realloc/sprintf.
- compile memguard.cc and link to your project  
test-memguard.cc is just an example how to use.

## How to use in dev but not use in prod
You may switch on/off the feature by (not) using define MEMGUARD by compiler.  
Switch on
```console
makki@makki-B450M-DS3H:~/Desktop/memguard$ gcc -DMEMGUARD -o test-memguard test-memguard.cc memguard.cc && ./test-memguard && echo rc=$?
2025.10.28 07:53:28 MemGuard: WARNING - overflow detected at sprintf in test-memguard.cc:main(28) in block allocated by test-memguard.cc:main(26)
Hello world!

MemGuard summary
Blocks: 1
Memory: 512
timestamp           size  chk file:function(line)      
=================== ===== === =========================
2025-10-28 07:53:28   512   1 test-memguard.cc:main(26) 'Hello world.'
rc=0
makki@makki-B450M-DS3H:~/Desktop/memguard$ 
```
Switch off
```console
makki@makki-B450M-DS3H:~/Desktop/memguard$ gcc -Wformat-overflow=0 -o test-memguard test-memguard.cc memguard.cc && ./test-memguard && echo rc=$?
Hello world!
rc=0
makki@makki-B450M-DS3H:~/Desktop/memguard$ 
```

## Logging
Define LOGDIR in your project to save findings. It not defined, stderr is used.

## License

Memguard is licensed under a 2-clause BSD license.

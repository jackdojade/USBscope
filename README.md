2-channel USBscope made of ATtiny45/85 MCU

Original project: [Yves Lebrac](http://yveslebrac.blogspot.com/2008/10/cheapest-dual-trace-scope-in-galaxy.html)

USBscope v4: [http://lugascan.com.ua/](http://lugascan.com.ua/)

You can find there:
- AD 14.3 PCB project
- SL 6 PCB project
- C source code for AVR MCU
- 2.0 C source code for AVR MCU with Atmel Studio 6 project
- C# source code for desktop application

---
Now USBscope has its own [VID/PID pair](http://pid.codes/1209/EBA7/)! Thanks [Arachnid](https://github.com/Arachnid) and [http://pid.codes/](http://pid.codes/) for it :3

`VID - 0x1209` `PID - 0xEBA7`

---

You can find two versions of the burnable firmware. [The first one](https://github.com/VictorGrig/USBscope/blob/master/AVR%20C%20source%20code/main.hex) - original and made by Yves Lebrac, [the second](https://github.com/VictorGrig/USBscope/blob/master/AVR%20C%20source%20code%202.0/Debug/main.hex) - brand new. It has Atmel Studio 6 valid project without errors that you can easily find during V-USB compilation. Also it has valid [VID/PID pair](http://pid.codes/1209/EBA7/) and built-in title `USBscope` and name `VictorGrigoryev` as an additional USB ID.

**WARNING! DO NOT FORGET TO SET THE FUSES WHILE PROGRAMMING!**

`-U lfuse:w:**0xE1:m -U hfuse:w:0xDD:m -U efuse:w:0xFF:m`

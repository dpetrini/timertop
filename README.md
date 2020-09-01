# timertop
Nostalgic project for Linux Kernel 2.6

This is a tool to measure how many times some timer was fired from linux Kernel (and thus discover which timers prevented you from going deep sleep longer). It is useful if you are improving power management in embedded systems. This project was suggested and contributed by other people like Tony Lindgren and Con Colivas. It was discussed in Linux Omap mail lists very far away. It was part of some other big submission 'dyn-ticks' patch. But for some reasons it was not included in any kernel version.

Anyway I resurrected it and put it here so it may be useful for someone else. 
It consists of a C-code command line tool (timertop) which relies in a kernel patch (also here) that exposes timers ocurrences in /proc in kernel.2.6.15 discussions, etc, etc.

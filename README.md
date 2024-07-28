# riscosbasic

Small tool to convert RISC OS' tokenised BASIC format (&FFB) to and from plain text.

E.g.

$ riscosbasic --decode ~/arculator/hostfs/BASICFile,ffb<br/>
10 PRINT "Hello"<br/>
20 PRINT "World"<br/>
30 <br/>
40 FOR x% = 1 TO 3<br/>
50 PRINT "!"<br/>
60 NEXT x%

$ echo '10 PRINT "Example"\n20PRINT"Encoding"' | riscosbasic --encode > ~/arculator/hostfs/BASICFile2,ffb

A --decode-nonumber option is also provided to generate a file without line numbers - however, this will currently bail if the source file does not increase in uniform 10s. Encoding can handle numbered or numberless input.

This program is designed to be minimal and simple. If you have a more complex feature in mind, have a look at https://github.com/ZornsLemma/basictool as it might be better-suited to your needs.

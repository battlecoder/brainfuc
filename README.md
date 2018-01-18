# brainfuc
Very simple brainfuck interpreter for Arduino with I/O operations.

Loosely based on my <a href="http://blog.damnsoft.org/brainfuck-on-arduino-end-of-file/" target="_blank">BOA</a> experiment. Unlike that project, however, this implementation does not depend on an external RAM IC, nor my low-level SD/FAT routines.

This code implements the standard brainfuck instruction set plus 2 special instructions:
<ul>
  <li> : (colon) Outputs value of current memory cell to digital pins 2 to 9.</li>
  <li> ; (semi-colon) Reads the binary value of pins 2 to 9 and stores the result in the current memory cell.</li>
</ul>

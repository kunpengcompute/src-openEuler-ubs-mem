### Notes:
* For hardware accelerated crc, you need to do something to enable it
* If you encountered following compile error:
```
/tmp/cc2oo0t5.s: Assembler messages:
/tmp/cc2oo0t5.s:574: Error: selected processor does not support `crc32cx w0,w0,x1'
/tmp/cc2oo0t5.s:632: Error: selected processor does not support `crc32cw w0,w0,w1'
/tmp/cc2oo0t5.s:690: Error: selected processor does not support `crc32ch w0,w0,w1'
/tmp/cc2oo0t5.s:748: Error: selected processor does not support `crc32cb w0,w0,w1'
```

To fix this, add following line into cmakelist
```
-march=armv8-a+crc
```
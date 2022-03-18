port stdstream : 0;
port binstream : 2 << 8;

proc outbin(val d) is 
{   binstream ! d and 255
;   binstream ! d >> 8
}

proc printn(port out, val n) is 
{   if n > 9 then printn(out, n/10) else skip
;   out ! (n rem 10) + '0'
}

proc printhex(port out, val n) is
  var d;
{   if n > 15 then printhex(out, n >> 4) else skip
;   d := n and 15
;   if d < 10 
    then out ! d + '0'
    else out ! (d - 10) + 'a'
}

func getchar() is
    var c;
{   stdstream ? c ; return c
}

proc fopen() is 
{   binstream ! -1
;   binstream ! 0
}

proc main() is 
{   printhex(stdstream, 0xDEAD)
;   printhex(stdstream, 0xBEEF)
}

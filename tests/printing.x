val bytesperword := 4;
port stdout : 0;

proc putval(val c) is
    stdout ! c

proc newline is
   putval('\n')

proc printn(val n) is { 
    if n > 9 then printn(n/10) else skip;
    putval((n rem 10) + '0')
}

proc printhex(val n) is
    var d;
{ 
    if n > 15 then printhex(n >> 4) else skip;
    d := n and 15;
    if d < 10 then putval(d + '0') else putval((d - 10) + 'a')
}

proc prints(s[]) is
    var n;
    var p;
    var w;
    var l;
    var b;
{ 
    n := 1;        
    p := 0;       
    w := s[p];     
    l := w and 255;
    w := w >> 8;  
    b := 1;  

    while (n <= l)
    do { 
        putval(w and 255);
        w := w >> 8; 
        n := n + 1;
        b := b + 1;
        if (b = bytesperword)
        then { 
            b := 0;
            p := p + 1;
            w := s[p]
        }
        else skip
    }  
}

proc main is 
{
    prints("hello world\n")
}


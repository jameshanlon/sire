const bytesperword := 4;
port outstream : 0 << 8;

proc putval(val c) is 
    outstream ! c

proc newline() is 
    putval('\n')

proc prints(s[]) is
    var n;
    var p;
    var w;
    var l;
    var b;
{ 
    n := 1;         % character index
    p := 0;         % array index
    w := s[p];      % word
    l := w and 255; % first byte: length
    w := w >> 8;    % character
    b := 1;         % byte

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

proc printn(val n) is
{ 
    if n > 9 
    then printn(n/10) 
    else skip;
    putval((n rem 10) + '0')
}

proc printhex(val n) is
    var d;
{ 
    if n > 15 
    then printhex(n >> 4) 
    else skip;
    
    d := n and 15;
    
    if d < 10 
    then putval(d + '0')
    else putval((d - 10) + 'a')
}

proc main() is 
    prints("hello world\n")

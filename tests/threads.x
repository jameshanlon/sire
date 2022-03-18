%port stdout : 0;
%val LENGTH := 26;
%var foo[LENGTH];

%proc putval(val c) is
%    stdout ! c
%
%proc newline() is
%   putval('\n')
%
%proc printn(val n) is 
%{ if n > 9 then printn(n/10) else skip
%; putval((n rem 10) + '0')
%}
%
%proc fill(bar[], val base) is
%  var i;
%{ for i:=0 to LENGTH-1 do
%    bar[i] := base + i
%}
%
%proc dump(baz[]) is 
%  var i;
%{ for i:=0 to LENGTH-1 do 
%  { putval(baz[i])
%  ; newline()
%  }
%}

proc foo() is 
    var i;
{ for i:=1 to 10 do skip
}

proc main() is
{ 
    foo()
  | foo()
  | foo()
}

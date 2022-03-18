const NUMCORES:=64,
    DIMENSION:=6;

%proc balance(val d) is
%    val myload;
%{ { chan[d] ! myload
%  | chan[d] ? hisload
%  }
%; move |myload-hisload| / 2 processes to/from here
%}

proc barrier(id: int) is
  var d, v: int;
  for d:=0 to DIMENSION-1 do 
      if(id > (id xor (1<<d)))
      then { chan[d] ! 0; chan[d] ? v }
      else { chan[d] ? v; chan[d] ! 0 }

proc node(id: int) is
  var i: int;
  for i:=0 to DIMENSION-1 do
    connect chan[i] to core[id xor (1<<i)] : chan[i]

proc dist(t, n: int) is
    if n = 1 then node(t)
    else { dist(t, n/2) 
        | on core[t+(n/2)] : dist(t+(n/2), n/2) }

proc main() is
    dist(0, NUMCORES)

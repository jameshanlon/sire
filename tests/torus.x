const NUMCORES:=4, DIM:=2,
    N:=0, E:=1, S:=2, W:=3; 
port led : 0x00010600;

proc torus(id: int) is 
{   connect chan[N] to core[(id+1) rem DIM] : chan[S];
    connect chan[S] to core[(id-1) rem DIM] : chan[N];
    connect chan[E] to core[(id+DIM) rem NUMCORES] : chan[W];
    connect chan[W] to core[(id+DIM) rem NUMCORES] : chan[E]
}

proc node(id: int) is
{   torus(id)
    % do something...
}

proc dist(t, n: int) is
    if n = 1 then node(t)
    else { dist(t, n/2) 
        | on core[t+(n/2)] : dist(t+(n/2), n/2) }

proc main() is
{ dist(0, NUMCORES)
}

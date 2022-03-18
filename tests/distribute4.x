const NUMCORES:=64; 
port led : 0x00010600;

proc d(t, n: int) is
    var a: int;
    if n = 1 then led ! 1
    else 
    { a := n>>2;
      { d(t, a)
      | on core[t+a]     : d(t+a,     a)
      | on core[t+(2*a)] : d(t+(2*a), a)
      | on core[t+(3*a)] : d(t+(3*a), a)
      }
    }

proc main() is 
{
    d(0, NUMCORES)
}

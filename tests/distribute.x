const NUMCORES := 64 
port led : 0x00010600

proc d(t, n: int) is
  if n = 1 then led ! 1
  else 
  { d(t, n/2) 
  | on core[t+(n/2)] : d(t+(n/2), n/2)
  }

proc main() is 
  d(0, NUMCORES)

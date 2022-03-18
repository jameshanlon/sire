% Iterative calculation
func fibItr (n: int) is 
  var x, y, z: int;
{ x := 0 ; y := 1
; while(n > 0) do 
  { z := x + y
  ; x := y
  ; y := z
  ; n := n - 1
  }
; return z
}

% Recursive calculation
func fibRec(n: int) is {
  if n > 1 
  then return fibRec(n-1) + fibRec(n-2)
  else if n = 0 then return 0 else return 1
}

proc main() is 
  fibItr(10);

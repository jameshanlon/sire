const MAX := 10000;

func testPrime (x: int) is
  var n, result: int;
{ result := true
; if (x rem 2) = 0 then return false else skip
; n := 3
; while n < (x >> 1) do 
  { if (x rem n) = 0 then result := false else skip
  ; n := n + 2
  }
; return result
}

proc main() is
  var i: int
  for i:=1 to MAX do testPrime(i)

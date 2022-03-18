const LENGTH := 100;
var a: int[LENGTH];

% Bubble sort
proc bsort(a: int[]; len: int) is
  var i, j, tmp: int;
  for i:=0 to len-1 do 
    for j:=0 to len-1 do 
      if a[j] > a[j+1] then 
      { tmp := a[j]
      ; a[j] := a[j+1]
      ; a[j+1] := tmp
      }
      else skip;

proc main() is
  var i;
{ for i:=0 to LENGTH-1 do a[i] := LENGTH-i
; msort(a, LENGTH)
}

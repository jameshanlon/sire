const LENGTH    := 1500, 
      THRESHOLD := 1000, 
      NUMCORES  := 64;
port led : 0x00010600;
var a: int[LENGTH];

% Merge
proc merge(a, b, c: int[]; la, lb: int) is
  var i, j, k: int;
{ i:=0 ; j:=0 ; k:=0
; while (i<la) and (j<lb) do 
  { if(a[i] <= b[j]) 
    then 
    { c[k] := a[i] 
    ; i:=i+1 ; k:=k+1
    } 
    else 
    { c[k] := b[j]
    ; j:=j+1 ; k:=k+1
    }
  }
; if i<la then c[k] := a[i] else skip
; if j<lb then c[k] := b[j] else skip
}

% Merge sort
proc msort(t, n: int; array: int[]; len: int) is
  var a, b: int[]; i, j: int
{ led ! 1
; if len > 1 then 
  { i := len / 2
  ; j := len - i
  ; a aliases array[0..]
  ; b aliases array[i..]
  ; if len > THRESHOLD then
    { msort(t, n/2, a, i) 
    | on core[t+(n/2)] : msort(t+(n/2), n/2, b, j) 
    }
    else 
    { msort(t, n/2, a, i) 
    ; msort(t+(n/2), n/2, b, j) 
    }
  ; merge(a, b, array, i, j)
  }
  else skip
; led ! 0
}

% Main
proc main() is
  var i: int;
{ for i:=0 to LENGTH-1 do a[i] := LENGTH-i
; msort(0, NUMCORES, a, LENGTH)
}

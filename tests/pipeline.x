proc node(i, len: int; left, right: chanend) is
  var received: int
{ received := 0
; if i > 0   then left  ? received     else skip
; if i < len then right ! received + i else skip
}

proc pipe(i, len: int; left, right: chanend) is
  var middle: chan
{ if i = (len - 1)
  then node(i, len, left, right)
  else 
  { node(i, len, left, middle)
  | pipe(i + 1, len, middle, right)
  }
}

proc main() is
  var c: chan;
{ pipe(0, 16, c, c)
}

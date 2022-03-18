val LEN := 10;

proc node(i, j: int; up, down, left, right: chanend)
is
{ % blah
}

proc matrix() is
  var i, j: int; v: chan[LEN]
{ par i:=1 to LEN do
    par j:=1 to LEN do
      node(i, j, v[i-1,j], v[i,j], h[i,j-1], h[i,j]);
}

proc main() is
{ matrix()
}

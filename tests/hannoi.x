proc hannoi(n, src, dst, using: int) is 
  if n = 1 then 
  { hannoi(n-1, src, using, dst)
  ; hannoi(n-1, using, dst, src)
  }
  else skip

proc main() is
  hannoi(10, 0, 2)

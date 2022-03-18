func factorial(x: int) is
   if x = 0
   then return 1 
   else return x * factorial(x-1)

proc main() is
  factorial(5)

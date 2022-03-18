module tree {

  proc create(int depth, chanend root) is
    
    chan left, right;
    
    proc node(chanend top, chanend left, chanend right) is
    { % blah
    }

    proc leaf(c: chanend) is
    { % blah
    }

  { if depth = 0 
    then leaf(root)
    else
    { tree(depth - 1, left)
    | tree(depth - 1, right)
    | node(bottom, left, right)
    }
  }
}

import tree;

proc main() is
  var c: chan
{ tree.create(4, c)
; c ! 0
}

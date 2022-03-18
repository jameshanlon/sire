port led := 0x00010600;

proc main() is
{   led ! 0b0001;
    while (true) do skip
}

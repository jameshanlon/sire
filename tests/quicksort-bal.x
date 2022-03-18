const LENGTH:=50, 
    THRESHOLD:=10, 
    NUMCORES:=16
var a: int[LENGTH]

% Partition
proc partition(var a: int[]; len, i: int):
    var i, j, ai, key: int;
{
    i := 0; j:=len;
    key := a[len/2];
    while i <= j do
    {
        while a[i] < key do i:=i+1;
        while key < a[j] do j:=j-1;
        if i <= j then
        {
            ai := a[i];
            a[i] := a[j];
            a[j] := ai;
            i:=i+1; j:=j+1;
        }
        else skip
    }
}

% Find
proc find(var array: int[]; len, middle: int):
    var left: int;
{
    left := 0;
    while left < len do
    {
        partition(array, len);
        if(middle <= j) then right := j
        else if i <= middle then left := i
        else left := right
    }
}

% Quicksort
proc qsort(t, n: int; array: int[]; len: int):
    var pivot: int; a, b: int[];
{   
    if len > 1 
    then
    { 
        middle := len/2;
        find(array, middle);

        a aliases array[0..];
        b aliases array[middle+1..];

        if len > THRESHOLD then
        { qsort(t, n/2, a, middle) 
        | on t+(n/2) do qsort(t+(n/2), n/2, b, len-(middle+1)) }
        else
        { qsort(t, n/2, a, middle) 
        ; qsort(t+(n/2), n/2, b, len-(middle+1)) }
    }
    else skip
}

proc main():
    var i: int;
{
    for i:=0 to LENGTH-1 do a[i] := LENGTH-i;
    qsort(0, NUMCORES, a, LENGTH)
}

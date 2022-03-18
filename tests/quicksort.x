const LENGTH:=50, 
    THRESHOLD:=10, 
    NUMCORES:=16
var a: int[LENGTH]

% Partition
proc partition(array: int[]; len: int) is
    var storeIndex, pivot, pivotValue, hold, i: int
{   
    pivot      := len/2;
    pivotValue := array[pivot];
    storeIndex := 0;
    
    % Move pivot to end
    array[pivot] := array[len-1];
    array[len-1] := pivotValue;

    % Reorder list (upto the pivot)
    for i:=0 to len-2 do {
        if array[i] <= pivotValue
        then {
            hold := array[i];
            array[i] := array[storeIndex];
            array[storeIndex] := hold;
            storeIndex := storeIndex + 1
        }
        else skip
    };

    % Move pivot to its final place: swap(storeIndex,pivot)
    hold := array[storeIndex];
    array[storeIndex] := array[len-1];
    array[len-1] := hold;

    return storeIndex
}

% Quicksort
proc qsort(t, n: int; array: int[]; len: int) is
    var pivot: int; a, b: int[]
{   
    if len > 10 
    then { 
        pivot := partition(array, len);

        a aliases array[0..];
        b aliases array[pivot+1..];

        if len > THRESHOLD then
        { qsort(t, n/2, a, pivot) 
        | on t+(n/2) do qsort(t+(n/2), n/2, b, len-(pivot+1)) }
        else
        { qsort(t, n/2, a, pivot) 
        ; qsort(t+(n/2), n/2, b, len-(pivot+1)) }
    }
    else skip
}

proc main() is
    var i: int
{
    for i:=0 to LENGTH-1 do a[i] := LENGTH-i;
    qsort(0, NUMCORES, a, LENGTH)
}

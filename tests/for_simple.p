program for_simple;
   type array_t = array[0..25] of integer;
var a,b,c,d,e,k,x,y,z : integer;
   A,B,C,D,E,F          : array_t;

{
function try_me(i,j : integer): integer;
begin
   i := i+j;
   try_me := i+4;
end;
}

{
procedure for_read_from_array(i,j : integer);
begin
   for k := 0 to e do
   begin
      B[k] := A[k];
      C[k] := B[k];
   end;
end;
}

begin
   a := 5;
   b := 10;
   c := 15;
   d := 20;
   e := 25;

   for k := 0 to e do
   begin
      {A[k] := a;
      B[k] := A[k];
      C[k] := B[k];}

      {
      x := A[k];
      z := D[k];
      B[k] := y;
      C[k] := x + z + B[k];
      }

      x := A[k];
      B[k] := y;
      C[k] := x; { + B[k];}
      x := D[k];
      E[k] := y;
      F[k] := x; { + C[k];}

      {      x := k;}
{      x := a + b * try_me(a,e) * c - d + e * 3;
      z := a + b * try_me(a,e) * z - d + e * 3;}
   end;

end.

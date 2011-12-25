program test2;
var a,b,c,d,x: integer;
function try_me(i,j:integer): integer;
begin
  i := i+j;
   try_me := i+4;
end;
begin
  a := 5;
  b := 10;

  c := 1 + try_me(a,b) + a;
  {d := a + x + 2*a + 8 - 3 * x ;}
end.
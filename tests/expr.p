program test1;
var a,b,c,d,e,x,z: integer;
function try_me(i,j:integer): integer;
begin
  i := i+j;
   try_me := i+4;
end;
begin
  a := 5;
  b := 10;
   c := 15;
   d := 20;
   e :=25;

   x := a + b * try_me(a,e) * c - d + e * 3;
   z := a + b * try_me(a,e) * z - d + e * 3;

   z := x + 7;

end.
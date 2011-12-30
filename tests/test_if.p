program test1;
var a,b,c,d,e,x: integer;
function try_me(i,j:integer): integer;
begin
  i := i+j;
   try_me := i+4;
end;
begin
  a := 5;
  b := 10;

  if ( x > a ) then
     a:=700
  else
     begin
        a:=100;
        b:=200;
        c:=300;
     end
end.
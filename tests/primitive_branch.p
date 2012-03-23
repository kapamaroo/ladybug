program test_labels;
var a,b,c,d,e,x: integer;
function try_if1(i,j:integer): integer;
begin
  if ( x > a ) then
     begin
     a:=700
     end
  else
     begin
     a:=100;
     b:=200;
     c:=300;
     end;
   try_if1:=b;
end; { try_if1 }

begin
   a := 5;
   b := 10;

   e:=try_if1(a,b);
end.
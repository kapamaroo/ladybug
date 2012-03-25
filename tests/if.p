program test_if;
var a,b,c,d,e,x: integer;
function try_if1(i,j:integer): integer;
begin
  if ( x > a ) then
     begin
     a:=700
     end
  else
     begin
     a:=100
     {b:=200;
     c:=300;}
     end;
   try_if1:=111
end; { try_if1 }

function try_if2(i,j:integer): integer;
begin
  if ( x > a ) then
     begin
     a:=700;
     b:=700;
     c:=700
     end
  else
     begin
        a:=100;
        b:=200;
        c:=300
     end;
   try_if2:=222
end; { try_if2 }

procedure proc_1(i,j:integer);
begin
  if ( x > a ) and (b = x) then
     begin
     a:=700;
     b:=800;
     c:=900;
     end
  else
     begin
        a:=100;
        b:=200;
        c:=300;
     end;
   c:=300;

end; { proc_1 }

begin
   a := 5;
   b := 10;

   e:=try_if1(a,b);
   proc_1(c,x);
   x:=try_if2(c,d)
end.
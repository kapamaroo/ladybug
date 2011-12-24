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

  c := try_me(a,b);
  d := a - x;
  c := 1 + try_me(a,b);
  d := a - x;
  c := try_me(a,b) + 4;

  if ( x > a ) then
    begin
     d := x + 5 ;
     e := 6;
     a := 9;
    end
  else
      x := a*3;


  if ( x <= a ) then
    begin
     d := x + 5 ;
     e := 6;
     a := 9;
    end
  else
    begin
      x := a*3;
  end

end.
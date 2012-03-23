program test_obsolete;
var a,b,c,d,e,x,y,w,z,q,r : integer;
   f,g,h,k,l,m,n,o  : integer;
function obsolete(i,j: integer): integer;
begin
   i := i+j;
   obsolete := 4;
end; { obsolete }

function try_me(i,j:integer): integer;
begin
   i := i+j;
   try_me := i+4;
end; { try_me }

begin
   a := 5;
   b := 10;
   c := 15;
   d := 20;
   e := 25;

   x := a + b * obsolete(a,e) * c - d + e * 3;
   a := try_me(f,d);
end.
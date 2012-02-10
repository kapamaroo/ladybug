program test_boolean;
var a,b,c,d,e : integer;
   x          : boolean;
begin
   a := 5;
   b := 10;
   c := 15;

   if (a > b) or (b < c) then
      a := 444
   {else
      a := 888
}
end.
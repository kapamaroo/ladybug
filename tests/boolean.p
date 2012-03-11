program test_boolean2;
var a,b,c,d,e : integer;
   x          : boolean;
begin
   a := 5;
   b := 10;
   c := 15;

   if (a > b) or (b < c) then
      a := 444;
      b := 555;
   {else
      c := 888
}
end.
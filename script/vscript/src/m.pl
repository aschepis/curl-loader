#!/usr/bin/perl

print "flex....";

if (system("flex lex.l")) {
  print("Failed");
  exit(1);
} else {
  system("mv -f lex.yy.c lex.yy.txt");
}
print "ok\n";

print "bison...";

if (system("bison  -d scr.y")) {
  print("Failed");
  exit(1);
} else {
  system("mv -f scr.tab.c scr.tab.txt");
}

print "ok\n";

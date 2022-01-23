#include <windows.h>
#include <string.h>

int main() {
system("cls");
char s[] = "Hello\n";
char asciiSet[] = " .,-+#$@";
char a[1];
HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
COORD pos;
unsigned long cChars;

WriteConsole(hStdout, s, lstrlen(s), &cChars, NULL);


pos.X = 9;
pos.Y = 1;
SetConsoleCursorPosition(hStdout, pos);
a[0] = asciiSet[4];
WriteConsole(hStdout, a, 1, &cChars, NULL);
return 0;

}
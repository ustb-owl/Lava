#include <stdio.h>

int getint() {
  int i;
  scanf("%d", &i);
  return i;
}

int getch() {
  char c;
  scanf("%c", &c);
  return c;
}

int getarray(int a[]) {
  int n;
  scanf("%d", &n);
  for (int i = 0; i < n; ++i) {
    scanf("%d", &a[i]);
  }
  return n;
}

void putint(int a) {
  printf("%d", a);
}

void putch(int a) {
  putchar(a);
}

void putarray(int n, int a[]) {
  printf("%d:", n);
  for (int i = 0; i < n; ++i) {
    printf(" %d", a[i]);
  }
}

// do nothing
void _sysy_starttime(int lineno) {}
void _sysy_stoptime(int lineno) {}

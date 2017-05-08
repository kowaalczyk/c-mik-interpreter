/* Krzysztof Kowalczyk | 385830 */

/* Poprawna wersja:
    - poprawione bledy zwiazane z wczytywaniem danych
    - poprawione core_dump (int zamiast char)
    - poprawione wczytywania (zera wiodace)
 */

#include <stdio.h>
#include <stdlib.h>

#define DEXLENGTH 32 /* Liczby dex nie powinny byc wieksze niz 16 bitow, ale 32 to bezpieczny margines */
#define CORE_DUMP 0
#define GET_INT 1
#define PUT_INT 2
#define GET_CHAR 3
#define PUT_CHAR 4
#define PUT_STRING 5

typedef int bool;

typedef struct mikState {
  unsigned char reg[16]; /* Rejestr */
  unsigned char mem[256]; /* Pamięć danych */
  unsigned short int ins[256]; /* Pamięć instrukcji */
  unsigned int pc; /* Wskaźnik na nast. instrukcję (0 <= pc <= 255) */
} state; /* Przechowuje stan maszyny mik */

unsigned int mod16(int a) {
  /* Oblicza mod z liczby a zgodnie ze specyfikacja z zadania */
  int c = a % 16;
  return (unsigned int)((c < 0) ? c + 16 : c);
}

unsigned int mod256(int a) {
  /* Oblicza mod z liczby a zgodnie ze specyfikacja z zadania */
  int c = a % 256;
  return (unsigned int)((c < 0) ? c + 256 : c);
}

unsigned char join(unsigned char a, unsigned char b) {
  /* sklejenie liczb a i b, zgodnie ze specyfikacja z zadania */
  return (a*16+b);
}

int valueDex(unsigned char c, unsigned int *base) {
  /* Zwraca wartos  c cyfry deksarnej c i zmienia odpowiednio baze */
  if(c>='0' && c<='9') {
    /* dec */
    *base = 10;
    return c-'0';
  } else if (c>='A' && c<='P') {
    /* hex */
    *base = 16;
    return c-'A';
  } else if (c>='Q' && c<='X') {
    /* oct */
    *base = 8;
    return c-'Q';
  } else if (c>='Y' && c<='Z') {
    /* bin */
    *base = 2;
    if(c=='Y') {
      return 1;
    } else {
      return 0;
    }
  } else {
    /* W pozostalych przypadkach */
    /* Nieprawodlowa cyfra - nic nie rob */
    return -1;
  }
}

int fromDex(unsigned char num[], unsigned int n) {
  /* konwertuje liczbe dex z tablicy num do zwracanej liczby int */
  unsigned int i;
  unsigned int currentBase;
  unsigned int ans = 0;
  for(i=0; i<n; i++) {
    int pom;
    pom = valueDex(num[i], &currentBase);
    ans = ans*currentBase + pom;
  }
  return ans;
}

void loadIntTab(unsigned short int *tab, unsigned int *i, unsigned int imax) {
  /* funkcja wczytujaca dane do tablicy (liczb calkowitych):
      tab - tablica do ktorej zapisywane sa dane z wejscia
      i - iterator od ktorego zaczynamy wpisywanie, po zakonczeniu funkcji wskazuje na pierwsze 0
      imax - maksymalna dlugosc tablicy
   */
  unsigned int c, j;
  bool increase_i; /* poprawa zer wiodacych */
  unsigned char currentString[DEXLENGTH];
  j=0;
  increase_i = 0;
  do {
    c = getchar();
    if(j==0 && (c==(int)'A' || c==(int)'0' || c==(int)'Q' || c==(int)'Z')) {
      increase_i = 1;/* ignoruj zera wiodace */
    } else if((c >= (int)'0' && c <= (int)'9') || (c >= (int)'A' && c <= (int)'Z')) {
      currentString[j] = (unsigned char)c;
      j++;
      increase_i = 1;
    } else if(increase_i) {
      if(j>0) {
        tab[*i] = fromDex(currentString, j);
        j=0;
      } else {
        tab[*i] = 0;
      }
      (*i)++;
      increase_i = 0;
    }
  } while (c != '%');
  /* uzupelnianie tablicy zerami */
  for(j=*i; j<imax; j++) {
    tab[j] = 0;
  }
}

void loadCharTab(unsigned char *tab, unsigned int *i, unsigned int imax) {
  /* funkcja wczytujaca dane do tablicy (char), dziala tak jak loadIntTab */
  unsigned int c, j;
  bool increase_i; /* poprawa zer wiodacych */
  unsigned char currentString[DEXLENGTH];
  j=0;
  increase_i = 0;
  do {
    c = getchar();
    if(j==0 && (c==(int)'A' || c==(int)'0' || c==(int)'Q' || c==(int)'Z')) {
      increase_i = 1;/* ignoruj zera wiodace */
    } else if((c >= (int)'0' && c <= (int)'9') || (c >= (int)'A' && c <= (int)'Z')) {
      currentString[j] = (unsigned char)c;
      j++;
      increase_i = 1;
    } else if(increase_i) {
      if(j>0) {
        tab[*i] = (unsigned char)fromDex(currentString, j);
        j=0;
      } else {
        tab[*i] = 0;
      }
      (*i)++;
      increase_i = 0;
    }
  } while (c != '%');
  /* uzupelnianie tablicy zerami */
  for(j=*i; j<imax; j++) {
    tab[j] = 0;
  }
}

void loadState(state *s) {
  unsigned int i=0;
  loadCharTab(s->reg, &i, 16);
  i=0;
  loadCharTab(s->mem, &i, 256);
  i=0;
  loadIntTab(s->ins, &i, 256);
  s->pc = i;
  loadIntTab(s->ins, &i, 256);
}

void core_dump(unsigned char c, state *s) {
  unsigned int i;
  for(i=0; i<16; i++) {
    fprintf(stderr, "%d \n", (int)(s->reg[i]));
  }
  fprintf(stderr, "%c\n", '%');
  for(i=0; i<256; i++) {
    fprintf(stderr, "%d \n", (int)(s->mem[i]));
  }
  fprintf(stderr, "%c\n", '%');
  for(i=0; i<(unsigned int)c; i++) {
    fprintf(stderr, "%d \n", (int)(s->ins[i]));
  }
  fprintf(stderr, "%c\n", '%');
  for(i=(unsigned int)c; i<256; i++) {
    fprintf(stderr, "%d \n", (int)(s->ins[i]));
  }
  fprintf(stderr, "%c\n", '%');
}

void doFunction(state *s) {
  int f, a, b, c; /* instrukcja i parametry */
  int tpc, tra, trb, trc; /*zmienne pomocnicze*/
  /* wczytanie parametrow */
  f = mod256((int)(s->ins[s->pc]) >> 12);
  a = mod256(((int)(s->ins[s->pc]) >> 8) & 15);
  b = mod256(((int)(s->ins[s->pc]) >> 4) & 15);
  c = mod256((int)(s->ins[s->pc]) & 15);
  /* ustawienie program countera */
  s->pc = mod256((int)(s->pc)+1);
  /* wykonanie odpowiedniej instrukcji */
  switch(f) {
    case 0:
      if(b!=c) {
        /*divide*/
        trc = (int)(s->reg[c]);
        if (trc != 0) {
            trb = (int)(s->reg[b]);
            s->reg[a] = (unsigned char)(trb / trc);
            s->reg[mod16(a+1)] = (unsigned char)(trb % trc);
        }
      }
      if(a!=b && b==c) {
        /*push*/
        s->mem[mod256((int)(--(s->reg[a])))] = s->reg[b];
      }
      if(a==b &&b==c) {
        /*halt*/
        exit(s->reg[a]);
      }
      break;
    case 1:
      if(a!=b && a!= c) {
        /*return*/
        tpc = (int)s->pc;
        s->pc = (unsigned int)s->mem[(unsigned int)(s->reg[a])];
        s->reg[a] += (unsigned char)(s->reg[c] + 1);
        s->reg[b] = (unsigned char)tpc;
      }
      if(a!=b && a==c) {
        /*pop*/
        s->reg[b] = (unsigned char)s->mem[mod256((int)(s->reg[a]++))];
      }
      if(a==b) {
        /*return from subroutine*/
        tpc = (int)s->pc;
        s->pc = (unsigned int)(s->reg[c]);
        s->reg[a] = (unsigned char)tpc;
      }
      break;
    case 2:
      if(b != c) {
        /*compare*/
        s->reg[a] = (s->reg[b] < s->reg[c]) ? 1 : 0;
      }
      if(b == c) {
        /*shift left*/
        s->reg[a] = (unsigned char)(mod256((int)(s->reg[b] << 1)));
      }
      break;
    case 3:
      if(b != c) {
        /*subtract*/
        s->reg[a] = s->reg[b] - s->reg[c]; /* TODO: Check if mod256 is required */
      }
      if(b == c) {
        /*shift right*/
        s->reg[a] = (unsigned char)(mod256((int)(s->reg[b] >> 1)));
      }
      break;
    case 4:
      if(b <= c) {
        /*load indexed*/
        s->reg[a] = s->mem[mod256((int)(s->reg[b] + s->reg[c]))];
      }
      if(b > c) {
        /*add*/
        s->reg[a] = s->reg[b] + s->reg[c];
      }
      break;
    case 5:
      if(b <= c) {
        /*store indexed*/
        s->mem[mod256((int)(s->reg[b] + s->reg[c]))] = s->reg[a];
      }
      if(b>c) {
        /*bitwise or*/
        s->reg[a] = s->reg[b] | s->reg[c];
      }
      break;
    case 6:
      if(b <= c) {
        /*multiply*/
        unsigned int x = s->reg[b] * s->reg[c];
        s->reg[mod16(a+1)] = (unsigned char)(x/256);
        s->reg[a] = (unsigned char)(x%256);
      }
      if(b>c) {
        /*bitwise and*/
        s->reg[a] = s->reg[b] & s->reg[c];
      }
      break;
    case 7:
      if(b <= c) {
        /*call indexed*/
        tpc = (int)(s->pc);
        s->pc = s->mem[mod256((int)(s->reg[b] + s->reg[c]))];
        s->reg[a] = (unsigned char)tpc;
      }
      if(b>c) {
        /*bitwise xor*/
        s->reg[a] = s->reg[b] ^ s->reg[c];
      }
      break;
    case 8:
      /*jump if zero*/
      if (s->reg[a] == 0) {
        s->pc = (unsigned int)join((unsigned char)b, (unsigned char)c);
      }
      break;
    case 9:
      /*jump if not zero*/
      if (s->reg[a] != 0) {
        s->pc = (unsigned int)join((unsigned char)b, (unsigned char)c);
      }
      break;
    case 10:
      /*call subroutine*/
      s->reg[a] = (unsigned char)(s->pc);
      s->pc = (unsigned int)join((unsigned char)b, (unsigned char)c);
      break;
    case 11:
      /*call*/
      s->mem[mod256((int)(--(s->reg[a])))] = (unsigned char)(s->pc);
      s->pc = (unsigned int)join((unsigned char)b, (unsigned char)c);
      break;
    case 12:
      /*load register*/
      s->reg[a] = s->mem[(unsigned int)join(b, c)];
      break;
    case 13:
      /*store register*/
      s->mem[(unsigned int)join(b, c)] = s->reg[a];
      break;
    case 14:
      /*load constant*/
      s->reg[a] = join((unsigned char)b, (unsigned char)c);
      break;
    case 15:
      /*system call*/
      switch (join((unsigned char)b, (unsigned char)c)) {
        case CORE_DUMP: /* 0 */
            core_dump(s->reg[a], s);
            break;
        case GET_INT: /* 1 */
            if (scanf("%d", &tra) != 1) {
              s->reg[a] = 0;
            } else {
              s->reg[mod16(a+1)] = 1;
              s->reg[a] = (unsigned char)tra;
            }
            break;
        case PUT_INT: /* 2 */
            printf("%d", (int)(s->reg[a]));
            break;
        case GET_CHAR: /* 3 */
            tra = getchar();
            if (tra == EOF) {
                s->reg[mod16(a+1)] = 0;
            } else { /* poprawa - brakowalo tego warunku */
                s->reg[mod16(a+1)] = 1;
                s->reg[a] = (unsigned char)tra;
            }
            break;
        case PUT_CHAR: /* 4 */
            putchar(s->reg[a]);
            break;
        case PUT_STRING: /* 5 */
            printf("%s", &(s->mem[(unsigned int)(s->reg[a])]));
            break;
        default:
            /* nic nie rób */
            break;
        }
      break;
  }
}

int main() {
  state currentState;
  loadState(&currentState);
  while(1) {
    doFunction(&currentState);
  }
  return 0;
}

#include <search.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void *root = NULL;

void *
xmalloc (unsigned n)
{
  void *p;
  p = malloc (n);
  if (p)
    {
      return p;
    }
  fprintf (stderr, "insufficient memory\n");
  exit (EXIT_FAILURE);
}

int
compare (const void *pa, const void *pb)
{
  if (*(int *) pa < *(int *) pb)
    {
      return -1;
    }
  if (*(int *) pa > *(int *) pb)
    {
      return 1;
    }
  return 0;
}

void
action (const void *nodep, const VISIT which, const int depth)
{
  int *datap;

  switch (which)
    {
    case preorder:
      break;
    case postorder:
      datap = *(int **) nodep;
      printf ("%6d\n", *datap);
      break;
    case endorder:
      break;
    case leaf:
      datap = *(int **) nodep;
      printf ("%6d\n", *datap);
      break;
    }
}

int
main (void)
{
  int i, *ptr;
  void *val;

  srand (time (NULL));
  for (i = 0; i < 12; i++)
    {
      ptr = (int *) xmalloc (sizeof(int));
      *ptr = rand () & 0xff;
      val = tsearch ((void *) ptr, &root, compare);
      if (val == NULL)
        {
          exit (EXIT_FAILURE);
        }
      else if ((*(int **) val) != ptr)
        {
        // Disabled because on gcc 5.3
//        réadressage R_X86_64_PLTOFF64 sans solution vers le symbole « free@@GLIBC_2.2.5 »
///usr/bin/ld.bfd.real : échec de l'édition de liens finale : Section non-représentable pour la sortie
//collect2: error: ld returned 1 exit status
//          free (ptr);
        }
    }
  twalk (root, action);
  tdestroy (root, free);
  exit (EXIT_SUCCESS);
}

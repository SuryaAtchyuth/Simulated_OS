#include "simos.h"

//===============================================================
// The interface to interact with clients for program submission
// --------------------------
// Should change to create server socket to accept client connections
// -- Best is to use the select function to get client inputs
// Should change term.c to direct the terminal output to client terminal
//===============================================================

void program_submission ()
{ char fname[100];

  fprintf (infF, "Submission file: ");
  scanf ("%s", &fname);
  if (uiDebug) fprintf (bugF, "File name: %s has been submitted\n", fname);
  submit_process (fname);
}



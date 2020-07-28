#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit() {
  wxInitAllImageHandlers();
  MainFrame* frame = new MainFrame(NULL, wxID_ANY, wxEmptyString);
  SetTopWindow(frame);
  frame->Show();
  return true;
}

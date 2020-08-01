#include "wx/checkbox.h"
#include "wx/frame.h"
#include "wx/gauge.h"
#include "wx/notebook.h"
#include "wx/panel.h"

#ifndef MAINFRAME_H
#define MAINFRAME_H

class MainFrame : public wxFrame {
 public:
  MainFrame(wxWindow* parent,
            wxWindowID id,
            const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE);

 private:
  void setProperties();
  void createLayout();
  wxPanel* createDisassemblyTab();
  wxPanel* createCompressionTab();
  wxPanel* createAssemblyTab();
  wxPanel* createVerificationTab();

  std::vector<wxString> inputPaths;
  wxString outputPath;
  int disassemblyTypeSelection = 0;
  // TODO - put the selection in an enum or something
  bool prettyPrint = false;
  bool goalTranspiling = false;

 protected:
  wxButton* compressButton;
  wxButton* compressionInputBrowse;
  wxButton* compressionOutputBrowse;
  wxButton* decompressButton;
  wxButton* disassembleButton;
  wxButton* disassemblyInputBrowse;
  wxButton* disassemblyOutputBrowse;
  wxCheckBox* goalTranspileCheckbox;
  wxCheckBox* prettyPrintCheckbox;
  wxGauge* compressionProgressBar;
  wxGauge* disassemblyProgressBar;
  wxNotebook* tabWindow;
  wxPanel* tabAssembly;
  wxPanel* tabCompression;
  wxPanel* tabDissassembly;
  wxPanel* tabVerification;
  wxRadioBox* compressionInputTypeSelect;
  wxRadioBox* disassemblyInputTypeSelect;
};

#endif

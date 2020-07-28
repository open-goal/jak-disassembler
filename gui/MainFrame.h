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

 protected:
  wxRadioBox* disassemblyInputTypeSelect;
  wxButton* disassemblyInputBrowse;
  wxButton* disassemblyOutputBrowse;
  wxCheckBox* prettyPrintCheckbox;
  wxCheckBox* goalTranspileCheckbox;
  wxButton* disassembleButton;
  wxGauge* disassemblyProgressBar;
  wxPanel* tabDissassembly;
  wxRadioBox* compressionInputTypeSelect;
  wxButton* compressionInputBrowse;
  wxButton* compressionOutputBrowse;
  wxButton* decompressButton;
  wxButton* compressButton;
  wxGauge* compressionProgressBar;
  wxPanel* tabCompression;
  wxPanel* tabAssembly;
  wxPanel* tabVerification;
  wxNotebook* tabWindow;
};

#endif

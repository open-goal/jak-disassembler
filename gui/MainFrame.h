#include "wx/checkbox.h"
#include "wx/frame.h"
#include "wx/gauge.h"
#include "wx/notebook.h"
#include "wx/panel.h"
#include "wx/listctrl.h"
#include "wx/treectrl.h"
#include "wx/scrolwin.h"
#include "Api.h"

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
  void updateDisassemblyListItem(int index, Api::DisassemblyInfo info);
  void initDisassemblyListItems();

  std::vector<std::string> inputPaths;
  wxString outputPath;
  int disassemblyTypeSelection = 0;
  // TODO - put the selection in an enum or something
  bool prettyPrint = false;
  bool goalTranspiling = false;

  std::vector<wxListItem> dgoFiles;

 protected:
  wxRadioBox* disassemblyInputTypeSelect;
  wxButton* disassemblyInputBrowse;
  wxButton* disassemblyOutputBrowse;
  wxCheckBox* prettyPrintCheckbox;
  wxCheckBox* goalTranspileCheckbox;
  wxButton* disassembleButton;
  wxGauge* disassemblyProgressBar;
  wxListCtrl* disassemblyOutputList;
  wxScrolledWindow* disassemblyOutputScrollPanel;
  wxPanel* tabDissassembly;
  wxRadioBox* compressionInputTypeSelect;
  wxButton* compressionInputBrowse;
  wxButton* compressionOutputBrowse;
  wxButton* decompressButton;
  wxButton* compressButton;
  wxGauge* compressionProgressBar;
  wxTreeCtrl* compressionOutputTreeList;
  wxScrolledWindow* compressionOutputScrollPanel;
  wxPanel* tabCompression;
  wxPanel* tabAssembly;
  wxPanel* tabVerification;
  wxNotebook* tabWindow;

  void Disassemble_Click(wxCommandEvent& evt);
};

#endif

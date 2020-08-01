#include "MainFrame.h"

#include "wx/button.h"
#include "wx/radiobox.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#include "Api.h"

MainFrame::MainFrame(wxWindow* parent,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style)
    : wxFrame(parent, id, title, pos, size, wxDEFAULT_FRAME_STYLE) {
  tabWindow = new wxNotebook(this, wxID_ANY);
  tabCompression = new wxPanel(tabWindow, wxID_ANY);
  tabDissassembly = new wxPanel(tabWindow, wxID_ANY);
  const wxString disassemblyInputTypeSelect_choices[] = {
      wxT("Game Folder"),
      wxT(".CGO/.DGO Files"),
      wxT(".go/.o Files"),
  };
  disassemblyInputTypeSelect =
      new wxRadioBox(tabDissassembly, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 3,
                     disassemblyInputTypeSelect_choices, 1, wxRA_SPECIFY_ROWS);
  disassemblyInputBrowse = new wxButton(tabDissassembly, wxID_ANY, wxT("Browse..."));
  disassemblyOutputBrowse = new wxButton(tabDissassembly, wxID_ANY, wxT("Browse..."));
  prettyPrintCheckbox = new wxCheckBox(tabDissassembly, wxID_ANY, wxEmptyString);
  goalTranspileCheckbox = new wxCheckBox(tabDissassembly, wxID_ANY, wxEmptyString);
  disassembleButton = new wxButton(tabDissassembly, wxID_ANY, wxT("Disassemble"));
  disassemblyProgressBar = new wxGauge(tabDissassembly, wxID_ANY, 10, wxDefaultPosition,
                                       wxDefaultSize, wxGA_HORIZONTAL | wxGA_SMOOTH);
  const wxString compressionInputTypeSelect_choices[] = {
      wxT("Game Folder"),
      wxT(".CGO/.DGO Files"),
      wxT(".go/.o Files"),
  };
  compressionInputTypeSelect =
      new wxRadioBox(tabCompression, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 3,
                     compressionInputTypeSelect_choices, 1, wxRA_SPECIFY_ROWS);
  compressionInputBrowse = new wxButton(tabCompression, wxID_ANY, wxT("Browse..."));
  compressionOutputBrowse = new wxButton(tabCompression, wxID_ANY, wxT("Browse..."));
  decompressButton = new wxButton(tabCompression, wxID_ANY, wxT("Decompress"));
  compressButton = new wxButton(tabCompression, wxID_ANY, wxT("Compress"));
  compressionProgressBar = new wxGauge(tabCompression, wxID_ANY, 10, wxDefaultPosition,
                                       wxDefaultSize, wxGA_HORIZONTAL | wxGA_SMOOTH);
  tabAssembly = new wxPanel(tabWindow, wxID_ANY);
  tabVerification = new wxPanel(tabWindow, wxID_ANY);

  setProperties();
  createLayout();

  std::vector<std::string> ye;
  ye.push_back("C:\\Users\\xtvas\\Repositories\\jak2-disassembler\\test\\COMMON.CGO");

  //Api::disassemble("C:\\Users\\xtvas\\Repositories\\jak2-disassembler\\test\\out", ye);
}

void MainFrame::setProperties() {
  SetTitle(wxT("Jak Disassembler and Tools"));
  disassemblyInputTypeSelect->SetSelection(0);
  compressionInputTypeSelect->SetSelection(0);
}

void MainFrame::createLayout() {
  // TODO - Probably only looks good on 4K
  SetMinSize(wxSize(800, 1000));
  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  tabWindow->AddPage(createDisassemblyTab(), wxT("Disassembly"));
  tabWindow->AddPage(createCompressionTab(), wxT("Compression"));
  tabWindow->AddPage(createAssemblyTab(), wxT("Assembly"));
  tabWindow->AddPage(createVerificationTab(), wxT("Verification"));

  mainSizer->Add(tabWindow, 1, wxEXPAND, 0);
  SetSizer(mainSizer);
  Layout();
}

wxPanel* MainFrame::createDisassemblyTab() {
  wxBoxSizer* disassemblySizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* disassemblyLogContainer = new wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer* disassemblyControlContainer = new wxFlexGridSizer(4, 2, 0, 0);
  wxStaticText* disassemblyControlsHeader =
      new wxStaticText(tabDissassembly, wxID_ANY, wxT("Controls"));
  disassemblyControlsHeader->SetFont(
      wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, 0, wxT("")));
  disassemblySizer->Add(disassemblyControlsHeader, 0, wxEXPAND | wxRIGHT, 5);
  disassemblyControlContainer->Add(disassemblyInputTypeSelect, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                                   5);
  disassemblyControlContainer->Add(disassemblyInputBrowse, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  wxStaticText* disassemblyOutputBrowseLabel =
      new wxStaticText(tabDissassembly, wxID_ANY, wxT("Output Folder"));
  disassemblyControlContainer->Add(disassemblyOutputBrowseLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                                   5);
  disassemblyControlContainer->Add(disassemblyOutputBrowse, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  wxStaticText* prettyPrintLabel = new wxStaticText(tabDissassembly, wxID_ANY, wxT("Pretty Print"));
  disassemblyControlContainer->Add(prettyPrintLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  disassemblyControlContainer->Add(prettyPrintCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  wxStaticText* goalTranspileLabel =
      new wxStaticText(tabDissassembly, wxID_ANY, wxT("Transpile To GOAL"));
  disassemblyControlContainer->Add(goalTranspileLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  disassemblyControlContainer->Add(goalTranspileCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  disassemblyControlContainer->AddGrowableRow(0);
  disassemblyControlContainer->AddGrowableRow(1);
  disassemblyControlContainer->AddGrowableRow(2);
  disassemblyControlContainer->AddGrowableRow(3);
  disassemblyControlContainer->AddGrowableCol(0);
  disassemblyControlContainer->AddGrowableCol(1);
  disassemblySizer->Add(disassemblyControlContainer, 1, wxEXPAND, 0);
  disassemblySizer->Add(disassembleButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  disassemblySizer->Add(disassemblyProgressBar, 0, wxALL | wxEXPAND, 5);
  wxStaticText* disassemblyLogHeader =
      new wxStaticText(tabDissassembly, wxID_ANY, wxT("Disassembly Information"));
  disassemblyLogHeader->SetFont(
      wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, 0, wxT("")));
  disassemblyLogContainer->Add(disassemblyLogHeader, 0, wxBOTTOM | wxEXPAND, 5);
  wxStaticText* disassemblyLogs =
      new wxStaticText(tabDissassembly, wxID_ANY,
                       wxT("Lots of TextOn Many Lines Lots of TextOn Many Lines Lots of TextOn "
                           "Many Lines Lots of TextOn Many Lines Lots of TextOn Many Lines Lots of "
                           "TextOn Many Lines Lots of TextOn Many Lines"));
  disassemblyLogs->Wrap(GetMinWidth());
  disassemblyLogContainer->Add(disassemblyLogs, 0, wxEXPAND | wxLEFT, 5);
  disassemblySizer->Add(disassemblyLogContainer, 2, wxEXPAND, 0);
  tabDissassembly->SetSizer(disassemblySizer);
  return tabDissassembly;
}

wxPanel* MainFrame::createCompressionTab() {
  wxBoxSizer* compressionSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* compressionLogContainer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* formSubmitContainer = new wxBoxSizer(wxHORIZONTAL);
  wxFlexGridSizer* compressionControlContainer = new wxFlexGridSizer(2, 2, 0, 0);
  wxStaticText* compressionControlsHeader =
      new wxStaticText(tabCompression, wxID_ANY, wxT("Controls"));
  compressionControlsHeader->SetFont(
      wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, 0, wxT("")));
  compressionSizer->Add(compressionControlsHeader, 0, wxEXPAND | wxRIGHT, 5);
  compressionControlContainer->Add(compressionInputTypeSelect, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                                   5);
  compressionControlContainer->Add(compressionInputBrowse, 1, wxALIGN_CENTER_VERTICAL, 5);
  wxStaticText* compressionOutputBrowseLabel =
      new wxStaticText(tabCompression, wxID_ANY, wxT("Output Folder"));
  compressionControlContainer->Add(compressionOutputBrowseLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                                   5);
  compressionControlContainer->Add(compressionOutputBrowse, 1, wxALIGN_CENTER_VERTICAL, 5);
  compressionControlContainer->AddGrowableRow(0);
  compressionControlContainer->AddGrowableRow(1);
  compressionControlContainer->AddGrowableCol(0);
  compressionControlContainer->AddGrowableCol(1);
  compressionSizer->Add(compressionControlContainer, 1, wxEXPAND, 0);
  formSubmitContainer->Add(decompressButton, 0, wxALL, 5);
  formSubmitContainer->Add(compressButton, 0, wxALL, 5);
  compressionSizer->Add(formSubmitContainer, 0, wxALIGN_CENTER_HORIZONTAL, 0);
  compressionSizer->Add(compressionProgressBar, 0, wxALL | wxEXPAND, 5);
  wxStaticText* compressionLogHeader =
      new wxStaticText(tabCompression, wxID_ANY, wxT("Decompression Information"));
  compressionLogHeader->SetFont(
      wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, 0, wxT("")));
  compressionLogContainer->Add(compressionLogHeader, 0, wxBOTTOM | wxEXPAND, 5);
  wxStaticText* compressionLogs =
      new wxStaticText(tabCompression, wxID_ANY, wxT("Lots of Text On Many Line"));
  compressionLogs->Wrap(GetMinWidth());
  compressionLogContainer->Add(compressionLogs, 0, wxEXPAND | wxLEFT, 5);
  compressionSizer->Add(compressionLogContainer, 2, wxEXPAND, 0);
  tabCompression->SetSizer(compressionSizer);
  tabCompression->Disable();
  return tabCompression;
}

wxPanel* MainFrame::createAssemblyTab() {
  tabAssembly->Disable();
  return tabAssembly;
}

wxPanel* MainFrame::createVerificationTab() {
  tabVerification->Disable();
  return tabVerification;
}

#include <future>
#include <sstream>
#include <thread>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include "MergedMiner.h"

class SoloMinerFrame : public wxFrame {
public:
  SoloMinerFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
    wxFrame(parent, id, title, pos, size, style) {

    wxPanel* panel = new wxPanel(this);

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxString donorNetworks[] = { "Bytecoin (BCN)", "BitMonero (BMR)", "QuazarCoin (QCN)" };
    d_donorNetworkRadioBox = new wxRadioBox(panel, wxID_ANY, "Donor network", wxDefaultPosition, wxDefaultSize, sizeof donorNetworks / sizeof donorNetworks[0], donorNetworks, 0, wxRA_SPECIFY_COLS);
    d_donorNetworkRadioBox->Bind(wxEVT_RADIOBOX, &SoloMinerFrame::onDonorNetworkChanged, this);
    sizer->Add(d_donorNetworkRadioBox, 0, wxALL, 5);

    wxSizer* donorWalletSizer = new wxBoxSizer(wxHORIZONTAL);
    donorWalletSizer->Add(new wxStaticText(panel, wxID_ANY, "Donor wallet address:"), 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 5);
    d_donorWalletTextCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    donorWalletSizer->Add(d_donorWalletTextCtrl, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
    sizer->Add(donorWalletSizer, 0, wxALL | wxGROW, 5);

    wxSizer* donorHostSizer = new wxBoxSizer(wxHORIZONTAL);
    donorHostSizer->Add(new wxStaticText(panel, wxID_ANY, "Donor host address:"), 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 5);
    d_donorHostChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    donorHostSizer->Add(d_donorHostChoice, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
    sizer->Add(donorHostSizer, 0, wxALL | wxGROW, 5);

    wxString acceptorNetworks[] = { "None", "Fantomcoin (FCN)" };
    d_acceptorNetworkRadioBox = new wxRadioBox(panel, wxID_ANY, "Acceptor network", wxDefaultPosition, wxDefaultSize, sizeof acceptorNetworks / sizeof acceptorNetworks[0], acceptorNetworks, 0, wxRA_SPECIFY_COLS);
    d_acceptorNetworkRadioBox->Bind(wxEVT_RADIOBOX, &SoloMinerFrame::onAcceptorNetworkChanged, this);
    sizer->Add(d_acceptorNetworkRadioBox, 0, wxALL, 5);

    wxSizer* acceptorWalletSizer = new wxBoxSizer(wxHORIZONTAL);
    acceptorWalletSizer->Add(new wxStaticText(panel, wxID_ANY, "Acceptor wallet address:"), 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 5);
    d_acceptorWalletTextCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    acceptorWalletSizer->Add(d_acceptorWalletTextCtrl , 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
    sizer->Add(acceptorWalletSizer, 0, wxALL | wxGROW, 5);

    wxSizer* acceptorHostSizer = new wxBoxSizer(wxHORIZONTAL);
    acceptorHostSizer->Add(new wxStaticText(panel, wxID_ANY, "Acceptor host address:"), 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 5);
    d_acceptorHostChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    acceptorHostSizer->Add(d_acceptorHostChoice, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
    sizer->Add(acceptorHostSizer, 0, wxALL | wxGROW, 5);

    wxSizer* threadCountSizer = new wxBoxSizer(wxHORIZONTAL);
    threadCountSizer->Add(new wxStaticText(panel, wxID_ANY, "Thread count:"), 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 5);
    d_threadCountChoice = new wxChoice(panel, wxID_ANY);
    unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    if (hardwareConcurrency == 0) {
      hardwareConcurrency = 2;
    }

    for (unsigned int i = 0; i < hardwareConcurrency; ++i) {
      std::ostringstream stream;
      stream << i + 1;
      d_threadCountChoice->Append(stream.str());
    }

    d_threadCountChoice->SetSelection(0);
    threadCountSizer->Add(d_threadCountChoice, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
    sizer->Add(threadCountSizer, 0, wxALL | wxGROW, 5);

    d_mineButton = new wxButton(panel, 100, "Start mining");
    d_mineButton->Bind(wxEVT_BUTTON, &SoloMinerFrame::onMineButton, this);
    sizer->Add(d_mineButton, 0, wxALL, 5);

    d_messagesTextCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, 300), wxTE_MULTILINE | wxTE_READONLY);
    sizer->Add(d_messagesTextCtrl, 0, wxALL | wxGROW, 5);

    panel->SetSizer(sizer);

    wxCommandEvent commandEvent;
    onDonorNetworkChanged(commandEvent);
    onAcceptorNetworkChanged(commandEvent);

    d_timer.Bind(wxEVT_TIMER, &SoloMinerFrame::onTimer, this);
    d_timer.Start(100);
    d_isMining = false;
  }

private:
  wxRadioBox* d_donorNetworkRadioBox;
  wxTextCtrl *d_donorWalletTextCtrl;
  wxChoice* d_donorHostChoice;
  wxRadioBox* d_acceptorNetworkRadioBox;
  wxTextCtrl *d_acceptorWalletTextCtrl;
  wxChoice* d_acceptorHostChoice;
  wxChoice* d_threadCountChoice;
  wxButton* d_mineButton;
  wxTextCtrl* d_messagesTextCtrl;
  wxTimer d_timer;
  MergedMiner d_mergedMiner;
  std::future<bool> d_mining;
  bool d_isMining;

  void onDonorNetworkChanged(wxCommandEvent& event) {
    if (d_donorNetworkRadioBox->GetSelection() == 0) {
      d_donorHostChoice->Clear();
      d_donorHostChoice->Append("127.0.0.1:8081");
      d_donorHostChoice->Append("93.190.142.146:8081");
      d_donorHostChoice->Append("217.23.12.74:8081");
      d_donorHostChoice->SetSelection(0);
      d_donorHostChoice->Enable();
    } else if (d_donorNetworkRadioBox->GetSelection() == 1) {
      d_donorHostChoice->Clear();
      d_donorHostChoice->Append("127.0.0.1:18081");
      d_donorHostChoice->SetSelection(0);
      d_donorHostChoice->Disable();
    } else {
      d_donorHostChoice->Clear();
      d_donorHostChoice->Append("127.0.0.1:23081");
      d_donorHostChoice->SetSelection(0);
      d_donorHostChoice->Disable();
    }
  }

  void onAcceptorNetworkChanged(wxCommandEvent& event) {
    if (d_acceptorNetworkRadioBox->GetSelection() == 0) {
      d_acceptorWalletTextCtrl->Disable();
      d_acceptorHostChoice->Clear();
      d_acceptorHostChoice->SetSelection(wxNOT_FOUND);
      d_acceptorHostChoice->Disable();
    } else {
      d_acceptorWalletTextCtrl->Enable();
      d_acceptorHostChoice->Clear();
      d_acceptorHostChoice->Append("127.0.0.1:24081");
      d_acceptorHostChoice->Append("93.190.142.146:24081");
      d_acceptorHostChoice->Append("217.23.12.74:24081");
      d_acceptorHostChoice->SetSelection(0);
      d_acceptorHostChoice->Enable();
    }
  }

  void onMineButton(wxCommandEvent& event) {
    if (d_isMining) {
      d_mergedMiner.stop();
      d_mining.wait();
      d_mergedMiner.start();
      d_isMining = false;
      processMessages();
      d_messagesTextCtrl->AppendText("Mining stopped\n");
      d_mineButton->SetLabel("Start mining");
    } else {
      std::string address1(d_donorHostChoice->GetString(d_donorHostChoice->GetSelection()));
      std::string wallet1(d_donorWalletTextCtrl->GetLineText(0));
      std::string address2(d_acceptorHostChoice->GetString(d_acceptorHostChoice->GetSelection()));
      std::string wallet2(d_acceptorWalletTextCtrl->GetLineText(0));
      size_t threads;
      std::istringstream(std::string(d_threadCountChoice->GetString(d_threadCountChoice->GetSelection()))) >> threads;
      d_mining = std::async(std::launch::async, &MergedMiner::mine, &d_mergedMiner, address1, wallet1, address2, wallet2, threads);
      d_isMining = true;
      d_messagesTextCtrl->AppendText("Mining started\n");
      d_mineButton->SetLabel("Stop mining");
    }
  }

  void onTimer(wxTimerEvent& event) {
    processMessages();
    if (d_isMining) {
      if (d_mining.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        d_isMining = false;
        d_messagesTextCtrl->AppendText("Mining stopped\n");
        d_mineButton->SetLabel("Start mining");
      }
    }
  }

  void processMessages() {
    std::string message;
    for (;;) {
      message = d_mergedMiner.getMessage();
      if (message.empty()) {
        return;
      }

      message += '\n';
      d_messagesTextCtrl->AppendText(message);
    }
  }
};

class SoloMiner : public wxApp {
public:
  bool OnInit() {
    SoloMinerFrame* frame = new SoloMinerFrame(nullptr, wxID_ANY, "SoloMiner", wxDefaultPosition, wxSize(480, 640), wxCAPTION | wxCLIP_CHILDREN | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU);
    frame->Show();
    return true;
  }
};

IMPLEMENT_APP(SoloMiner)

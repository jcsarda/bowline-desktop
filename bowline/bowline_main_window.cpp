#include "bowline.h"
#include "bowline_control.cpp"
#include "bowline_config.cpp"

using namespace Rice;

enum
{
  ID_Quit=1
};

class BowlineMainWindow : public BowlineControl
{
public:
  BowlineMainWindow(
    const wxString path = wxEmptyString, 
    const wxString name = wxEmptyString,
    const bool chrome = true, 
    const wxSize size = wxDefaultSize
  ) : BowlineControl(NULL, path, name, chrome, size) 
  {
    BowlineMainWindow::SetInstance(this);
    wxTheApp->SetTopWindow(this);
    
    fileMenu = new wxMenu();
    fileMenu->Append(ID_Quit, _("E&xit"));
    menuBar->Append(fileMenu, _("&File"));
    Connect( ID_Quit, wxEVT_COMMAND_MENU_SELECTED,
                  (wxObjectEventFunction) &BowlineMainWindow::OnQuit );    
  };
  
  void SetupConfiguration(){
    wxSize coords = wxSize(
      BowlineConfig::getInt(_("width")), 
      BowlineConfig::getInt(_("height"))
    );
    wxString appName  = BowlineConfig::getString(_("name"));
    bool chrome       = BowlineConfig::getBool(_("chrome"));
    wxString path     = BowlineConfig::getString(_("index_path"));

    SetName(appName);
    SetSize(coords);
    SetChrome(chrome);
    LoadFile(path);
  }
  
  void OnQuit(wxCommandEvent& WXUNUSED(event))
  {
    Close(true);
  }
  
  static BowlineMainWindow *GetInstance(){ return mainWindowInstance; }
  static void SetInstance(BowlineMainWindow *win) { mainWindowInstance = win; }
  
protected:
  static BowlineMainWindow *mainWindowInstance;
  wxMenu *fileMenu;
};

BowlineMainWindow *BowlineMainWindow::mainWindowInstance = NULL;

void Init_Bowline_Main_Window(){
  Module rb_mBowline        = define_module("Bowline");
  Module rb_mBowlineDesktop = define_module_under(rb_mBowline, "Desktop");
  
  Class rb_cBowlineMainWindow =
    define_class_under<BowlineMainWindow, BowlineControl>(rb_mBowlineDesktop, "MainWindow")
    .define_singleton_method("get", &BowlineMainWindow::GetInstance);
}
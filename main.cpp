#include <wx/wx.h>
#include <wx/file.h>
#include <wx/stdpaths.h>
#include <wx/timer.h>
#include <iostream>
#include <rice/detail/ruby.hpp>
#include <ruby/encoding.h>

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "wx_pathname.cpp"
#ifdef __WXMAC__
#include "wx_stdpaths_cf.cpp"
#endif
#include "ruby_utils.cpp"
#include "bowline/bowline.cpp"

extern "C" {
  static void
  ruby_init_encoding(){
    rb_encoding *lenc;
    lenc = rb_locale_encoding();
    (void)rb_filesystem_encoding();
    rb_enc_set_default_external(rb_enc_from_encoding(lenc));
  }
  
  void Init_prelude(void);
  
  static void
  ruby_init_gems()
  {
      rb_define_module("Gem");
      Init_prelude();
      rb_const_remove(rb_cObject, rb_intern_const("TMP_RUBY_PREFIX"));
  }
}

class App : public wxApp
{
public:
  bool OnInit();
  int OnExit();
  void InitRuby();
  void AddLoadPath(const wxString& path);
  void Tick(wxTimerEvent& evt);
  void Idle(wxIdleEvent& evt);
  wxString ResourcePath();
  wxString LibPath();
  BowlineMainWindow* window;
  wxTimer tickTimer;
};

IMPLEMENT_APP(App)

bool App::OnInit()
{
  window = new BowlineMainWindow();

  InitRuby();
  InitBowline();

  int error;
  rb_load_protect(rb_str_new2("script/init"), Qfalse, &error);
  if(error){
    RubyUtils::LogError();
    throw "Ruby Error";
  }
  
  Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(App::Idle), NULL, this);
  tickTimer.Connect(tickTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(App::Tick), NULL, this);
  tickTimer.Start(50);
  
  #ifdef __WXMAC__
  // Enable interaction for unbundled apps
  ProcessSerialNumber PSN;
  GetCurrentProcess(&PSN);
  TransformProcessType(&PSN,kProcessTransformToForegroundApplication);
  SetFrontProcess(&PSN);
  
  // Don't exit when top frame is closed
  // wxApp::SetExitOnFrameDelete(false);
  #endif
  
  return true;
}

int App::OnExit()
{
  tickTimer.Stop();
  rb_exit(0);
  return 0;
}

void App::InitRuby(){
  int argc = App::argc;
  char** argv = App::argv;

  ruby_sysinit(&argc, &argv);
  RUBY_INIT_STACK;
  ruby_init();
  ruby_init_loadpath();
  ruby_init_encoding();
  ruby_script("bowline");
        
  wxString lib_path      = this->LibPath();
  wxString rubylib_path  = wxPathname::Join(lib_path, "rubylib");
  
  wxString version  = "1.9.1";
  
#ifdef __WXOSX_COCOA__
  wxString platform = "i386-darwin10.2.0";
#endif

#ifdef __WXGTK__
  wxString platform = "i686-linux";
#endif
    
  AddLoadPath(wxPathname::Join(rubylib_path, version));                          // RUBY_LIB
  AddLoadPath(wxPathname::Join(rubylib_path, version, platform));                // RUBY_ARCHLIB
  AddLoadPath(wxPathname::Join(rubylib_path, "site_ruby"));                      // RUBY_SITE_LIB
  AddLoadPath(wxPathname::Join(rubylib_path, "site_ruby", version));             // RUBY_SITE_LIB2
  AddLoadPath(wxPathname::Join(rubylib_path, "site_ruby", version, platform));   // RUBY_SITE_ARCHLIB
  AddLoadPath(wxPathname::Join(rubylib_path, "vendor_ruby"));                    // RUBY_VENDOR_LIB
  AddLoadPath(wxPathname::Join(rubylib_path, "vendor_ruby", version));           // RUBY_VENDOR_LIB2
  AddLoadPath(wxPathname::Join(rubylib_path, "vendor_ruby", version, platform)); // RUBY_VENDOR_ARCHLIB
  
  wxString resource_path = this->ResourcePath();
  AddLoadPath(resource_path);
  
  ruby_init_gems();
}

void App::AddLoadPath(const wxString& path){
  ruby_incpush(path.c_str());
}

void App::Tick(wxTimerEvent& WXUNUSED(evt)){
  rb_eval_string_protect("Bowline::Desktop.tick", NULL);
}

void App::Idle(wxIdleEvent& WXUNUSED(evt)) {
  // This absolutely sucks. Needed to get threads running.
  // There is a Ruby API to do this properly (see RubyGVL),
  // but it keeps segfaulting on me
  rb_eval_string_protect("sleep(0.05)", NULL);
  rb_eval_string_protect("Bowline::Desktop.idle", NULL);
}

wxString App::ResourcePath(){
  if(App::argc > 1){
    wxString argv1 = App::argv[1];
    if(argv1 == ".") return wxGetCwd();
    if(wxIsAbsolutePath(argv1)) return argv1;
    return wxPathname::Join(wxGetCwd(), argv1);
  } else {
    wxString path = wxStandardPaths::Get().GetResourcesDir();
    return path;
  }
}

wxString App::LibPath(){
#ifdef __WXMAC__
  wxString path = wxStandardPathsCFExt().GetRawExecutablePath();
#else
  wxString path = wxStandardPaths::Get().GetExecutablePath();
#endif
  path = wxPathname::Dirname(path);
  return wxPathname::Join(path, "libs");
}
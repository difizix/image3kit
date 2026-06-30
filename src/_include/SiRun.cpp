//! \file SiRun.cpp definitions of general utility commands
// Developed by y:
//  - Ali Q. Raeini (2017-2020)
//......................................................................

#ifdef MULTITHREAD
 #include <thread>
#endif

#define _InitGlobals
#include "typses.h" // replaceFromTo
#include "globals.h"

#include "InputFile.h"


#include "SiRun.h"

using namespace std;

//!# SiM namespace: is used to define C++ functions  which will be invoked from input files

namespace SiM {

#ifdef MULTITHREAD
  class threadDetacher {// not tested
    stringstream ins_;
    thread  trd_;
    public:
    static int iTrd;
    threadDetacher(SiR::ProcessPP f, string s, storeT& stor)
    :  ins_(s), trd_(f, ref(ins_), ref(stor)) {}
    ~threadDetacher(){ (cout<<" ~ thread ").flush(); trd_.join(); }
  };
  int threadDetacher::iTrd=0;
  bool runDetach( stringstream& ins, storeT& stor)  {
    KeyHint("{list of commands}\n// runs sindo commands in background, TODO: test");

    (cout<<"running commands:\n{\n").flush();

    InputFile inps(false); inps.readStream(ins);

    const void* sir_ = dbget(stor, "_SiR_");
    const auto& key_funs = static_cast<const SiR*>(sir_)->key_funs;

    for (const auto& kywrd:inps.data())  {
      const auto kyfunc = key_funs.find(kywrd.first);
      if (kyfunc!=key_funs.end())  {
        (cout<<" "<<kywrd.first<<"&: ").flush();
        dbset(stor,kywrd.first+"_thread"+_s(++threadDetacher::iTrd), new threadDetacher(kyfunc->second, kywrd.second, stor));
        cout<<"&"<<endl; //! error thread is killed at main exit! TODO: join in the end
      }
      else cout<<"  skipped executing entry \""<<kywrd.first<<"\":/ "<<endl;
    }
    (cout<<"finished commands}").flush();
    return true;
  }
#endif

  bool run( stringstream& ins, storeT& stor)  {
    KeyHint("{list of commands} // superfluous");
    auto cwdO = getpwd();

    (cout<<" commands:/** ").flush();
    InputFile inps(false); inps.readStream(ins);
    inps.echoKeywords();
    (cout<<" */\n").flush();
    const void* sir_ = dbget(stor, "_SiR_");
    static_cast<const SiR*>(sir_)->process(inps, "running");

    if(cwdO != getpwd()) { ensure(chdir(cwdO)==0); }
    return true;
  }

  bool help( stringstream& ins, storeT& stor)  {
    KeyHint("key subkey  // lists available/the keywords and their usage");

    string keyname, subkey;
    ins>>keyname>>subkey;
    cout<<"? "<<keyname<<" "<<subkey<<": ";
    const void* sir_ = dbget(stor, "_SiR_");
    string hlp = static_cast<const SiR*>(sir_)->help(keyname,subkey);
    if (hlp.empty()) // FIXME: temporary
    {
      hlp = static_cast<const SiR*>(sir_)->help("VxlPro",keyname);
      if (hlp.size()) cout<<" available as a sub-command:\nVxlPro ";
    }
    cout<<hlp<<endl;
    return true;
  }

  bool runPar( stringstream& ins, storeT& stor) {
    KeyHint("{list: of commands}; // runs commands in parallel");

    (cout<<"running in par:\n{\n").flush();
    InputFile inps(false); inps.readStream(ins);
    const auto& key_funs = static_cast<const SiR*>(dbget(stor, "_SiR_"))->key_funs;
    #ifdef OpenMP
     _Pragma("omp parallel for")
    #endif
    for(size_t itrd=0; itrd<inps.data().size(); ++itrd) {
      const auto& kywrd=inps.data()[itrd];
      const auto kyfunc = key_funs.find(kywrd.first);
      if (kyfunc!=key_funs.end())  {
        (cout<<" "<<kywrd.first<<": ").flush();
        stringstream ss;
        ss.str(kywrd.second);
        (*(kyfunc->second))(ss, stor);
        cout<<endl;
      }
      else
        cout<<"  skipped executing \""<<kywrd.first<<"\" in runPar:/ "<<endl;
    }
    (cout<<"finished in par}").flush();
    return true;
  }

  bool runFor( stringstream& ins, storeT& stor)  {// "in" can be anywhere, obsolete
    KeyHint("VAR { in val1 val2 ...; commands... VAR...; } \n// runs commands replacing each occurrence of VAR with values provided after ``in'' subkeyword;");
     //! runFor XX in x1 x2 x3 ; [  echo ...XX... ]
    string var; ins>>var;
    string instr = ins.str().substr(ins.tellg());
    //cout<<"instr: { "<<ins.str()<<" }"<<endl;
    {  InputFile inps(false); inps.readStream(ins); ins.clear();
      ins.str(inps.kwrd("in"));   cout<<var<<" in "<<ins.str()<<endl;
    }
    const auto& sir = *static_cast<const SiR*>(dbget(stor, "_SiR_"));
    string itr; ins>>itr;

    auto cwdO = getpwd();

    while(itr.size())  {
      cout<<var<<"_"<<itr<<"{ "<<endl;
      stringstream insi(replaceFromTo(instr,var,itr));
      InputFile inps(false); inps.readStream(insi);
      inps.set("in",""); inps.renameKeys("in","");
      inps.echoKeywords();
      sir.process(inps, "iteration_"+itr);
      cout<<"}"<<endl;
      itr.clear(); ins>>itr;

      if(cwdO != getpwd()) { ensure(chdir(cwdO)==0); }
    }
    return true;
  }


  bool runForX( stringstream& ins, storeT& stor)  {//! for
    KeyHint("XX YY { in  x1 y1  x2 y2; commands...XX...YY;...; }\n// runs commands after replacing each occurrence of XX and YY with the tuples (x1 y1 ...), provided after ``in'' subkey;"
    "\n// 'in' should be the first subKey");
     //! runFor XX in x1 x2 x3 ; [  echo ...XX... ]

    vector<string> kynams;
    while(ins) { // first keyword should be 'in'
      auto bg = ins.tellg(); string vr; ins>>vr;
      if (vr!="in")  kynams.push_back(vr);  else  { ins.seekg(bg); break; } }

    string instr = ins.str().substr(ins.tellg());
    {  InputFile inps(false); inps.readStream(ins); ins.clear();
      ins.str(inps.kwrd("in"));   cout<<kynams<<" in "<<ins.str()<<endl;
    }
    const void* sir_ = dbget(stor, "_SiR_");
    const auto& sir = *static_cast<const SiR*>(sir_);

    auto cwdO = getpwd();

    vars<string> itrs(kynams.size()); ins>>itrs;
    while(itrs.back().size()) {
      cout<<"\n{";//<<itrs<<endl;
      string incp = instr;  for_(itrs,i){ incp=replaceFromTo(incp,kynams[i],itrs[i]); }
      stringstream insi(incp);
      InputFile inps(false); inps.readStream(insi);
      inps.set("in",_s(kynams)+" @  "+_s(itrs)); inps.renameKeys("in",""/*empty key ignored*/);
      cout<<"/** "; inps.echoKeywords(); cout<<"*/";
      sir.process(inps, "iteration:_"+_s(itrs));
      cout<<"}" <<endl;
      for_(itrs,i)  itrs[i].clear();
      ins>>itrs;

      if(cwdO != getpwd()) { ensure(chdir(cwdO)==0); }
    }
    ensure(itrs[0].empty(),"unused data in 'in' args: "+itrs[0]);
    cout<<"\n// end for"<<endl;
    return true;
  }

  bool forPar( stringstream& ins, storeT& stor)  {
    KeyHint("XX YY { in  x1 y1  x2 y2; commands...XX...YY;...; } \n// runs commands after replacing each occurrence of XX and YY with the tuples (x1 y1 ...) provided after ``in'' subkey;"
    "\n// 'in' should be the first subKey");
     //! forPar XX in x1 x2 x3 ; [  echo ...XX... ]

    vector<string> kynams;
    while(ins) // first keyword should be 'in'
    {  auto bg = ins.tellg(); string vr; ins>>vr;
      if(vr!="in")  kynams.push_back(vr); else { ins.seekg(bg); break; } }

    string instr = ins.str().substr(ins.tellg());
    {InputFile inps(false); inps.readStream(ins); ins.clear();
      ins.str(inps.kwrd("in"));   cout<<kynams<<" in "<<ins.str()<<"{"<<endl;
    }
    const void* sir_ = dbget(stor, "_SiR_");
    const auto& sir = *static_cast<const SiR*>(sir_);

    auto cwdO = getpwd();

    stvec<vars<string>> XXs;
    {  vars<string> itrs(kynams.size());
      while(ins>>itrs) XXs.push_back(itrs); }

    cout<<kynams<<" --> "<<XXs<<endl<<endl<<endl;
    #ifdef OpenMP
     _Pragma("omp parallel for")
    #endif
    for(size_t itrd=0; itrd<XXs.size(); ++itrd)  {
      const auto& itrs = XXs[itrd];
      (cout<<"\n{ ").flush();
      string incp = instr;  for_(itrs,i){ incp=replaceFromTo(incp,kynams[i],itrs[i]); }
      stringstream insi(incp);
      InputFile inps(false); inps.readStream(insi);
      inps.set("in",_s(kynams)+"~>"+_s(itrs)); inps.renameKeys("in",""/*empty key ignored*/);
      insi.clear(); inps.echoKeywords(insi); cout<<"/** "+insi.str()+"*/ ";
      sir.process(inps, "iteration:_"+_s(itrs));
      cout<<"}"<<endl;

      if(cwdO != getpwd()) { ensure(chdir(cwdO)==0); }
    }
    cout<<"\n};//par for"<<endl;
    return true;
  }


  bool _sh_( stringstream& ins, storeT& stor)  {
    KeyHint("{ list of system (bash) commands }");
    auto cmds = replaceFromTo(replaceFromTo(ins.str(),"\\\\","\\\\"),"\\$","\\$");
    (cout<<"running ```.bash\n"<<cmds<<"\n```  : {\n").flush();
    int ec = system(("bash <<_EndfCmd_\n"+cmds+"\n_EndfCmd_\n").c_str());
    ensure(!ec, " bash exit code: "+_s(ec),ec);
    (cout<<" } ").flush();
    return ec;
  }

  bool _cd_( stringstream& ins, storeT& stor)  {
    KeyHint("./path/to/directory  // Note: moving out of current directory is not allowed");
     // TODO: use C++17 file-system tighten security and rename _cd_ to cd
    string dir;    ins>>dir;
    ensure(dir.size()>0 && dir[0]!='/' && (dir.size()<2 || dir[1]!='.'), "_cd_: "+dir+", not allowed", -1);
    cout<<"Changing working directory: "<<dir<<", " << _TRY_(chdir(dir.c_str())) << endl;
    return 0;
  }

   bool mkDir( stringstream& ins, storeT& stor)  {
    KeyHint("directory_name to create");
    // TODO use C++17 file-system
    string dir;    ins>>dir;    if(dir.back()!='/') dir=dir+"/";
    ensure(dir.size()>1 && dir[0]!='/' && dir[1]!='.', "mkDir "+dir+", not allowed", -1);
    if(size_t sl=dir.find_first_of("\\/")) { if(sl<dir.size()) {
      dir=dir.substr(0,sl);
      cout<<"Creating folder: "<<dir<<"  "
        << _TRY_(mkdirs(dir))
        <<endl;
    } }
    return 0;
  }

  bool echo( stringstream& ins, storeT& stor)  {
    KeyHint("message");
    (cout<<ins.str()).flush();
    return 0;
  }

  bool ignore( stringstream& ins, storeT& stor) {
    KeyHint("{ section to ignore }");
    return 0;
  }

  #ifdef _STOR_PUB
  bool expose( stringstream& ins, storeT& stor)  {
    KeyHint("[var(s)] //<- name of variable to expose as global (obsolete),\n"
      "      Lists variables if no arguments provided"
      "      Note: VxlRead auto exposes its stuff");
    string varNam;
    int ii=0;
    while((ins>>varNam) && varNam.size() && ++ii)  {
      (cout<<" "<<varNam<<" ").flush();
      leakset(_STOR,varNam,dbget(stor,varNam));
      varNam.clear();
    }
    if(ii)
      (cout<<"\n\t"<<ii<<" made global").flush();
    else {
      (cout<<"Variables x"<<stor.size()<<": ").flush();
        for(const auto& var:stor) if("_SiR_"!=var.first) cout<< " "<<var.first;
      (cout<<"\nGlobals x"<<_STOR.size()<<": ").flush();
        for(const auto& var:_STOR) if("_SiR_"!=var.first) cout<< " "<<var.first;
      }

    return 0;
  }
  #endif

  bool erase( stringstream& ins, storeT& stor)  {
    KeyHint("var(s)  //<- name of variable to erase from memory");
    string varNam;
    while((ins>>varNam) && varNam.size())  {
      (cout<<" ~"<<varNam<<" ").flush();
      stor.erase(varNam);
      #ifdef _STOR_PUB // todo
      if (auto vi = _STOR.find(varNam); vi!=_STOR.end()) _STOR.erase(vi);
      #endif
      varNam.clear();
    }
    return 0;
  }

  bool clear( stringstream& ins, storeT& stor)  {
    KeyHint("// erase everything, note: use `erase var...` instead");
    cout<<" ~store[*] "<<endl;
    void* sir_ = dbget(stor, "_SiR_");
    stor.clear();
    leakset(stor, "_SiR_", static_cast<SiR*>(sir_));
    #ifdef _STOR_PUB
    _STOR.clear();
    #endif
    return 0;
  }

} // namespace SiM

SiR::SiR() {
  rootDir_=getpwd();
  if(key_funs.size()==0)  {
    key_funs.insert({""         ,& SiM::ignore});
    key_funs.insert({"skip"     ,& SiM::ignore}); //obsolete
    key_funs.insert({"SKIP"     ,& SiM::ignore});
    key_funs.insert({"Note"     ,& SiM::ignore});
    key_funs.insert({"help"     ,& SiM::help});
    key_funs.insert({"echo"     ,& SiM::echo});
    #ifdef _STOR_PUB
    key_funs.insert({"expose"   ,& SiM::expose}); // use clear to unshare, for now
    #endif
    key_funs.insert({"erase"    ,& SiM::erase});
    key_funs.insert({"clear"    ,& SiM::clear});
    key_funs.insert({"run"      ,& SiM::run});
    key_funs.insert({"for"      ,& SiM::runForX});
    key_funs.insert({"runFor"   ,& SiM::runFor});
    key_funs.insert({"runPar"   ,& SiM::runPar});
    key_funs.insert({"forPar"   ,& SiM::forPar});
    #ifdef MULTITHREAD
    key_funs.insert({"runDetach",& SiM::runDetach});
    #endif
    key_funs.insert({"mkdir"    ,& SiM::mkDir});
    key_funs.insert({"cd"       ,& SiM::_cd_});
    key_funs.insert({"_sh_"     ,& SiM::_sh_});
    key_funs.insert({"bash"     ,& SiM::_sh_});
    key_funs.insert({"mkDir"    ,& SiM::mkDir});
    leakset(stor_, "_SiR_", this);
  }
};

void SiR::process(istream& ins, string dictNam) const {
  while (true)  {
    streampos begLine = ins.tellg();
    string cmd;
    ins>>cmd;
    if (ins.fail())      {cout<<"  @"<<ins.tellg()<<"  "<<dictNam<<" done."<<endl;  break; }
    else if (cmd[0]=='{' || cmd[0]=='}') {ins.seekg(int(ins.tellg())-cmd.size()+1); continue;}
    else if (cmd[0]=='#' || cmd[0]=='\'' || cmd[0]=='/' || cmd[0]=='%')  ins.ignore(10000,'\n');
    else {
      auto paer = key_funs.find(cmd);
      if (paer!=key_funs.end())  {
        (cout<<" "<<cmd<<": ").flush();
        stringstream ss;
        if(ins.peek()!='\n') ins.get (*(ss.rdbuf()));
        (*(paer->second))(ss, stor_);
        cout<<endl;
      }
      else {
        cout<<"  ended processing "+dictNam+" before: \""+cmd+"\", no such command :/"<<endl;
        ins.clear();
        ins.seekg(begLine);
        break;
      }
    }
  }
}

void SiR::process(const InputFile& inp, string act) const {
  for(const auto& key:inp.data())  {
    auto paer = key_funs.find(key.first);
    if (paer!=key_funs.end())  {
      (cout<<""<<key.first<<": ").flush();
      stringstream ss(key.second);
      (*(paer->second))(ss, stor_);
      cout<<endl;
    }
    else {
      if(key.first!="end") cout<<"  stopped "+act+" "+inp.fileName()+" before: \""+key.first+"\", no such command :/"<<endl;
      break;
    }
  }
}


string SiR::help(string key, string subkey) const  {
  stringstream keys;
  if(key.size() && key[0]!='-')  {
      auto paer = key_funs.find(key);
      if (paer!=key_funs.end())  {
        stringstream ss(subkey.empty()?  "?" : "? "+subkey);
        (*(paer->second))(ss, stor_);
        return ss.str();
      }
      else
        cout<<" no such keyword `"<<key<<"`. Run `help` with no argument for available commands"<<endl;
      return "";// empty -> signal not found
  }
  else {
    keys<<"Available commands:\n";
    vector<pair<string,ProcessPP>> keyfuns(key_funs.begin(), key_funs.end());
    sort(keyfuns.begin(), keyfuns.end());
    for(const auto& proc:keyfuns) if(proc.first.size()>1)  {
      keys<<"  "<< left<<setw(15)<<proc.first+":"<<" ";
      stringstream ss("?");
      try                         {  (*proc.second)(ss, stor_);  }
      catch (exception &exc) {  cerr << proc.first<<" KeyHelp not implemented:" << exc.what() << endl; }
      catch (...)                 {  cerr << proc.first<<" KeyHelp not implemented:" << endl; }
      keys<<replaceFromTo(ss.str(),"\n","\n                  ")<<"\n\n";
    }
    keys<<"\nRun ``help: keyword'' for information about individual keywords.\n";
  }
  return keys.str();
}

int usage_SiR(const char* app_name)  {
  cout<<"\n"
        "Usage: \n"
        "   "<<app_name<<" input_file.cc                #<-- typical usage\n"
        "Alternative usages: \n"
        "   "<<app_name<<" -c \"cmnd args; cmnd2 args2;\" #<-- read commands from stdin\n"
        "   "<<app_name<<" -d dir/  input.cc            #<-- run input.cc in directory dir\n"
        "   "<<app_name<<" -d dir/ -c \"cmnd args;...\"   #<-- run commands in a dir\n"
        "   "<<app_name<<" -h                           #<-- print this help message\n"
        "   "<<app_name<<" ? key                        #<-- print the keyword help\n"
        "   "<<app_name<<" ? key subkey                 #<-- print sub-command help\n"
        "\n"<<endl;
  return 0;
}

#ifdef USE_POPY
int pocketpy_main(std::string str, storeT& stor);
#endif

//!# Template main function, to be called after populating SiR (C runner) with specialized SiM (C modules)
//!# MainT usage:
//! \code
//! #include "SiR.cpp"
//! int main(int argc, char *argv[])  {
//!     plugins SiM_derived_class;
//!     return MainT<SiR>(argc, argv, &plugins);
//! }
//! \endcode
int MainT(int argc, char **argv, const SiR* siruner, int (*usage_sir)(const char*))  {
  (cout<<"/""/-*-C-*- "<<__DATE__<<"; "<<argv[0]).flush();
  if(argc<2 || argc>5) return usage_sir(argv[0])-1;

  rlset(siruner->stor_, "_argcvs_", new pair<int, char**>(argc,argv));

  string header(argv[1]);
  string keystr= argc>2 ? argv[2] : "";
  string subkey= argc>3 ? argv[3] : "";

  if(header=="?" || header=="-h")  {
    (cout<<" "<<header<<" "<<keystr<<" "<<subkey<<"; ").flush();
    if(keystr.empty())  usage_sir(argv[0]);
    else                cout<<keystr<<" usage:\n ";
    cout<<keystr<<": "<<siruner->help(keystr, subkey)<<endl;
    return 0;
  }
  if(header=="-d")  {
    (cout<<" -d "<<header<<" ").flush();
    if(subkey.empty()) return usage_sir(argv[0])-1;
    enforce(chdir(argv[2])==0, string("Failed on cd ")+argv[2]);
    header=subkey;  keystr= argc>4 ? argv[4] : "";
  }

  #ifdef USE_POPY
  if (header=="-c") {
    (cout<<" -c  '"<<keystr<<"' # pocketpy").flush();
    if (keystr.empty())  return usage_sir(argv[0])-1;
    pocketpy_main(keystr, siruner->stor_);
  }
  else if (hasExt(header,".py"))
  {
    (cout<<" '"<<header<<"' # pocketpy").flush();
    std::ifstream in(header);
    //istringstream keyins; keyins << in.rdbuf();
    auto size = std::filesystem::file_size(header);
    keystr.resize(size,'\0');
    in.read(&keystr[0], size);
    pocketpy_main(keystr, siruner->stor_);
  }
  else
  #endif
  { //.cc
    InputFile inp(false);
    if (header=="-i") {
      (cout<<" -i  '"<<keystr<<"' ").flush();
      if (keystr.empty())  return usage_sir(argv[0])-1;
      istringstream keyins(keystr);
      inp.readStream(keyins);
    }
    else if(header.size()<4)  return usage_sir(argv[0])-1;
    else {
      cout<<" "<<header<<" ";
      inp.readFile(header);
    }

    cout<<" //siruner:\n"<<endl;
    siruner->process(inp);
    cout<< "\n// ~siruner" << endl;
  }

  return 0;
}

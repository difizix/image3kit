/*-------------------------------------------------------------------------*\
 Developed by:
 - A. Q. Raeini (2018-2020)
\*-------------------------------------------------------------------------*/

using namespace std;



#include "surfUtils.h"


dbl3s mapFacesGetPoints(std::vector<std::vector<face> >& facess, const piece<point> pointsAll)  {
  ints pValidIs(pointsAll.size(),-1); int pInd=-1;
  for(auto&faces:facess) for(auto&fac:faces) for(auto& pi:fac) if(pValidIs[pi]==-1) pValidIs[pi]=++pInd;
  dbl3s points(pInd+1);
  for(auto&faces:facess) for(auto&fac:faces) for(auto& pi:fac) pi=pValidIs[pi];
  for_(pValidIs,i) if(pValidIs[i]!=-1) { points[pValidIs[i]]=pointsAll[i]; }
  return points;
}

std::vector<std::vector<face> > getbsoleteOrderedFaces(const facePieceList& facezsz, int vv)  {
  std::vector<std::vector<face> > facess(256);
  {
    ints nFacs(256,0);
    const facePiece& facezs=facezsz[vv];
    for(const auto&fac:facezs) { ensure(0<=fac.zone && fac.zone<255);  ++nFacs[fac.zone];}
    for(int i=0; i<256; ++i) if(nFacs[i]) { facess[i].resize(nFacs[i], face({-1,-1,-1,-1})); nFacs[i]=-1; }
    for(const auto&fac:facezs) { facess[fac.zone][++nFacs[fac.zone]]=fac; ensure(facess[fac.zone][nFacs[fac.zone]][0]>=0);}
  }


  auto invertFace = [](const face& f) { return face({f[3],f[2],f[1],f[0]}); };
  for(int v2=0; v2<vv; ++v2) if(facezsz[v2].size()) // when searching for faces only faces toward higher labels are extracted, here we also add those toward lower voxel-labels
  { /// beak each voxel value surfacce manifold into zones
    ints nFacs(256,0);
    const facePiece& facezs=facezsz[v2];
    for(const auto&fac:facezs) { if(fac.zone==vv)  ++nFacs[v2];}
    for(int iz=0; iz<vv; ++iz) if(nFacs[iz]) { facess[iz].resize(nFacs[iz], face({-1,-1,-1,-1})); nFacs[iz]=-1; }
    for(const auto&fac:facezs)  if(fac.zone==vv) { facess[v2][++nFacs[v2]]=invertFace(fac);  ensure(facess[v2][nFacs[v2]][0]>=0);} // wrong zone
  }
  return facess;
}

void writeSurfaceFiles(const facePieceList& facezsz, const piece<point> pointsAll, const std::string& fileNames)  {

 for(size_t vv=0; vv<facezsz.size(); ++vv) if(facezsz[vv].size())  {
  string ext = fileNames.substr(fileNames.size()-4);
  string basNam = fileNames.substr(0,fileNames.size()-4)+_s(vv);

  cout<<"writing surface "<<basNam+ext<<" for voxels values "<<vv<<endl;
  /// beak each voxel value surface manifold into zones

  std::vector<std::vector<face> > facess = getbsoleteOrderedFaces(facezsz, vv);

  dbl3s points = mapFacesGetPoints(facess,pointsAll);
  /// get rid of unused points

  int nFaces=0, nZones=0;
  for(const auto& rg:facess) { nFaces+=rg.size(); nZones+=rg.size()>0; }

  if(ext==".obj")  {
    ofstream fil(basNam+ext);
    ensure(fil.good());

    fil << "o " << basNam<<"\n"
        << "# n_points : " << points.size() << "\n"
        << "# n_faces  : " << nFaces << "\n"
        << "# n_zones  : " << nZones << "\n";

    // Print zone names as comment
    for_(facess, zoneI)
      if(facess[zoneI].size())
        fil  << "#  " << zoneI << "  zone" << zoneI
             << "  (nFaces: " << facess[zoneI].size() << ")" << "\n";

    // Write vertex coords
    for_(points, ptI)  {
      const point& pt = points[ptI];
      fil  << "v " << pt.x << ' '  << pt.y << ' '  << pt.z << "\n";
    }

    fil  << endl;

    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];
      fil << "vt  " << zoneI/255. <<' '<< zoneI/255. << endl;

      if (zone.size())  {
        fil << "g  zone" << zoneI << endl;
        fil << "#  zone"<<zoneI<<" size: " << zone.size() << endl;
        {
          for_(zone, zfI)  {
            const face& f = zone[zfI];

            fil << 'f';
            for_(f, fp)  fil << ' ' << f[fp] + 1<<'/'<<zoneI;//+1
            fil << "\n";
          }
        }
      }
    }
    fil << endl;
  }
  else {
    if(ext!=".vtk")     cerr << "outputSurface format "<<fileNames<<"not supported switching to vtk";

    ofstream fil(basNam+".vtk");
    ensure(fil.good());

    fil << "# vtk DataFile Version 2.0\n"
        << "Generated using image3kit...\n"
        << "ASCII\n\n"
        << "DATASET POLYDATA"<<endl;

    // Write vertex coords
    fil  << "POINTS " << points.size() << " float" << '\n';
    for_(points, ptI)  {
      const point& pt = points[ptI];
      fil  << pt.x << ' ' << pt.y << ' ' << pt.z << '\n';
    }

    int nFaces=0, nZones=0;
    for(const auto& rg:facess) { nFaces+=rg.size(); nZones+=rg.size()>0; }

    label nNodes = nFaces*4;
    fil << '\n'
        << "POLYGONS " << nFaces << ' ' << nFaces + nNodes << '\n';


    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];

      for_(zone, zfI)  {
        const face& f = zone[zfI];
        fil << f.size();
        for_(f, fp)  fil << ' ' << f[fp];
        fil << '\n';
      }
    }


    fil << '\n'
        << "CELL_DATA " << nFaces << '\n'
        << "FIELD attributes 1\n"
        << "zone 1 " << nFaces << " int\n" ;

    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];

      for_(zone, zfI)  {
        fil << zoneI;//+1
        fil << '\n';
      }
    }
  }

 } // for vv
}



void writeMergeSurfaceFile(const facePieceList& facezsz, const piece<point> points, const std::string& fileNames) //,std::vector<string> rockTypes
{
  string ext = fileNames.substr(fileNames.size()-4);
  string basNam = fileNames.substr(0,fileNames.size()-4);
  (cout<<"writing surface "<<basNam+ext<<" ").flush();

  std::vector<std::vector<face> > facess(256*256);

  for(size_t vv=0; vv<facezsz.size(); ++vv) if(facezsz[vv].size())  {
    (cout<<" "<<vv).flush();
    /// beak  each voxel value surfacce manifold into zones
    const facePiece& facezs=facezsz[vv];
    ints nFacs(256,0);
    for(const auto&fac:facezs) { ensure(0<=fac.zone && fac.zone<255);  ++nFacs[fac.zone];}
    for(int iz=0; iz<256; ++iz) if(nFacs[iz]) { facess[iz+vv*256].resize(nFacs[iz], face({-1,-1,-1,-1})); nFacs[iz]=-1; }
    for(const auto&fac:facezs) { facess[fac.zone+vv*256][++nFacs[fac.zone]]=fac;  ensure(facess[fac.zone+vv*256][nFacs[fac.zone]][0]>=0);}
    (cout<<".").flush();
  }
  (cout<<" .").flush();

  int nFaces=0, nZones=0;
  for(const auto& rg:facess) { nFaces+=rg.size(); nZones+=rg.size()>0; }

  if(ext==".obj")  {
    ofstream fil(basNam+ext);
    ensure(fil.good());

    fil << "o " << basNam<<"\n"
        << "# n_points : " << points.size() << "\n"
        << "# n_faces  : " << nFaces << "\n"
        << "# n_zones  : " << nZones << "\n";

    // Print zone names as comment
    for_(facess, zoneI)
      if(facess[zoneI].size())
        fil  << "#  " << zoneI << "  v"<< zoneI/256<<"_to_v"<<zoneI%256
             << "  (nFaces: " << facess[zoneI].size() << ")" << "\n";

    // Write vertex coords
    for_(points, ptI)  {
      const point& pt = points[ptI];
      fil  << "v " << pt.x << ' '  << pt.y << ' '  << pt.z << "\n";
    }

    fil  << endl;

    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];
      fil << "vt  " << zoneI/255. <<' '<< zoneI/255. << endl;

      if (zone.size())  {
        fil << "g  v" << zoneI/256<<"_to_v"<<zoneI%256<< endl;
        fil << "#  v" << zoneI/256<<"_to_v"<<zoneI%256<<" size: " << zone.size() << endl;
        {
          for_(zone, zfI)  {
            const face& f = zone[zfI];
            fil << 'f';
            for_(f, fp)  fil << ' ' << f[fp] + 1<<'/'<<zoneI;//+1
            fil << "\n";
          }
        }
      }
    }
    fil << endl;
  }
  else {
    if(ext!=".vtk")     cerr << "outputSurface format "<<ext<<" not supported switching to .vtk";

    ofstream fil(basNam+".vtk");
    ensure(fil.good());

    fil << "# vtk DataFile Version 2.0\n"
        << "Generated using image3kit ...\n"
        << "ASCII\n\n"
        << "DATASET POLYDATA"<<endl;

    // Write vertex coords
    fil  << "POINTS " << points.size() << " float" << '\n';
    for_(points, ptI)   fil  << points[ptI] << '\n';


    int nFaces=0;
    for(const auto& rg:facess) { nFaces+=rg.size(); }

    fil << "\n""POLYGONS " << nFaces << ' ' << nFaces + nFaces*4 << '\n';


    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];

      for_(zone, zfI)  {
        const face& f = zone[zfI];
        fil << f.size();
        for_(f, fp)  fil << ' ' << f[fp];
        fil << '\n';
      }
    }


    fil  << '\n'
      << "CELL_DATA " << nFaces << '\n'
      << "FIELD attributes 1\n"
      << "zone 1 " << nFaces << " int\n" ;

    for_(facess, zoneI)  {
      const facePiece& zone = facess[zoneI];

      for_(zone, zfI)   fil << zoneI*256+zone[zfI].zone << '\n';
    }
  }


}


void writeVTK(const std::string& fnam, const surfMsh& srf) {
  ofstream fil(fnam);
  ensure(fil.good(), "Cannot open file for writing: " + fnam, -1);

  const auto& points = srf.points;
  const auto& faces = srf.faces;

  fil << "# vtk DataFile Version 2.0\n"
      << "surface written by image3kit\n"
      << "ASCII\n\n"
      << "DATASET POLYDATA\n";

  fil << "POINTS " << points.size() << " float\n";
  for_(points, ptI) {
    const point& pt = points[ptI];
    fil << pt.x << ' ' << pt.y << ' ' << pt.z << '\n';
  }

  fil << "\nPOLYGONS " << faces.size() << ' ' << faces.size() * 5 << '\n';
  for_(faces, fI) {
    const face& f = faces[fI];
    fil << 4 << ' ' << f[0] << ' ' << f[1] << ' ' << f[2] << ' ' << f[3] << " \n";
  }

  fil << '\n'
      << "CELL_DATA " << faces.size() << '\n'
      << "FIELD attributes 1\n"
      << "zone 1 " << faces.size() << " int\n";

  for_(faces, fI) {
    if (fI) fil << ((fI % 20) ? ' ' : '\n');
    fil << (faces[fI].zone ? faces[fI].zone : 1);
  }
  fil << '\n';
}

surfMsh readVTK(const std::string& fnam) {
  surfMsh msh;
  DynamicField<point>& points = msh.points;
  std::vector<face>& faczz = msh.faces;

  std::ifstream fil(fnam);
  ensure(fil.good(), fnam + " not opened", -1);

  std::string vtkdata;
  getline(fil, vtkdata);
  getline(fil, vtkdata);
  fil >> vtkdata; ensure(vtkdata == "ASCII", "expected ASCII got " + vtkdata + " in " + fnam, -1);
  fil >> vtkdata; ensure(vtkdata == "DATASET");
  fil >> vtkdata; ensure(vtkdata == "POLYDATA");

  size_t npoints(0);
  fil >> vtkdata; ensure(vtkdata == "POINTS");
  fil >> npoints; ensure(npoints);
  fil >> vtkdata; ensure(vtkdata == "float" || vtkdata == "double");
  points.resize(npoints);
  for_(points, ptI) fil >> points[ptI];

  while (fil >> vtkdata && vtkdata != "POLYGONS");
  ensure(vtkdata == "POLYGONS");
  int nFaces = 0, nNumbers = 0;
  fil >> nFaces >> nNumbers;
  ensure(nFaces > 0);

  std::string nextTok;
  fil >> nextTok;
  if (nextTok == "OFFSETS") {
    getline(fil, vtkdata);
    for_i_(0, nFaces) fil >> vtkdata;
    fil >> vtkdata; ensure(vtkdata == "CONNECTIVITY", "expected CONNECTIVITY got " + vtkdata);
    getline(fil, vtkdata);
    --nFaces;
    faczz.resize(nFaces);
    for (face& f : faczz) for (label& p : f) fil >> p;
  }
  else {
    faczz.resize(nFaces);
    for (int fI = 0; fI < nFaces; ++fI) {
      int nv = (fI == 0) ? std::stoi(nextTok) : 0;
      if (fI > 0) fil >> nv;
      ensure(nv == 4, "Expected quad face with 4 vertices", -1);
      for (int k = 0; k < 4; ++k) fil >> faczz[fI][k];
    }
  }

  while (fil >> vtkdata) {
    if (vtkdata == "CELL_DATA") {
      size_t nCells = 0;
      fil >> nCells;
      std::string fieldTok, attrName;
      int nArrays = 0;
      fil >> fieldTok;
      if (fieldTok == "FIELD") {
        fil >> attrName >> nArrays;
        for (int a = 0; a < nArrays; ++a) {
          std::string arrName, dType;
          int nComp = 0;
          size_t nTuples = 0;
          fil >> arrName >> nComp >> nTuples >> dType;
          if (arrName == "zone" && nTuples == faczz.size()) {
            for_(faczz, fI) {
              double z = 1;
              fil >> z;
              faczz[fI].zone = int(z);
            }
          }
          else {
            double dummy;
            for (size_t k = 0; k < nTuples * nComp; ++k) fil >> dummy;
          }
        }
      }
      break;
    }
  }

  // Ensure faces are grouped by zone (1, 2, 3) if not already sorted
  bool sorted = true;
  for (size_t i = 1; i < faczz.size(); ++i) {
    if (faczz[i].zone < faczz[i - 1].zone) { sorted = false; break; }
  }
  if (!sorted) {
    std::stable_sort(faczz.begin(), faczz.end(), [](const face& a, const face& b) {
      return a.zone < b.zone;
    });
  }

  return msh;
}

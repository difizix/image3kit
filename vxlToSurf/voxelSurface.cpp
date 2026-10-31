/*-------------------------------------------------------------------------*\
 Developed by:
 - A. Q. Raeini (2018-2020)
\*-------------------------------------------------------------------------*/

/// Description:  creates a surface between the pore and the solid from a 3D rock image

#include "voxelImageI.h"
#include "surfUtils.h"

#include <set>
#include <iostream>


using namespace std;




inline int collectManifoldFaces(label pI, label connectingFace ,DynamicList<label> & group1, bool handlNonManiflEdges,
const facePiece& faces, const piece<point>& points, const labelListList& pPoints, const labelListList& pFaces )  {


  const bool selectMin=false;

  int nClctd=0;
  nClctd+=appendUnique(group1,connectingFace);

  const labelListList eFaces = edgeFaces(pPoints[pI],faces,pFaces[pI]);
  for_(pPoints[pI], eI)  {

    const ints & myEFs=eFaces[eI];
    if (myEFs.size()==1)
      Info<<"\n Error point "<<points[pI]<<" connected to edge of only one face   " <<points[pI]<<"  " <<points[pPoints[pI][eI]]<<"    face: " <<eFaces[eI] << "\n\n";

    else if (myEFs.size()==2)  {
      if      (myEFs[0]==connectingFace) nClctd+=appendUnique(group1,myEFs[1]);
      else if (myEFs[1]==connectingFace) nClctd+=appendUnique(group1,myEFs[0]);
    }
    else if (myEFs.size()>2 && handlNonManiflEdges)  {
      bool isMyEdge =false;
      for_(myEFs, fI)
        if( myEFs[fI]==connectingFace )
          isMyEdge=true;

      if(isMyEdge)  {


        dbls closeNess(myEFs.size(), selectMin ? 1000.: -1000. );
        dbl3 masterNormal=normal(faces[connectingFace],points);
        dbl3 Ce=0.5*(points[pI]+points[pPoints[pI][eI]]);
        dbl3 tmf=centre(faces[connectingFace],points)-Ce;
        tmf/=mag(tmf)+1e-15;

        for_(myEFs, fI) if ( !(myEFs[fI]==connectingFace) )  {
          dbl3 tf=centre(faces[myEFs[fI]],points)-Ce;
          tf/=mag(tf)+1e-15;
          scalar sin=tf&masterNormal;
          scalar cos=tf&tmf;
          const double PI=3.14159265;
          double angle=std::atan2 (sin,cos) * 180 / PI;

          if ( angle<0.) angle=360+angle;
          closeNess[fI]=angle;
        }

        if (!selectMin)        closeNess=-closeNess;

        label nei=std::distance(closeNess.begin(), std::min_element(closeNess.begin(), closeNess.end()));
        if (appendUnique(group1,myEFs[nei])==1)   ++nClctd;

      }
    }

  }

  return nClctd;
}


//=============================================================================================
//=============================================================================================
//=============================================================================================
//=============================================================================================


void correct( facePiece & faces, DynamicList<point> & points, bool handlNonManiflEdges )  {
  Info<<" "<<points.size()<<"  points and  "<<faces.size()<<"  faces in surface, looking for non-manifolds:"<<endl;

  // new points and changed faces
  DynamicList<point> addedPoints; addedPoints.reserve(points.size()/100+1);
  //DynamicList<face>  modifiedFaces(faces.size()/100+1);
  std::map<label,face>  changedFaces;//(faces.size()/100+1);
  //const auto& meshPoints = points;

  label nProblemPoints = 0;



  const labelListList pFaces  = pointFaces(points.size(),faces);
  const labelListList pPoints = getPointPoints(points.size(),faces);

  size_t nSkipped = 0;
  label iLastPoint=points.size()-1;
  for_(points, pI)  {
    const ints& myFaces = pFaces[pI];
    if (myFaces.size()>5)  {

      DynamicList<label> group1; group1.reserve(myFaces.size());

      group1.append(myFaces[0]);
      size_t nClctdTotal=1;

      //int gfI=0;
      {int nClctd; do { nClctd=0;
         for_(group1, gfI)  nClctd += collectManifoldFaces(pI, group1[gfI] ,group1,handlNonManiflEdges, faces, points, pPoints, pFaces);
         nClctdTotal+=nClctd;
      } while (nClctd>0);}
      for_(group1, gfI)
      nClctdTotal+= collectManifoldFaces(pI, group1[gfI] ,group1,handlNonManiflEdges, faces, points, pPoints, pFaces);

      if(nClctdTotal<myFaces.size())  {
        bool PreviouslyModified=false;
        for_(group1,gfI)   if (changedFaces.find(group1[gfI])!=changedFaces.end())    PreviouslyModified=true;


        if( (nClctdTotal<myFaces.size()-2) &&  (nClctdTotal>2) && !PreviouslyModified)  {
          ++nProblemPoints;

          addedPoints.append(points[pI]); ++iLastPoint;  ///. clone the point, the clone will go to the end
          ensure(group1.size());
          for_(group1,gfI)  {
            face modifedF=faces[group1[gfI]];        ///. get the face
            label index=distance(modifedF.begin(), find(modifedF.begin(),modifedF.end(),pI));

            auto modifOld=changedFaces.find(group1[gfI]);
            if (modifOld!=changedFaces.end())  modifedF=modifOld->second;

            if (index>=0) modifedF[index]=iLastPoint; ///. change the face // TODO change inline
            else          Info<<index<<"Error in collecting connected faces : negative array index "<<endl;

            changedFaces.insert({group1[gfI],modifedF});
            //modifiedFaces.append( modifedF );  ///. save the changed face
          }
        }
        else  ++nSkipped; //Info<<"Point "<<pI<<" skipped, as this will cause singly connected edges,  collected " <<  nClctdTotal<<" faces out of "<<myFaces.size()<<endl;
      }

    }
    //else if(myFaces.size()<3)  Info<<myFaces.size()<<" "<<pI<<":wrong point:("<<points[pI]<<"), nf:"<<myFaces.size()<<" \n";


  }

  if(nSkipped)  Info<<"  "<<nSkipped<<" points skipped, as this will cause singly connected edges"<<endl;


  Info<< nProblemPoints<< " multiply shared (nonmanifold) edges,  ";
  Info<<"addedPoints: "<<addedPoints.size() <<"  changedFaces: "<<changedFaces.size() <<endl;


  //points.pback(piece<point>(&addedPoints[0], &addedPoints.back()));
  points.insert( points.end(), addedPoints.begin(), addedPoints.end() );


  for(const auto& chngdF:changedFaces)  faces[chngdF.first] = chngdF.second;




  size_t nEdges=0;
  for_(pPoints,pI) for(auto pJ:pPoints[pI]) if(pJ>int(pI)) ++nEdges;
  Info<<"nEdges: \t"<<nEdges <<"\t;  nPoints: \t"<<len(pPoints) <<"\t;  nFaces: \t"<<len(faces)<<"\t;  Euler_characteristic: \t"<<len(faces)-int(nEdges-len(pPoints))<< "\n\n"; // used for Euler number computation

  Info<<faces.size()<<" faces "<<points.size()<<" points "<<endl;

}







static int getMinPos(const dbls& array) {
	scalar mn = array[0];
	int minPos = 0;
	for (size_t i = 1; i < array.size(); ++i) {
		if (mn > array[i]) { mn = array[i]; minPos = i; }
	}
	return minPos;
}

static int findPos(const std::vector<label>& list, label value) {
	for_(list, i) if (value == list[i]) return i;
	return -1;
}

inline int appendSinglyConnectedNeis(label pI, DynamicList<label>& group1, const SurfTopology& topo,
                                     const std::vector<face>& faces, const piece<point>& points,
                                     const ints& fMarks, bool handlemultipliConnectedEdges) {
	const bool selectMin = false;
	int nCollected = 0;

	const labelListList& pEdges = topo.pointEdges;
	const labelListList& eFaces = topo.edgeFaces;
	const auto& edges = topo.edges;

	for (size_t gfI = 0; gfI < group1.size(); ++gfI) {
		label connectingFace = group1[gfI];
		const ints& myEdges = pEdges[pI];
		for_(myEdges, myeI) {
			label eI = myEdges[myeI];
			const ints& myEdgeFaces = eFaces[eI];

			if (myEdgeFaces.size() == 2) {
				if (myEdgeFaces[0] == connectingFace) nCollected += appendUnique(group1, myEdgeFaces[1]);
				else if (myEdgeFaces[1] == connectingFace) nCollected += appendUnique(group1, myEdgeFaces[0]);
			}
			else if (myEdgeFaces.size() > 2) {
				bool isMyEdge = false;
				label cnctedMark = fMarks[connectingFace];
				label otherFI = -1;
				label anotherFI = -1;
				for_(myEdgeFaces, fI) {
					if (fMarks[myEdgeFaces[fI]] != cnctedMark) {
						if (otherFI == -1) otherFI = myEdgeFaces[fI];
						else if (fMarks[myEdgeFaces[fI]] != fMarks[otherFI]) anotherFI = myEdgeFaces[fI];
					}
					else if (myEdgeFaces[fI] == connectingFace) isMyEdge = true;
				}

				if (otherFI >= 0 && anotherFI >= 0) {
					nCollected += appendUnique(group1, otherFI);
					nCollected += appendUnique(group1, anotherFI);
				}

				if (isMyEdge && handlemultipliConnectedEdges) {
					dbls closeNess(myEdgeFaces.size(), selectMin ? 1000. : -1000.);
					vec3 masterNormal = normal(faces[connectingFace], points);
					vec3 Ce = 0.5 * (points[edges[eI][0]] + points[edges[eI][1]]);
					vec3 tmf = centre(faces[connectingFace], points) - Ce;
					tmf /= mag(tmf) + 1e-15;

					for_(myEdgeFaces, fI) {
						if (myEdgeFaces[fI] != connectingFace && fMarks[myEdgeFaces[fI]] == fMarks[connectingFace]) {
							vec3 tf = centre(faces[myEdgeFaces[fI]], points) - Ce;
							tf /= mag(tf) + 1e-15;
							scalar s = tf & masterNormal;
							scalar c = tf & tmf;
							const double PI = 3.14159265;
							double angle = std::atan2(s, c) * 180 / PI;
							if (angle < 0.) angle = 360 + angle;
							closeNess[fI] = angle;
						}
					}

					if (!selectMin) closeNess = -closeNess;

					label nei = getMinPos(closeNess);
					if (appendUnique(group1, myEdgeFaces[nei]) == 1) ++nCollected;
				}
			}
		}
	}
	return nCollected;
}

void correctMultiZone(std::vector<face>& faces, ints& fMarks, DynamicField<point>& points, bool handlemultipliConnectedEdges) {
	Info << "\t" << points.size() << " points,  " << faces.size() << " faces, correcting:  * ";
	cout.flush();

	DynamicList<point> addedPoints;
	DynamicList<face> modifiedFaces;
	DynamicList<label> changedFIndices;

	label nProblemPoints = 0;

	SurfTopology topo(points.size(), faces);
	const labelListList& pFaces = topo.pointFaces;
	const std::vector<face>& Sfaces = faces;

	const label nPoints = points.size();
	label iLastPoint = nPoints - 1;
	for (label pI = 0; pI < nPoints; ++pI) {
		const ints& myFaces = pFaces[pI];
		if (myFaces.size() > 5) {
			DynamicList<label> group1;
			group1.reserve(myFaces.size());
			group1.append(myFaces[0]);
			while (appendSinglyConnectedNeis(pI, group1, topo, Sfaces, points, fMarks, handlemultipliConnectedEdges));
			appendSinglyConnectedNeis(pI, group1, topo, Sfaces, points, fMarks, handlemultipliConnectedEdges);

			if (group1.size() < myFaces.size()) {
				if ((group1.size() < myFaces.size() - 2) && (group1.size() > 2)) {
					bool PreviouslyModified = false;
					for_(group1, gfI) if (findPos(changedFIndices, group1[gfI]) >= 0) PreviouslyModified = true;
					if (!PreviouslyModified) {
						nProblemPoints++;
						addedPoints.append(points[pI]);
						++iLastPoint;

						for_(group1, gfI) {
							face modifiedFace = Sfaces[group1[gfI]];
							label iModFace = findPos(changedFIndices, group1[gfI]);
							if (iModFace >= 0) {
								modifiedFace = modifiedFaces[iModFace];
								Info << iModFace << "Error in correcting faces : dbl cor face" << endl;
							}
							changedFIndices.append(group1[gfI]);
							label index = Sfaces[group1[gfI]].which(pI);
							if (index >= 0) modifiedFace[index] = iLastPoint;
							else Info << gfI << ":Error in correcting faces : negative array index " << index << endl;
							modifiedFaces.append(modifiedFace);
						}
					}
				}
				else Info << "Point " << pI << ", collected " << group1.size() << " faces out of " << myFaces.size() << ", skipped, as this will cause singly connected edges" << endl;
			}
		}
	}

	Info << nProblemPoints << " shared edges,  ";
	Info << "addedPoints: " << addedPoints.size() << "  changedFIndices: " << changedFIndices.size() << "  changedFaces: " << modifiedFaces.size() << "   ";

	points.insert(points.end(), addedPoints.begin(), addedPoints.end());
	for_(changedFIndices, i) faces[changedFIndices[i]] = modifiedFaces[i];

	Info << faces.size() << " faces " << points.size() << " points " << endl;
}

void correctbioti(std::vector<face>& faces, ints& fMarks, DynamicField<point>& points, int stage) {
	Info << points.size() << " points,  " << faces.size() << " faces,  stage:" << stage << "   correcting: ";
	cout.flush();

	DynamicList<label> deletedFacesIndices;
	label nProblemPoints = 0;
	label nbads = 0;

	std::vector<face>& Sfaces = faces;
	ints pMarks(points.size(), 0);
	std::vector<ints> pFaces(points.size());
	for_(Sfaces, fI) {
		const face& f = Sfaces[fI];
		for_(f, i) ++pMarks[f[i]];
	}
	for_(pFaces, pI) pFaces[pI].resize(pMarks[pI]);
	pMarks = -1;
	for_(Sfaces, fI) {
		const face& f = Sfaces[fI];
		for_(f, i) pFaces[f[i]][++pMarks[f[i]]] = fI;
	}

	ints pointPointmap(points.size(), -1);

	for_(pFaces, pI) {
		label pII = pI;
		label n3fs(0), delFI(-1);
		const ints& myFaces = pFaces[pI];
		if (myFaces.size() > 4) {
			label myMark(fMarks[myFaces[0]]);
			bool CLine(false);
			for_(myFaces, i) {
				const face& f = Sfaces[myFaces[i]];
				label mePIinf = f.which(pII);
				if (mePIinf < 0) {
					++nbads;
				}
				else if (pFaces[f.nextLabel(mePIinf)].size() == 3 && pFaces[f.prevLabel(mePIinf)].size() == 3) {
					++n3fs;
					delFI = myFaces[i];
				}
				if (myMark != fMarks[myFaces[i]]) CLine = true;
			}
			if (CLine && n3fs == 0 && stage == 2) {
				for_(myFaces, i) {
					const face& f = Sfaces[myFaces[i]];
					label mePIinf = f.which(pII);
					if (mePIinf < 0) {
						++nbads;
					}
					else {
						label opospI = f[(mePIinf + 2) % f.size()];
						if (pFaces[opospI].size() == 3 && pFaces[f.prevLabel(mePIinf)].size() > 6 && pFaces[f.nextLabel(mePIinf)].size() > 6) {
							++n3fs;
							delFI = myFaces[i];
							pII = f.prevLabel(mePIinf);
						}
					}
				}
			}
		}

		if ((n3fs == 1 || (stage >= 1 && n3fs >= 1)) && (findPos(deletedFacesIndices, delFI) < 0) && pointPointmap[pII] == -1 && pointPointmap[pI] == -1) {
			nProblemPoints++;
			const face& f = Sfaces[delFI];
			label mePIinf = f.which(pII);
			if (mePIinf < 0) {
				Info << "  bad:" << pI << " " << pII << "  ";
			}
			else {
				int keptP, delP;
				if (f.nextLabel(mePIinf) < f.prevLabel(mePIinf)) {
					keptP = f.nextLabel(mePIinf);
					delP = f.prevLabel(mePIinf);
				}
				else {
					delP = f.nextLabel(mePIinf);
					keptP = f.prevLabel(mePIinf);
				}

				if (pointPointmap[delP] == -1 && pointPointmap[keptP] == -1) {
					pointPointmap[delP] = keptP;
					deletedFacesIndices.append(delFI);
				}
			}
		}
	}

	Info << nProblemPoints << " dbl 3-nei points in face. bads:" << nbads << " *  ";

	{
		label iLastPoint = -1;
		for_(points, i) {
			if (pointPointmap[i] < 0) {
				points[++iLastPoint] = points[i];
				pointPointmap[i] = iLastPoint;
			}
			else {
				if (pointPointmap[i] >= label(i)) Info << " Errorsddsfdf " << endl;
				pointPointmap[i] = pointPointmap[pointPointmap[i]];
				points[pointPointmap[i]] = 0.5 * (points[i] + points[pointPointmap[i]]);
			}
		}
		points.resize(iLastPoint + 1);
	}
	Info << "    " << points.size() << " points  ";
	{
		for_(deletedFacesIndices, i) faces[deletedFacesIndices[i]].zone = -1;
		label iLastFace = -1;
		for_(Sfaces, i) {
			if (faces[i].zone >= 0) {
				face f = faces[i];
				for_(f, ii) f[ii] = pointPointmap[f[ii]];
				faces[++iLastFace] = f;
				fMarks[iLastFace] = fMarks[i];
			}
		}
		faces.resize(iLastFace + 1);
		fMarks.resize(iLastFace + 1);
	}
	Info << faces.size() << " faces " << endl;
}

surfMsh createMultiZoneSurface(const VoxelImage& vxlImg) {
	surfMsh srfmsh;
	int3 n = vxlImg.size3();
	int nx_2 = n[0] - 2, ny_2 = n[1] - 2, nz_2 = n[2] - 2;
	dbl3 X0 = vxlImg.X0();
	dbl3 dx = vxlImg.dx();
	X0 += dx;

	int nAllFaces = 0;
	for (int kk = 1; kk <= nz_2; kk++)
		for (int jj = 1; jj <= ny_2; jj++)
			for (int ii = 1; ii <= nx_2; ii++) {
				unsigned char vv = vxlImg(ii, jj, kk);
				if (vv == 1 || vv == 2) {
					if (ii != 1 && vv != vxlImg(ii - 1, jj, kk) && 1 != vxlImg(ii - 1, jj, kk)) ++nAllFaces;
					if (ii != nx_2 && vv != vxlImg(ii + 1, jj, kk) && 1 != vxlImg(ii + 1, jj, kk)) ++nAllFaces;
					if (jj != 1 && vv != vxlImg(ii, jj - 1, kk) && 1 != vxlImg(ii, jj - 1, kk)) ++nAllFaces;
					if (jj != ny_2 && vv != vxlImg(ii, jj + 1, kk) && 1 != vxlImg(ii, jj + 1, kk)) ++nAllFaces;
					if (kk != 1 && vv != vxlImg(ii, jj, kk - 1) && 1 != vxlImg(ii, jj, kk - 1)) ++nAllFaces;
					if (kk != nz_2 && vv != vxlImg(ii, jj, kk + 1) && 1 != vxlImg(ii, jj, kk + 1)) ++nAllFaces;
				}
			}

	Info << "nAllFaces: " << nAllFaces << endl;

	int iPoints = -1;
	VoxelField<int> point_mapper(n[0] + 1, n[1] + 1, n[2] + 1, -1);
	DynamicField<point>& points = srfmsh.points;
	std::vector<face> faces_All(nAllFaces);
	ints fMarks(nAllFaces);

	#define addPointToFace_m(pointIndex, kkk, jjj, iii, type) \
		if (point_mapper(iii, jjj, kkk) < 0) { \
			points.append(point(dx.x * (iii + 1.) + X0.x, dx.y * (jjj + 1.) + X0.y, dx.z * (kkk + 1.) + X0.z)); \
			point_mapper(iii, jjj, kkk) = ++iPoints; \
		} \
		faces_##type[i##type##Faces][pointIndex] = point_mapper(iii, jjj, kkk);

	#define recordFaces_m(l10, l11, l20, l21, l30, l31, ii, jj, kk, type) \
		{ \
			++i##type##Faces; \
			addPointToFace_m(0, ii, jj, kk, type) \
			addPointToFace_m(1, ii + l10, jj + l20, kk + l30, type) \
			addPointToFace_m(2, ii + l10 + l11, jj + l20 + l21, kk + l30 + l31, type) \
			addPointToFace_m(3, ii + l11, jj + l21, kk + l31, type) \
		}

	#define clockwiserecordFaces3(type)  recordFaces_m(0, 1, 1, 0, 0, 0, kk - 1, jj - 1, ii - 1, type)
	#define uclockwiserecordFaces3(type) recordFaces_m(1, 0, 0, 1, 0, 0, kk - 1, jj - 1, ii    , type)
	#define clockwiserecordFaces2(type)  recordFaces_m(1, 0, 0, 0, 0, 1, kk - 1, jj - 1, ii - 1, type)
	#define uclockwiserecordFaces2(type) recordFaces_m(0, 1, 0, 0, 1, 0, kk - 1, jj    , ii - 1, type)
	#define clockwiserecordFaces1(type)  recordFaces_m(0, 0, 0, 1, 1, 0, kk - 1, jj - 1, ii - 1, type)
	#define uclockwiserecordFaces1(type) recordFaces_m(0, 0, 1, 0, 0, 1, kk    , jj - 1, ii - 1, type)

	int iAllFaces = -1;
	for (int kk = 1; kk <= nz_2; kk++)
		for (int jj = 1; jj <= ny_2; jj++)
			for (int ii = 1; ii <= nx_2; ii++) {
				unsigned char vv = vxlImg(ii, jj, kk);
				if (vv == 1 || vv == 2) {
					if (ii != 1 && vv != vxlImg(ii - 1, jj, kk) && 1 != vxlImg(ii - 1, jj, kk)) {
						clockwiserecordFaces3(All);
						fMarks[iAllFaces] = vv + vxlImg(ii - 1, jj, kk);
					}
					if (ii != nx_2 && vv != vxlImg(ii + 1, jj, kk) && 1 != vxlImg(ii + 1, jj, kk)) {
						uclockwiserecordFaces3(All);
						fMarks[iAllFaces] = vv + vxlImg(ii + 1, jj, kk);
					}
					if (jj != 1 && vv != vxlImg(ii, jj - 1, kk) && 1 != vxlImg(ii, jj - 1, kk)) {
						clockwiserecordFaces2(All);
						fMarks[iAllFaces] = vv + vxlImg(ii, jj - 1, kk);
					}
					if (jj != ny_2 && vv != vxlImg(ii, jj + 1, kk) && 1 != vxlImg(ii, jj + 1, kk)) {
						uclockwiserecordFaces2(All);
						fMarks[iAllFaces] = vv + vxlImg(ii, jj + 1, kk);
					}
					if (kk != 1 && vv != vxlImg(ii, jj, kk - 1) && 1 != vxlImg(ii, jj, kk - 1)) {
						clockwiserecordFaces1(All);
						fMarks[iAllFaces] = vv + vxlImg(ii, jj, kk - 1);
					}
					if (kk != nz_2 && vv != vxlImg(ii, jj, kk + 1) && 1 != vxlImg(ii, jj, kk + 1)) {
						uclockwiserecordFaces1(All);
						fMarks[iAllFaces] = vv + vxlImg(ii, jj, kk + 1);
					}
				}
			}

	#undef addPointToFace_m
	#undef recordFaces_m
	#undef clockwiserecordFaces3
	#undef uclockwiserecordFaces3
	#undef clockwiserecordFaces2
	#undef uclockwiserecordFaces2
	#undef clockwiserecordFaces1
	#undef uclockwiserecordFaces1

	point_mapper.reset(0, 0, 0, 0);
	Info << "nPoints: " << points.size() << "      " << endl;

	correctMultiZone(faces_All, fMarks, points, true);
	correctMultiZone(faces_All, fMarks, points, true);
	correctMultiZone(faces_All, fMarks, points, true);
	correctMultiZone(faces_All, fMarks, points, true);
	correctMultiZone(faces_All, fMarks, points, false);
	correctMultiZone(faces_All, fMarks, points, false);
	correctMultiZone(faces_All, fMarks, points, false);
	correctMultiZone(faces_All, fMarks, points, false);

	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 0);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 1);
	correctbioti(faces_All, fMarks, points, 1);

	std::vector<face>& facesSorted_All = srfmsh.faces;
	facesSorted_All.resize(faces_All.size());
	ints zoneSizes(3, 0);

	int indF = -1;
	for (size_t i = 0; i < faces_All.size(); ++i)
		if (fMarks[i] == 1) { facesSorted_All[++indF] = faces_All[i]; facesSorted_All[indF].zone = 1; }
	zoneSizes[0] = indF + 1;
	for (size_t i = 0; i < faces_All.size(); ++i)
		if (fMarks[i] == 2) { facesSorted_All[++indF] = faces_All[i]; facesSorted_All[indF].zone = 2; }
	zoneSizes[1] = indF + 1 - zoneSizes[0];
	for (size_t i = 0; i < faces_All.size(); ++i)
		if (fMarks[i] == 3) { facesSorted_All[++indF] = faces_All[i]; facesSorted_All[indF].zone = 3; }
		else if (fMarks[i] > 3) Info << " Error wrong label in the image" << endl;
	zoneSizes[2] = indF + 1 - zoneSizes[1] - zoneSizes[0];

	Info << zoneSizes[0] << " + " << zoneSizes[1] << " + " << zoneSizes[2] << " = " << indF + 1 << endl;
	return srfmsh;
}






surfMsh createSurface(const InputFile& inp, const VoxelImage& vxlImg,  bool unitIsVoxels)  {
  surfMsh srfmsh;
  facePieceList& faces_bs = srfmsh.faces_bs;
  DynamicField<point>& points = srfmsh.points;

  Info<<__FUNCTION__<<": "<<endl;
  int3 n = vxlImg.size3() - 2;
  dbl3 dx = unitIsVoxels ? dbl3(1.0) : vxlImg.dx();
  dbl3 X0 = unitIsVoxels ? dbl3(0.0) : vxlImg.X0() + dx;
  Info<< " dx: "<< dx <<", X0: "<< X0 <<endl; // greped by tests


  std::array<size_t,256> nFaces; nFaces.fill(0);

  for (int iz=1;iz<=n[2];++iz)
   for (int iy=1;iy<=n[1];++iy)
    for (int ix=1;ix<=n[0];++ix) {
      //if (!vxlImg(ix,iy,iz))  {
        unsigned char vv=vxlImg(ix,iy,iz);
        unsigned char
        neiv=vxlImg(ix-1,iy,iz);
        if (neiv>vv) ++nFaces[vv];

        neiv=vxlImg(ix+1,iy,iz);
        if (neiv>vv) ++nFaces[vv];

        neiv=vxlImg(ix,iy-1,iz);
        if (neiv>vv) ++nFaces[vv];

        neiv=vxlImg(ix,iy+1,iz);
        if (neiv>vv) ++nFaces[vv];

        neiv=vxlImg(ix,iy,iz-1);
        if (neiv>vv) ++nFaces[vv];

        neiv=vxlImg(ix,iy,iz+1);
        if (neiv>vv) ++nFaces[vv];
      //}
    }

  for(int ib=0;ib<256;++ib)  if(nFaces[ib])  cout<< " Faces_"<<ib<<" "<<nFaces[ib]<<endl;
  cout<<endl;




  (cout<<"creating faces, ").flush();

  {   size_t nAll=0;
    for(int ib=0;ib<256;++ib)  nAll+=nFaces[ib];
    srfmsh.faces.resize(nAll);
    nAll=0;
    for(int ib=0;ib<256;++ib)  { faces_bs[ib].reset(facePiece(&srfmsh.faces[nAll], nFaces[ib]));  nAll+=nFaces[ib]; }
  }

  VoxelField<int> point_mapper(n[0]+1,n[1]+1,n[2]+1,-1);




  cout<<"creating surface mesh"<<endl;
  int iPoints=-1;
  auto point_mapper_insert = [&point_mapper, &points,  &dx,  &X0, &iPoints](int iix, int iiy, int iiz) ->
   int {
    if (point_mapper(iix,iiy,iiz)<0)  {
      points.append(point(dx[0]*iix+X0[0],dx[1]*iiy+X0[1],dx[2]*iiz+X0[2]));
      point_mapper(iix,iiy,iiz)=++iPoints;
      return  iPoints;
    }
    return point_mapper(iix,iiy,iiz);
   };


  #define recordF_m( l10,l11,l20,l21,l30,l31, ii,jj,kk,type )            \
    {                                                                     \
      ++iFaces[type] ;                                                     \
      /*faces_bs[type][iFaces[type]].setSize(4+1);*/                                \
      faces_bs[type][iFaces[type]][0]=point_mapper_insert(ii,jj,kk);                    \
      faces_bs[type][iFaces[type]][1]=point_mapper_insert(ii+l11,jj+l21,kk+l31);           \
      faces_bs[type][iFaces[type]][2]=point_mapper_insert(ii+l10+l11,jj+l20+l21,kk+l30+l31);  \
      faces_bs[type][iFaces[type]][3]=point_mapper_insert(ii+l10,jj+l20,kk+l30);             \
      faces_bs[type][iFaces[type]].zone=neiv;                 \
    }


  #define  kclockwiserecordF(type) recordF_m( 1,0,0,1,0,0,   ix-1,iy-1,iz-1, type)
  #define kuclockwiserecordF(type) recordF_m( 0,1,1,0,0,0,   ix-1,iy-1,iz  , type)
  #define  jclockwiserecordF(type) recordF_m( 0,1,0,0,1,0,   ix-1,iy-1,iz-1, type)
  #define juclockwiserecordF(type) recordF_m( 1,0,0,0,0,1,   ix-1,iy  ,iz-1, type)
  #define  iclockwiserecordF(type) recordF_m( 0,0,1,0,0,1,   ix-1,iy-1,iz-1, type)
  #define iuclockwiserecordF(type) recordF_m( 0,0,0,1,1,0,   ix  ,iy-1,iz-1, type)





  std::array<int,256> iFaces; iFaces.fill(-1);

  for (int iz=1;iz<=n[2];iz++)  {
    if(iz%20==9) { (cout<<(iz%100 ? '.' : '\n')).flush(); }
    for (int iy=1;iy<=n[1];iy++)
      for (int ix=1; ix<=n[0]; ix++) {
        //if (!vxlImg(ix,iy,iz))
        unsigned char vv=vxlImg(ix,iy,iz);

        unsigned char
        neiv=vxlImg(ix-1,iy,iz);
        if (neiv>vv)  {iclockwiserecordF( vv)}

        neiv=vxlImg(ix+1,iy,iz);
        if (neiv>vv)  {iuclockwiserecordF( vv)}


        neiv=vxlImg(ix,iy-1,iz);
        if (neiv>vv)  {jclockwiserecordF( vv)}

        neiv=vxlImg(ix,iy+1,iz);
        if (neiv>vv)  {juclockwiserecordF( vv)}


        neiv=vxlImg(ix,iy,iz-1);
        if (neiv>vv)  {kclockwiserecordF( vv)}

        neiv=vxlImg(ix,iy,iz+1);
        if (neiv>vv)  {kuclockwiserecordF( vv)}
      }
  }


//______________________________________________

  point_mapper.reset(0,0,0,0);


  Info<<"\nnPoints: "<<points.size()<<"  "<<endl;

  if (inp.getOr("correctSurface", true))
   for(auto& facezs:faces_bs) if(facezs.size())  {
    correct(facezs,points,true);
    correct(facezs,points,true);
    correct(facezs,points,true);
    correct(facezs,points,true);
    correct(facezs,points,true);
    correct(facezs,points,false);
    correct(facezs,points,true);
    correct(facezs,points,false);
    correct(facezs,points,false);
    correct(facezs,points,false);
   }

  return srfmsh;

}

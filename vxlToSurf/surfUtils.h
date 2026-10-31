/*-------------------------------------------------------------------------*\
 Light weight surface utility functions, alternatives to those in openfoam.

 Developed by:
 - A. Q. Raeini (2018-2026)
\*-------------------------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "typses.h"
#include "InputFile.h"

template<typename T> class VoxelImageT;
using VoxelImage = VoxelImageT<unsigned char>;

using label = int;
using scalar = double;
using word = std::string;

class face : public std::array<label, 4>
{
public:
	int zone;
	face() : std::array<label, 4>{0, 0, 0, 0}, zone(0) {}
	face(const std::array<label, 4>& pointIs, int z = 0) : std::array<label, 4>(pointIs), zone(z) {}
	int nextLabel(int pI) const { return (*this)[(pI + 1) & 3]; }
	int prevLabel(int pI) const { return (*this)[(pI + 3) & 3]; }
	int which(label p) const {
		for (int i = 0; i < 4; ++i) if ((*this)[i] == p) return i;
		return -1;
	}
	std::array<label, 2> faceEdge(int pI) const { return {(*this)[pI], (*this)[(pI + 1) & 3]}; }
	void flip() { std::swap((*this)[1], (*this)[3]); }
};

template <typename T> using DynamicList = std::vector<T>;
template <typename T> using DynamicField = std::vector<T>;
typedef std::vector<std::vector<label>> labelDynamicListList;
typedef std::vector<ints> labelListList;
typedef ints labelList;
typedef piece<face> facePiece;
typedef std::array<facePiece, 256> facePieceList;

#ifdef __EMSCRIPTEN__
typedef float3 point;
typedef float3 vec3;
typedef float3 real3;
typedef vars<float3> vectorField;
typedef vars<float3> vec3s;
typedef float real;
typedef floats reals;
#else
typedef dbl3 point;
typedef dbl3 vec3;
typedef dbl3 real3;
typedef dbl3s vectorField;
typedef dbl3s vec3s;
typedef double real;
typedef dbls reals;
#endif

struct surfMsh {
	std::vector<face> faces;
	facePieceList faces_bs;
	DynamicField<point> points;
	std::map<std::string, reals> cdata; // face data
	std::map<std::string, reals> pdata; // node data

	void reserve(size_t nf, size_t np) { faces.reserve(nf); points.reserve(np); }
	void movePoints(const piece<point>& newPts) { points.assign(newPts.begin(), newPts.end()); }
};

#define append  push_back
#define setSize resize
#define Info    std::cout

surfMsh createSurface(const InputFile& inp, const VoxelImage& vxlImg, bool unitIsVoxels);
surfMsh createMultiZoneSurface(const VoxelImage& vxlImg);
int smoothSurf(const InputFile& inp, facePieceList& facezs, piece<point> points);
void writeSurfaceFiles(const facePieceList& facezsz, const piece<point> pointsAll, const std::string& fnams);
void writeMergeSurfaceFile(const facePieceList& facezsz, const piece<point> pointsAll, const std::string& fnams);
void writeVTK(const std::string& fnam, const surfMsh& srf);
surfMsh readVTK(const std::string& fnam);
inline surfMsh readConvertSurface(const std::string& fnam) { return readVTK(fnam); }

dbl3s mapFacesGetPoints(std::vector<std::vector<face>>& facess, const piece<point> pointsAll);
std::vector<std::vector<face>> getbsoleteOrderedFaces(const facePieceList& facezsz, int vv);

inline int appendUnique(DynamicList<label>& dynList, label value)
{
	for_(dynList, i) if (dynList[i] == value) return 0;
	dynList.append(value);
	return 1;
}

template<typename T, template<typename ...> class C>
int findPosi(const C<T>& list, const T& val) { return distance(list.begin(), find(list.begin(), list.end(), val)); }
template<typename T, template<typename ...> class C>
int hasValue(const C<T>& list, const T& val) { return find(list.begin(), list.end(), val) != list.end(); }
template<class C>
typename C::const_iterator nextCircIter(const C& f, typename C::const_iterator itr) { return ++itr == f.end() ? f.begin() : itr; }
template<class C>
typename C::const_iterator prevCircIter(const C& f, typename C::const_iterator itr) { return itr == f.begin() ? f.end() - 1 : --itr; }

inline labelListList getPointPoints(size_t nPoints, const facePiece& faces)
{
	labelDynamicListList pntPntsTmp(nPoints);
	labelListList pntPoints(nPoints);
	for_(faces, fI)
	{
		const face& f = faces[fI];
		for (int pI = 0; pI < 4; ++pI)
		{
			appendUnique(pntPntsTmp[f[pI]], f.nextLabel(pI));
			appendUnique(pntPntsTmp[f[pI]], f.prevLabel(pI));
		}
	}

	for_(pntPoints, i) pntPoints[i] = pntPntsTmp[i];
	return pntPoints;
}

inline labelListList getPointPoints(size_t nPoints, const std::vector<facePiece>& facess)
{
	labelDynamicListList pntPntsTmp(nPoints);
	labelListList pntPoints(nPoints);
	for (auto& faces : facess) for_(faces, fI)
	{
		const face& f = faces[fI];
		for (int pI = 0; pI < 4; ++pI)
		{
			appendUnique(pntPntsTmp[f[pI]], f.nextLabel(pI));
			appendUnique(pntPntsTmp[f[pI]], f.prevLabel(pI));
		}
	}

	for_(pntPoints, i) pntPoints[i] = pntPntsTmp[i];
	return pntPoints;
}

inline labelListList pointIFaces(size_t nPoints, const facePiece& faces) {
	labelDynamicListList pointFacesTmp(nPoints);
	labelListList pFaces(nPoints);
	for_(faces, fI) {
		const face f = faces[fI];
		for_(f, i) appendUnique(pointFacesTmp[f[i]], i);
	}
	for_(pointFacesTmp, i) pFaces[i] = pointFacesTmp[i];
	return pFaces;
}

inline labelListList pointFaces(size_t nPoints, const facePiece& faces) {
	labelDynamicListList pointFacesTmp(nPoints);
	labelListList pFaces(nPoints);
	for_(faces, fI)
	{
		const face f = faces[fI];
		for (auto pI : f) appendUnique(pointFacesTmp[pI], fI);
	}

	for_(pFaces, i) pFaces[i] = pointFacesTmp[i];
	return pFaces;
}

inline labelListList edgeFaces(const ints& myPPoints, const facePiece& faces, const ints& myPFaces) {
	labelDynamicListList edgFacsTmp(myPPoints.size());
	labelListList edgFacs(myPPoints.size());
	for_(myPFaces, mfI) {
		const face& f = faces[myPFaces[mfI]];
		for (auto pI : f) {
			for_(myPPoints, neIp)
				if (myPPoints[neIp] == pI)
				{ appendUnique(edgFacsTmp[neIp], myPFaces[mfI]); break; }
		}
	}

	for_(edgFacs, i) edgFacs[i] = edgFacsTmp[i];
	return edgFacs;
}

/// Surface connectivity used by the mesh-repair and contact-line layer code.
/// Point indices are global (indices into the surface points); points not used by any face have empty lists.
/// Edges are numbered in the order they are first met when looping over faces and their edges,
/// and keep the orientation of that first face; all face lists are in increasing face order.
struct SurfTopology {
	labelListList pointFaces; ///< faces using each point
	std::vector<std::array<label, 2>> edges;
	labelListList edgeFaces;  ///< faces sharing each edge (can be > 2 on non-manifold / three-phase edges)
	labelListList pointEdges; ///< edges connected to each point

	SurfTopology(size_t nPoints, const facePiece& faces) {
		std::vector<std::vector<label>> pf(nPoints), pe(nPoints);
		std::vector<std::vector<label>> ef;
		std::map<std::pair<label, label>, label> edgeId; // (low, high) point index -> edge index
		for_(faces, fI) {
			const face& f = faces[fI];
			for (int i = 0; i < 4; ++i) {
				if (pf[f[i]].empty() || pf[f[i]].back() != label(fI)) pf[f[i]].push_back(fI);
				auto e = f.faceEdge(i);
				auto key = std::minmax(e[0], e[1]);
				auto [it, isNew] = edgeId.try_emplace({key.first, key.second}, label(edges.size()));
				if (isNew) {
					edges.push_back(e);
					ef.emplace_back();
					pe[e[0]].push_back(it->second);
					pe[e[1]].push_back(it->second);
				}
				auto& eFs = ef[it->second];
				if (eFs.empty() || eFs.back() != label(fI)) eFs.push_back(fI);
			}
		}
		pointFaces.resize(nPoints);
		pointEdges.resize(nPoints);
		for (size_t i = 0; i < nPoints; ++i) { pointFaces[i] = pf[i]; pointEdges[i] = pe[i]; }
		edgeFaces.resize(ef.size());
		for_(ef, i) edgeFaces[i] = ef[i];
	}
};

inline void deriveMarks(const facePiece& faces, size_t nPoints, ints& fMarks, ints& pMarks) {
	fMarks = ints(faces.size(), 1);
	pMarks = ints(nPoints, 2);
	for_(faces, i) {
		fMarks[i] = faces[i].zone ? faces[i].zone : 1;
		if (fMarks[i] == 1) for (auto p : faces[i]) pMarks[p] = 1;
	}
	for_(faces, i) {
		if (fMarks[i] == 3) for (auto p : faces[i]) pMarks[p] = 3;
	}
	for_(faces, i) {
		if (fMarks[i] == 2) for (auto p : faces[i]) if (pMarks[p] != 2) pMarks[p] = 4;
	}
}

inline reals computePWeights(const facePiece& faces, const labelListList& pNeips, const ints& fMarks, const ints& pMarks) {
	reals pWeights(pNeips.size(), 1.);
	for (int ii = 0; ii < 3; ++ii) {
		for_(faces, i) if (fMarks[i] == 2)
			for (auto p : faces[i]) if (pMarks[p] != 2) pWeights[p] = 0.25;
		reals pWeightsOrig(pWeights);
		for_(pNeips, pI) {
			const ints& neiPoints = pNeips[pI];
			scalar avgPWeights(pWeightsOrig[pI]);
			scalar sumWeightsKc(1.);
			for (label pJ : neiPoints) {
				scalar weight = 1.;
				avgPWeights += weight * pWeightsOrig[pJ];
				sumWeightsKc += weight;
			}
			if (sumWeightsKc > 0.5) {
				avgPWeights /= sumWeightsKc;
				pWeights[pI] = 0.99 * avgPWeights + (1. - 0.99) * pWeightsOrig[pI];
			} else {
				pWeights[pI] = pWeightsOrig[pI];
			}
		}
	}
	return pWeights;
}

inline dbl3 areax2(const face& f, const piece<point>& points) {
	dbl3 diag = points[f[2]] - points[f[0]];
	return ((points[f[1]] - points[f[0]]) ^ diag) + (diag ^ (points[f[3]] - points[f[0]]));
}
inline dbl3 normal(const face& f, const piece<point>& points) { dbl3 aa = areax2(f, points); return aa / (mag(aa) + 1e-32); }
/// Face centre: area-weighted average of the centroids of the 4 triangles (vertex average, edge i)
inline dbl3 centre(const face& f, const piece<point>& points) {
	const dbl3 xAvg = 0.25 * (points[f[0]] + points[f[1]] + points[f[2]] + points[f[3]]);
	scalar areaSum = 0;  // sum of 2 x triangle areas
	dbl3 momentSum(0, 0, 0); // sum of 2 x area x 3 x triangle centroid
	for (int i = 0; i < 4; ++i) {
		const point& a = points[f[i]];
		const point& b = points[f.nextLabel(i)];
		scalar area2 = mag((a - xAvg) ^ (b - xAvg));
		areaSum += area2;
		momentSum += area2 * (a + b + xAvg);
	}
	return areaSum > 1e-37 ? momentSum / (3. * areaSum) : xAvg;
}
inline dbl3s faceCentres(const facePiece& facez, const piece<point>& points) {
	dbl3s res(facez.size());
	for (size_t fi = 0; fi < facez.size(); ++fi) res[fi] = centre(facez[fi], points);
	return res;
}
inline vars<dbl3s> faceCentres(const std::vector<facePiece>& facezs, const piece<point>& points) {
	vars<dbl3s> res(facezs.size());
	for (size_t iz = 0; iz < facezs.size(); ++iz) res[iz] = faceCentres(facezs[iz], points);
	return res;
}

inline void computePointAreas(vec3s& pAw, vec3s& pAs, const std::vector<face>& faces, const piece<point> newPoints,
                              const piece<int> fMarks = piece<int>{}, const piece<int> pMarks = piece<int>{}) {
	pAw = vec3s(newPoints.size(), vec3(0, 0, 0));

	if (fMarks.size()) {
		pAs = vec3s(newPoints.size(), vec3(0, 0, 0));

		for_(faces, fI) {
			const face& fac = faces[fI];
			vec3 cf = centre(fac, newPoints);
			for_(fac, pI) {
				int e0 = fac[pI], e1 = fac[(pI + 1) & 3];
				vec3 Ce = 0.5 * (newPoints[e0] + newPoints[e1]);

				vec3 AEdgeTri = 0.5 * ((newPoints[e0] - cf) ^ (Ce - cf));

				if (pMarks[e0] == fMarks[fI]) {
					pAw[e0] += AEdgeTri;
					pAs[e0] += AEdgeTri;
				}
				else if (fMarks[fI] != 2) pAw[e0] += AEdgeTri;
				else                      pAs[e0] += AEdgeTri;

				AEdgeTri = 0.5 * ((newPoints[e1] - cf) ^ (Ce - cf));

				if (pMarks[e1] == fMarks[fI]) {
					pAw[e1] -= AEdgeTri;
					pAs[e1] -= AEdgeTri;
				}
				else if (fMarks[fI] != 2) pAw[e1] -= AEdgeTri;
				else                      pAs[e1] -= AEdgeTri;
			}
		}
	}
	else {
		for_(faces, fI) {
			const face& fac = faces[fI];
			vec3 cf = centre(fac, newPoints);
			for_(fac, pI) {
				int e0 = fac[pI], e1 = fac[(pI + 1) & 3];
				vec3 Ce = 0.5 * (newPoints[e0] + newPoints[e1]);

				vec3 AEdgeTri = 0.5 * ((newPoints[e0] - cf) ^ (Ce - cf));
				pAw[e0] += AEdgeTri;
				AEdgeTri = 0.5 * ((newPoints[e1] - cf) ^ (Ce - cf));
				pAw[e1] -= AEdgeTri;
			}
		}
	}
}

inline void extrapolateCLNormals(vec3s& pNs, const labelListList& pNeips, const ints& pMarks) {
	for_(pNeips, pI) {
		if (pMarks[pI] == 4) {
			vec3 pNsNei(0, 0, 0);
			for (label pJ : pNeips[pI]) {
				if (pMarks[pJ] == 2) pNsNei += pNs[pJ];
			}
			if (mag(pNsNei) > 0.1) {
				pNsNei /= mag(pNsNei);
				pNs[pI] = 1.333 * pNs[pI] - 0.333 * pNsNei;
				pNs[pI] /= mag(pNs[pI]);
			}
		}
	}
}

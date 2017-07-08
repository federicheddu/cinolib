/*********************************************************************************
*  Copyright(C) 2016: Marco Livesu                                               *
*  All rights reserved.                                                          *
*                                                                                *
*  This file is part of CinoLib                                                  *
*                                                                                *
*  CinoLib is dual-licensed:                                                     *
*                                                                                *
*   - For non-commercial use you can redistribute it and/or modify it under the  *
*     terms of the GNU General Public License as published by the Free Software  *
*     Foundation; either version 3 of the License, or (at your option) any later *
*     version.                                                                   *
*                                                                                *
*   - If you wish to use it as part of a commercial software, a proper agreement *
*     with the Author(s) must be reached, based on a proper licensing contract.  *
*                                                                                *
*  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE       *
*  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.     *
*                                                                                *
*  Author(s):                                                                    *
*                                                                                *
*     Marco Livesu (marco.livesu@gmail.com)                                      *
*     http://pers.ge.imati.cnr.it/livesu/                                        *
*                                                                                *
*     Italian National Research Council (CNR)                                    *
*     Institute for Applied Mathematics and Information Technologies (IMATI)     *
*     Via de Marini, 6                                                           *
*     16149 Genoa,                                                               *
*     Italy                                                                      *
**********************************************************************************/
#include <cinolib/meshes/abstract_surface_mesh.h>

namespace cinolib
{

template<class M, class V, class E, class F>
CINO_INLINE
uint AbstractSurfaceMesh<M,V,E,F>::vert_opposite_to(const uint eid, const uint vid) const
{
    assert(this->edge_contains_vert(eid, vid));
    if (this->edge_vert_id(eid,0) != vid) return this->edge_vert_id(eid,0);
    else                                  return this->edge_vert_id(eid,1);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::verts_are_ordered_CCW(const uint fid, const uint curr, const uint prev) const
{
    uint prev_offset = this->face_vert_offset(fid, prev);
    uint curr_offset = this->face_vert_offset(fid, curr);
    if (curr_offset == (prev_offset+1)%this->verts_per_face(fid)) return true;
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void AbstractSurfaceMesh<M,V,E,F>::vert_ordered_one_ring(const uint vid,
                                                         std::vector<uint> & v_ring,       // sorted list of adjacent vertices
                                                         std::vector<uint> & f_ring,       // sorted list of adjacent triangles
                                                         std::vector<uint> & e_ring,       // sorted list of edges incident to vid
                                                         std::vector<uint> & e_link) const // sorted list of edges opposite to vid
{
    v_ring.clear();
    f_ring.clear();
    e_ring.clear();
    e_link.clear();

    if (this->adj_v2e(vid).empty()) return;
    uint curr_e  = this->adj_v2e(vid).front(); assert(edge_is_manifold(curr_e));
    uint curr_v  = this->vert_opposite_to(curr_e, vid);
    uint curr_f  = this->adj_e2f(curr_e).front();
    // impose CCW winding...
    if (!this->verts_are_ordered_CCW(curr_f, curr_v, vid)) curr_f = this->adj_e2f(curr_e).back();

    // If there are boundary edges it is important to start from the right triangle (i.e. right-most),
    // otherwise it will be impossible to cover the entire umbrella
    std::vector<uint> b_edges = vert_boundary_edges(vid);
    if (b_edges.size()  > 0)
    {
        assert(b_edges.size() == 2); // otherwise there is no way to cover the whole umbrella walking through adjacent triangles!!!

        uint e = b_edges.front();
        uint f = this->adj_e2f(e).front();
        uint v = vert_opposite_to(e, vid);

        if (!this->verts_are_ordered_CCW(f, v, vid))
        {
            e = b_edges.back();
            f = this->adj_e2f(e).front();
            v = vert_opposite_to(e, vid);
            assert(this->verts_are_ordered_CCW(f, v, vid));
        }

        curr_e = e;
        curr_f = f;
        curr_v = v;
    }

    do
    {
        e_ring.push_back(curr_e);
        f_ring.push_back(curr_f);

        uint off = this->face_vert_offset(curr_f, curr_v);
        for(uint i=0; i<this->verts_per_face(curr_f)-1; ++i)
        {
            curr_v = this->face_vert_id(curr_f,(off+i)%this->verts_per_face(curr_f));
            if (i>0) e_link.push_back( this->face_edge_id(curr_f, curr_v, v_ring.back()) );
            v_ring.push_back(curr_v);
        }

        curr_e = this->face_edge_id(curr_f, vid, v_ring.back()); assert(edge_is_manifold(curr_e));
        curr_f = (this->adj_e2f(curr_e).front() == curr_f) ? this->adj_e2f(curr_e).back() : this->adj_e2f(curr_e).front();

        v_ring.pop_back();

        if (edge_is_boundary(curr_e)) e_ring.push_back(curr_e);
    }
    while(e_ring.size() < this->adj_v2e(vid).size());
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<uint> AbstractSurfaceMesh<M,V,E,F>::vert_ordered_vert_ring(const uint vid) const
{
    std::vector<uint> v_ring; // sorted list of adjacent vertices
    std::vector<uint> f_ring; // sorted list of adjacent triangles
    std::vector<uint> e_ring; // sorted list of edges incident to vid
    std::vector<uint> e_link; // sorted list of edges opposite to vid
    vert_ordered_one_ring(vid, v_ring, f_ring, e_ring, e_link);
    return v_ring;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<uint> AbstractSurfaceMesh<M,V,E,F>::vert_ordered_face_ring(const uint vid) const
{
    std::vector<uint> v_ring; // sorted list of adjacent vertices
    std::vector<uint> f_ring; // sorted list of adjacent triangles
    std::vector<uint> e_ring; // sorted list of edges incident to vid
    std::vector<uint> e_link; // sorted list of edges opposite to vid
    vert_ordered_one_ring(vid, v_ring, f_ring, e_ring, e_link);
    return f_ring;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<uint> AbstractSurfaceMesh<M,V,E,F>::vert_ordered_edge_ring(const uint vid) const
{
    std::vector<uint> v_ring; // sorted list of adjacent vertices
    std::vector<uint> f_ring; // sorted list of adjacent triangles
    std::vector<uint> e_ring; // sorted list of edges incident to vid
    std::vector<uint> e_link; // sorted list of edges opposite to vid
    vert_ordered_one_ring(vid, v_ring, f_ring, e_ring, e_link);
    return e_ring;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<uint> AbstractSurfaceMesh<M,V,E,F>::vert_ordered_edge_link(const uint vid) const
{
    std::vector<uint> v_ring; // sorted list of adjacent vertices
    std::vector<uint> f_ring; // sorted list of adjacent triangles
    std::vector<uint> e_ring; // sorted list of edges incident to vid
    std::vector<uint> e_link; // sorted list of edges opposite to vid
    vert_ordered_one_ring(vid, v_ring, f_ring, e_ring, e_link);
    return e_link;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
double AbstractSurfaceMesh<M,V,E,F>::vert_area(const uint vid) const
{
    double area = 0.0;
    for(uint fid : this->adj_v2f(vid)) area += face_area(fid)/static_cast<double>(this->verts_per_face(fid));
    return area;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
double AbstractSurfaceMesh<M,V,E,F>::vert_mass(const uint vid) const
{
    return vert_area(vid);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::vert_is_boundary(const uint vid) const
{
    for(uint eid : this->adj_v2e(vid)) if (edge_is_boundary(eid)) return true;
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<uint> AbstractSurfaceMesh<M,V,E,F>::vert_boundary_edges(const uint vid) const
{
    std::vector<uint> b_edges;
    for(uint eid : this->adj_v2e(vid)) if (edge_is_boundary(eid)) b_edges.push_back(eid);
    return b_edges;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::edge_is_manifold(const uint eid) const
{
    return (this->adj_e2f(eid).size() <= 2);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::edge_is_boundary(const uint eid) const
{
    return (this->adj_e2f(eid).size() < 2);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::edges_share_face(const uint eid1, const uint eid2) const
{
    for(uint fid1 : this->adj_e2f(eid1))
    for(uint fid2 : this->adj_e2f(eid2))
    {
        if (fid1 == fid2) return true;
    }
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
ipair AbstractSurfaceMesh<M,V,E,F>::edge_shared(const uint fid0, const uint fid1) const
{
    std::vector<uint> shared_verts;
    uint v0 = this->face_vert_id(fid0,0);
    uint v1 = this->face_vert_id(fid0,1);
    uint v2 = this->face_vert_id(fid0,2);

    if (this->face_contains_vert(fid1,v0)) shared_verts.push_back(v0);
    if (this->face_contains_vert(fid1,v1)) shared_verts.push_back(v1);
    if (this->face_contains_vert(fid1,v2)) shared_verts.push_back(v2);
    assert(shared_verts.size() == 2);

    ipair e;
    e.first  = shared_verts.front();
    e.second = shared_verts.back();
    return e;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
int AbstractSurfaceMesh<M,V,E,F>::face_shared(const uint eid0, const uint eid1) const
{
    for(uint fid0 : this->adj_e2f(eid0))
    for(uint fid1 : this->adj_e2f(eid1))
    {
        if (fid0 == fid1) return fid0;
    }
    return -1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
int AbstractSurfaceMesh<M,V,E,F>::face_adjacent_along(const uint fid, const uint vid0, const uint vid1) const
{
    // WARNING : assume the edge vid0,vid1 is manifold!
    uint eid = this->face_edge_id(fid, vid0, vid1);
    assert(edge_is_manifold(eid));

    for(uint nbr : this->adj_f2f(fid))
    {
        if (this->face_contains_vert(nbr,vid0) &&
            this->face_contains_vert(nbr,vid1))
        {
            return nbr;
        }
    }
    return -1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool AbstractSurfaceMesh<M,V,E,F>::face_is_boundary(const uint fid) const
{
    return (this->adj_f2f(fid).size() < 3);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
double AbstractSurfaceMesh<M,V,E,F>::face_mass(const uint fid) const
{
    return face_area(fid);
}



}
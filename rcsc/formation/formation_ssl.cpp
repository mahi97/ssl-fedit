// -*-c++-*-

/*!
  \file formation_cdt.cpp
  \brief Constrained Delaunay Triangulation formation data classes Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "formation_ssl.h"

#include <rcsc/geom/segment_2d.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/math_util.h>

#include <sstream>
#include <cstdio>

namespace rcsc {

using namespace formation;

const std::string FormationSSL::NAME( "SSLFormation" );

namespace {

inline
double
round_coord( const double & val )
{
    return rint( val / SampleData::PRECISION ) * SampleData::PRECISION;
}

}

/*-------------------------------------------------------------------*/
/*!

 */
FormationSSL::FormationSSL(int _count)
    : Formation()
{
    count = _count;
    for ( int i = 0; i < 11; ++i )
    {
        M_role_name[i] = "Dummy";
    }
}



/*-------------------------------------------------------------------*/
/*!

 */
void
FormationSSL::createDefaultData()
{

    createNewRole( 1, "LeftAttacker", Formation::SIDE );
    //M_formation->createNewRole( 2, "RightAttacker", Formation::SIDE );
//	if (count >= 2)
//		setSymmetryType( 2, 1, "RightAttacker" );

    SampleData data;
    data.ball_.assign( 0.0, 0.0 );
	if (count == 1)
	{
		data.players_.push_back( Vector2D( 1.0, -1.5 ) );
	}
    if (count == 2)
    {
        data.players_.push_back( Vector2D( 1.0, -1.5 ) );
        data.players_.push_back( Vector2D( 1.0, 1.5 ) );
    }
    if (count == 3)
    {
        createNewRole( 3, "Center", Formation::CENTER );
        data.players_.push_back( Vector2D( 1.0, -1.5 ) );
        data.players_.push_back( Vector2D( 1.0, 1.5 ) );
        data.players_.push_back( Vector2D( -0.5, 0 ) );
    }
    if (count == 4)
    {
        createNewRole( 3, "LeftHalfBack", Formation::SIDE );
        setSymmetryType( 4, 3, "RightHalfBack" );
        data.players_.push_back( Vector2D( 1.0, -1.5 ) );
        data.players_.push_back( Vector2D( 1.0, 1.5 ) );
        data.players_.push_back( Vector2D( 0.0, -1.0 ) );
        data.players_.push_back( Vector2D( 0.0, 1.0 ) );
    }
    if (count == 5)
    {
        createNewRole( 3, "LeftHalfBack", Formation::SIDE );
        setSymmetryType( 4, 3, "RightHalfBack" );
        createNewRole( 5, "Center", Formation::CENTER );
        data.players_.push_back( Vector2D( 1.0, -1.5 ) );
        data.players_.push_back( Vector2D( 1.0, 1.5 ) );
        data.players_.push_back( Vector2D( 0.0, -1.0 ) );
        data.players_.push_back( Vector2D( 0.0, 1.0 ) );
        data.players_.push_back( Vector2D( -0.5, 0 ) );
    }
    for (int i=count+1;i<=11;i++)
        data.players_.push_back( Vector2D( 3.22, 2.22 ) );
    M_samples->addData( *this, data, false );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationSSL::setRoleName( const int unum,
                           const std::string & name )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return;
    }

    M_role_name[unum - 1] = name;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string
FormationSSL::getRoleName( const int unum ) const
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return std::string( "" );
    }

    return M_role_name[unum - 1];
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationSSL::createNewRole( const int unum,
                             const std::string & role_name,
                             const Formation::SideType type )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return;
    }

    setRoleName( unum, role_name );

    switch ( type ) {
    case Formation::CENTER:
        setCenterType( unum );
        break;
    case Formation::SIDE:
        setSideType( unum );
        break;
    case Formation::SYMMETRY:
        std::cerr << __FILE__ << ":" << __LINE__
                  << " ***ERROR*** Invalid side type "
                  << std::endl;
        break;
    default:
        break;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
FormationSSL::getPosition( const int unum,
                           const Vector2D & focus_point ) const
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return Vector2D::INVALIDATED;
    }

    const Triangulation::Triangle * tri
        = M_triangulation.findTriangleContains( focus_point );

    // linear interpolation
    return interpolate( unum, focus_point, tri );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationSSL::getPositions( const Vector2D & focus_point,
                            std::vector< Vector2D > & positions ) const
{
    positions.clear();

    const Triangulation::Triangle * tri = M_triangulation.findTriangleContains( focus_point );

    for ( int unum = 1; unum <= 11; ++unum )
    {
        positions.push_back( interpolate( unum, focus_point, tri ) );
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
FormationSSL::interpolate( const int unum,
                           const Vector2D & focus_point,
                           const Triangulation::Triangle * tri ) const
{

    if ( ! tri )
    {
        const int v_index = M_triangulation.findNearestPoint( focus_point );

        if ( v_index < 0 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** No vertex!"
                      << std::endl;
            return Vector2D::INVALIDATED;
        }

        try
        {
            //std::cerr << "found nearest vertex id= " << v->id() << std::endl;
            return M_sample_vector.at( v_index ).getPosition( unum );
        }
        catch ( std::exception & e )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " exception caught! "
                      << e.what() << std::endl;
            return Vector2D::INVALIDATED;
        }
    }

    try
    {
        const Vector2D vertex_0 = M_sample_vector.at( tri->v0_ ).ball_;
        const Vector2D vertex_1 = M_sample_vector.at( tri->v1_ ).ball_;
        const Vector2D vertex_2 = M_sample_vector.at( tri->v2_ ).ball_;

        Vector2D result_0 = M_sample_vector.at( tri->v0_ ).getPosition( unum );
        Vector2D result_1 = M_sample_vector.at( tri->v1_ ).getPosition( unum );
        Vector2D result_2 = M_sample_vector.at( tri->v2_ ).getPosition( unum );

        Line2D line_0( vertex_0, focus_point );
        Segment2D segment_12( vertex_1, vertex_2 );
        Vector2D intersection_12 = segment_12.intersection( line_0 );

        if ( ! intersection_12.isValid() )
        {
            if ( focus_point.dist2( vertex_0 ) < 1.0e-5 )
            {
                return result_0;
            }

            std::cerr << __FILE__ << ":" << __LINE__
                      << "***ERROR*** No intersection!\n"
                      << " focus=" << focus_point
                      << " line_intersection=" << intersection_12
                      << "\n v0=" << vertex_0
                      << " v1=" << vertex_1
                      << " v2=" << vertex_2
                      << std::endl;

            return ( result_0 + result_1 + result_2 ) / 3.0;
        }

        // distance from vertex_? to interxection_12
        double dist_1i = vertex_1.dist( intersection_12 );
        double dist_2i = vertex_2.dist( intersection_12 );

        // interpolation result of vertex_1 & vertex_2
        Vector2D result_12
            = result_1
            + ( result_2 - result_1 ) * ( dist_1i / ( dist_1i + dist_2i ) );

        // distance from vertex_0 to ball
        double dist_0b = vertex_0.dist( focus_point );
        // distance from interxectin_12 to ball
        double dist_ib = intersection_12.dist( focus_point );

        // interpolation result of vertex_0 & intersection_12
        Vector2D result_012
            = result_0
            + ( result_12 - result_0 ) * ( dist_0b / ( dist_0b + dist_ib ) );

        return result_012;
    }
    catch ( std::exception & e )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " Exception caught!! "
                  << e.what()
                  << std::endl;
        return  Vector2D::INVALIDATED;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationSSL::train()
{
    if ( ! M_samples )
    {
        return;
    }

    M_triangulation.clear();
    M_sample_vector.clear();

    const SampleDataSet::DataCont::const_iterator d_end = M_samples->dataCont().end();
    for ( SampleDataSet::DataCont::const_iterator d = M_samples->dataCont().begin();
          d != d_end;
          ++d )
    {
        M_triangulation.addPoint( d->ball_ );
        M_sample_vector.push_back( *d );
    }

    const SampleDataSet::Constraints::const_iterator c_end = M_samples->constraints().end();
    for ( SampleDataSet::Constraints::const_iterator c = M_samples->constraints().begin();
          c != c_end;
          ++c )
    {
        M_triangulation.addConstraint( static_cast< size_t >( c->first->index_ ),
                                       static_cast< size_t >( c->second->index_ ) );
    }


    M_triangulation.compute();
    //M_triangulation.triangulate();
    //M_triangulation.constrain();
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationSSL::readConf( std::istream & is )
{
    if ( ! readRoles( is ) )
    {
        return false;
    }

    if ( ! readVertices( is ) )
    {
        return false;
    }

    //
    // read End tag
    //

    std::string line_buf;
    while ( std::getline( is, line_buf ) )
    {
        if ( line_buf.empty()
             || line_buf[0] == '#'
             || ! line_buf.compare( 0, 2, "//" ) )
        {
            continue;
        }

        if ( line_buf != "End" )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " *** ERROR *** readRolesV2(). Failed getline "
                      << std::endl;
            return false;
        }

        break;
    }

    if ( is.eof() )
    {
        std::cerr << "Input stream reaches EOF"
                  << std::endl;
    }


    train();
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationSSL::readSamples( std::istream & )
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationSSL::readRoles( std::istream & is )
{
    std::string line_buf;

    //
    // read Begin tag
    //

    while ( std::getline( is, line_buf ) )
    {
        if ( line_buf.empty()
             || line_buf[0] == '#'
             || ! line_buf.compare( 0, 2, "//" ) )
        {
            continue;
        }

        if ( line_buf != "Begin Roles" )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " *** ERROR *** readRolesV2(). Illegal header ["
                      << line_buf << ']'
                      << std::endl;
            return false;
        }

        break;
    }

    //
    // read role data
    //

    for ( int unum = 1; unum <= 11; ++unum )
    {
        while ( std::getline( is, line_buf ) )
        {
            if ( line_buf.empty()
                 || line_buf[0] == '#'
                 || ! line_buf.compare( 0, 2, "//" ) )
            {
                continue;
            }
            break;
        }

        int read_unum = 0;
        char role_name[128];
        int symmetry_number = 0;

        if ( std::sscanf( line_buf.c_str(),
                          " %d %127s %d ",
                          &read_unum, role_name, &symmetry_number ) != 3
             || read_unum != unum )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " *** ERROR *** readRolesV2(). Illegal role data. num="
                      << unum
                      << " [" << line_buf << "]"
                      << std::endl;
            return false;
        }

        //
        // create role or set symmetry.
        //
        const Formation::SideType type = ( symmetry_number == 0
                                           ? Formation::CENTER
                                           : symmetry_number < 0
                                           ? Formation::SIDE
                                           : Formation::SYMMETRY );
        if ( type == Formation::CENTER )
        {
            createNewRole( unum, role_name, type );
        }
        else if ( type == Formation::SIDE )
        {
            createNewRole( unum, role_name, type );
        }
        else
        {
            setSymmetryType( unum, symmetry_number, role_name );
        }
    }

    //
    // read End tag
    //

    while ( std::getline( is, line_buf ) )
    {
        if ( line_buf.empty()
             || line_buf[0] == '#'
             || ! line_buf.compare( 0, 2, "//" ) )
        {
            continue;
        }

        if ( line_buf != "End Roles" )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " *** ERROR *** readRolesV2(). Failed getline "
                      << std::endl;
            return false;
        }

        break;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationSSL::readVertices( std::istream & is )
{
    M_samples = SampleDataSet::Ptr( new SampleDataSet() );

    if ( ! M_samples->read( is ) )
    {
        M_samples.reset();
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationSSL::readConstraints( std::istream & )
{
    //std::cerr << "FormationSSL::readConstraints()" << std::endl;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationSSL::printConf( std::ostream & os ) const
{
    printRoles( os );
    printVertices( os );
    printConstraints( os );

    os << "End" << std::endl;
    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationSSL::printSamples( std::ostream & os ) const
{
    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationSSL::printRoles( std::ostream & os ) const
{
    os << "Begin Roles\n";

    for ( int unum = 1; unum <= 11; ++unum )
    {
        os << unum << ' '
           << M_role_name[unum - 1] << ' '
           << M_symmetry_number[unum - 1] << '\n';
    }

    os << "End Roles\n";

    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationSSL::printVertices( std::ostream & os ) const
{
    M_samples->print( os );
    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationSSL::printConstraints( std::ostream & os ) const
{
    std::cerr << "FormationSSL::printConstraints()" << std::endl;
    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
namespace {

Formation::Ptr
create()
{
    Formation::Ptr ptr( new FormationSSL() );
    return ptr;
}

rcss::RegHolder f = Formation::creators().autoReg( &create,
                                                   FormationSSL::NAME );

}

}

// -*-c++-*-

/*!
  \file formation_rbf.cpp
  \brief RBF formation data classes Source File.
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

#include "formation_rbf.h"

#include <rcsc/math_util.h>

#include <boost/random.hpp>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace rcsc {

using namespace formation;

const std::string FormationRBF::NAME( "RBF" );

const double FormationRBF::Param::PITCH_LENGTH = 105.0 + 10.0;
const double FormationRBF::Param::PITCH_WIDTH = 68.0 + 10.0;


/*-------------------------------------------------------------------*/
/*!

 */
FormationRBF::Param::Param()
    : M_net( 2, 2 )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
FormationRBF::Param::getPosition( const Vector2D & ball_pos,
                                  const Formation::SideType ) const
{
    RBFNetwork::input_vector input( 2, 0.0 );

    input[0] = ball_pos.x;
    input[1] = ball_pos.y;

    RBFNetwork::output_vector output;

    M_net.propagate( input, output );

    return Vector2D( bound( -PITCH_LENGTH*0.5, output[0], PITCH_LENGTH*0.5 ),
                     bound( -PITCH_WIDTH*0.5, output[1], PITCH_WIDTH*0.5 ) );
}

/*-------------------------------------------------------------------*/
/*!
  Format:  Role <RoleNameStr>
*/
bool
FormationRBF::Param::readRoleName( std::istream & is )
{
    std::string line_buf;
    if ( ! std::getline( is, line_buf ) )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read" << std::endl;
        return false;
    }

    std::istringstream istr( line_buf );
    if ( ! istr.good() )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read" << std::endl;
        return false;
    }

    std::string tag;
    istr >> tag;
    if ( tag != "Role" )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read" << std::endl;
        return false;
    }

    istr >> M_role_name;
    if ( M_role_name.empty() )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read role name" << std::endl;
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationRBF::Param::readParam( std::istream & is )
{
    std::string line_buf;
    if ( ! std::getline( is, line_buf ) )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read" << std::endl;
        return false;
    }

    std::istringstream istr( line_buf );
    if ( ! istr.good() )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read" << std::endl;
        return false;
    }

    if ( ! M_net.read( istr ) )
    {
        std::cerr  << __FILE__ << ":" << __LINE__
                   << " Failed to read position param" << std::endl;
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationRBF::Param::read( std::istream & is )
{
    // read role name
    if ( ! readRoleName( is ) )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " Failed to read role name" << std::endl;
        return false;
    }

    if ( ! readParam( is ) )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " Failed to read parameters" << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationRBF::Param::printRoleName( std::ostream & os ) const
{
    if ( M_role_name.empty() )
    {
        os << "Role Default\n";
    }
    else
    {
        os << "Role " << M_role_name << '\n';
    }
    return os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
FormationRBF::Param::printParam( std::ostream & os ) const
{
    M_net.print( os ) << '\n';
    return os;
}

/*-------------------------------------------------------------------*/
/*
  Role <role name>
  <bko.x> <bkoy>
  <offense playon>
  <defense playon>
  ...

*/
std::ostream &
FormationRBF::Param::print( std::ostream & os ) const
{
    printRoleName( os );
    printParam( os );

    return os << std::flush;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
FormationRBF::FormationRBF()
    : Formation()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationRBF::createDefaultData()
{
    // 1: goalie
    // 2: left center back
    // 3(2): right center back
    // 4: left side back
    // 5(4): right side back
    // 6: defensive half
    // 7: left offensive half
    // 8(7): left side half
    // 9(8): right side half
    // 10: left forward
    // 11(10): right forward
/*    createNewRole( 1, "Goalie", Formation::CENTER );
    createNewRole( 2, "CenterBack", Formation::SIDE );
    setSymmetryType( 3, 2, "CenterBack" );
    createNewRole( 4, "SideBack", Formation::SIDE );
    setSymmetryType( 5, 4, "SideBack" );
    createNewRole( 6, "DefensiveHalf", Formation::CENTER );
    createNewRole( 7, "OffensiveHalf", Formation::SIDE );
    setSymmetryType( 8, 7, "OffensiveHalf" );
    createNewRole( 9, "SideForward", Formation::SIDE );
    setSymmetryType( 10, 9, "SideForward" );
    createNewRole( 11, "CenterForward", Formation::CENTER );

    SampleData data;

    data.ball_.assign( 0.0, 0.0 );
    data.players_.push_back( Vector2D( -50.0, 0.0 ) );
    data.players_.push_back( Vector2D( -20.0, -8.0 ) );
    data.players_.push_back( Vector2D( -20.0, 8.0 ) );
    data.players_.push_back( Vector2D( -18.0, -18.0 ) );
    data.players_.push_back( Vector2D( -18.0, 18.0 ) );
    data.players_.push_back( Vector2D( -15.0, 0.0 ) );
    data.players_.push_back( Vector2D( 0.0, -12.0 ) );
    data.players_.push_back( Vector2D( 0.0, 12.0 ) );
    data.players_.push_back( Vector2D( 10.0, -22.0 ) );
    data.players_.push_back( Vector2D( 10.0, 22.0 ) );
    data.players_.push_back( Vector2D( 10.0, 0.0 ) );

	M_samples->addData( *this, data, false );*/

	createNewRole( 1, "LeftAttacker", Formation::SIDE );
	//M_formation->createNewRole( 2, "RightAttacker", Formation::SIDE );
	setSymmetryType( 2, 1, "RightAttacker" );

	SampleData data;
	data.ball_.assign( 0.0, 0.0 );
	int count = 2;

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
	for (int i=count+1;i<=11;i++)
		data.players_.push_back( Vector2D( 3.22, 2.22 ) );
	M_samples->addData( *this, data, false );

}


/*-------------------------------------------------------------------*/
/*!

 */
void
FormationRBF::setRoleName( const int unum,
                           const std::string & name )
{
    boost::shared_ptr< FormationRBF::Param > p = getParam( unum );

    if ( ! p )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " You cannot set the role name of player "
                  << unum
                  << std::endl;
        return;
    }

    p->setRoleName( name );
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string
FormationRBF::getRoleName( const int unum ) const
{
    const boost::shared_ptr< const FormationRBF::Param > p = param( unum );
    if ( ! p )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " You cannot get the role name of player "
                  << unum
                  << std::endl;
        return std::string( "" );;
    }

    return p->roleName();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationRBF::createNewRole( const int unum,
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

    if ( type == Formation::CENTER )
    {
        setCenterType( unum );
    }
    else if ( type == Formation::SIDE )
    {
        setSideType( unum );
    }
    else
    {
        // symmetry
    }

    // erase old parameter, if exist
    std::map< int, boost::shared_ptr< FormationRBF::Param > >::iterator it
        = M_param_map.find( unum );
    if ( it != M_param_map.end() )
    {
        M_param_map.erase( it );
    }

    boost::shared_ptr< FormationRBF::Param > param( new FormationRBF::Param );
    param->setRoleName( role_name );

    M_param_map.insert( std::make_pair( unum, param ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
FormationRBF::getPosition( const int unum,
                           const Vector2D & ball_pos ) const
{
    const boost::shared_ptr< const FormationRBF::Param > ptr = param( unum );
    if ( ! ptr )
    {
        std::cerr << __FILE__ << ':' << __LINE__
                  << " *** ERROR *** FormationRBF::Param not found. unum = "
                  << unum
                  << std::endl;
        return Vector2D::INVALIDATED;
    }
    Formation::SideType type = Formation::SIDE;
    if ( M_symmetry_number[unum - 1] > 0 )  type = Formation::SYMMETRY;
    if ( M_symmetry_number[unum - 1] == 0 ) type = Formation::CENTER;

    return ptr->getPosition( ball_pos, type );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationRBF::getPositions( const Vector2D & focus_point,
                            std::vector< Vector2D > & positions ) const
{
    positions.clear();

    for ( int unum = 1; unum <= 11; ++unum )
    {
        positions.push_back( getPosition( unum, focus_point ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
FormationRBF::train()
{
    if ( ! M_samples
         || M_samples->dataCont().empty() )
    {
        return;
    }

    std::cerr << "FormationRBF::train. Started!!" << std::endl;

    for ( int unum = 1; unum <= 11; ++unum )
    {
        int number = unum;

        boost::shared_ptr< FormationRBF::Param > param = getParam( number );
        if ( ! param )
        {
            std::cerr << __FILE__ << ": " << __LINE__
                      << " *** ERROR ***  No formation parameter for player " << unum
                      << std::endl;
            break;
        }

        RBFNetwork & net = param->getNet();
        std::cerr << "FormationRBF::train. " << unum
                  << " current unit size = "
                  << net.units().size()
                  << std::endl;

        if ( net.units().size() < M_samples->dataCont().size() )
        {
            SampleDataSet::DataCont::const_iterator d = M_samples->dataCont().begin();
            int count = net.units().size();
            std::cerr << "FormationRBF::train. need to add new center "
                      << M_samples->dataCont().size() - net.units().size() << std::endl;
            while ( --count >= 0
                    && d != M_samples->dataCont().end() )
            {
                std::cerr << "FormationRBF::train. skip known data...." << std::endl;
                ++d;
            }

            while ( d != M_samples->dataCont().end() )
            {
                std::cerr << "FormationRBF::train. added new center "
                          << d->ball_
                          << std::endl;
                RBFNetwork::input_vector center( 2, 0.0 );
                center[0] = d->ball_.x;
                center[1] = d->ball_.y;
                net.addCenter( center );

                ++d;
            }
        }

        net.printUnits( std::cerr );

        RBFNetwork::input_vector input( 2, 0.0 );
        RBFNetwork::output_vector teacher( 2, 0.0 );

        const SampleDataSet::DataCont::const_iterator d_end = M_samples->dataCont().end();
        int loop = 0;
        double ave_err = 0.0;
        double max_err = 0.0;
        bool success = false;
        while ( ++loop <= 5000 )
        {
            ave_err = 0.0;
            max_err = 0.0;
            double data_count = 1.0;
            for ( SampleDataSet::DataCont::const_iterator d = M_samples->dataCont().begin();
                  d != d_end;
                  ++d, data_count += 1.0 )
            {
                input[0] = d->ball_.x;
                input[1] = d->ball_.y;
                teacher[0] = d->players_[unum - 1].x;
                teacher[1] = d->players_[unum - 1].y;

                if ( loop == 2 )
                {
                    std::cerr << "  ----> " << unum
                              << "  ball = " << input[0] << ", " << input[1]
                              << "  teacher = " << teacher[0] << ", " << teacher[1]
                              << std::endl;
                }

                double err = net.train( input, teacher );
                if ( max_err < err )
                {
                    max_err = err;
                }
                ave_err
                    = ave_err * ( ( data_count - 1.0 ) / data_count )
                    + err / data_count;
            }

            if ( max_err < 0.001 )
            {
                std::cerr << "  ----> converged. average err=" << ave_err
                          << "  last max err=" << max_err
                          << std::endl;
                success = true;
                //printMessageWithTime( "train. converged. loop=%d", loop );
                break;
            }
        }
        if ( ! success )
        {
            std::cerr << "  *** Failed to converge *** " << std::endl;
            //printMessageWithTime( "train. Failed to converge. player %d", unum );
        }
        std::cerr << "  ----> " << loop
                  << " loop. last average err=" << ave_err
                  << "  last max err=" << max_err
                  << std::endl;
        net.printUnits( std::cerr );
    }
    std::cerr << "FormationRBF::train. Ended!!" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

 */
boost::shared_ptr< FormationRBF::Param >
FormationRBF::getParam( const int unum )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return boost::shared_ptr< FormationRBF::Param >
            ( static_cast< FormationRBF::Param * >( 0 ) );
    }

    std::map< int, boost::shared_ptr< FormationRBF::Param > >::const_iterator
        it = M_param_map.find( unum );

    if ( it == M_param_map.end() )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** Parameter not found! unum = "
                  << unum << std::endl;
        return boost::shared_ptr< FormationRBF::Param >
            ( static_cast< FormationRBF::Param * >( 0 ) );
    }

    return it->second;
}

/*-------------------------------------------------------------------*/
/*!

 */
boost::shared_ptr< const FormationRBF::Param >
FormationRBF::param( const int unum ) const
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** invalid unum " << unum
                  << std::endl;
        return boost::shared_ptr< const FormationRBF::Param >
            ( static_cast< FormationRBF::Param * >( 0 ) );
    }

    std::map< int, boost::shared_ptr< FormationRBF::Param > >::const_iterator
        it = M_param_map.find( unum );

    if ( it == M_param_map.end() )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " *** ERROR *** Parameter not found! unum = "
                  << unum << std::endl;
        return boost::shared_ptr< const FormationRBF::Param >
            ( static_cast< FormationRBF::Param * >( 0 ) );
    }

    return it->second;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationRBF::readConf( std::istream & is )
{
    if ( ! readPlayers( is ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
FormationRBF::readPlayers( std::istream & is )
{
    int player_read = 0;
    std::string line_buf;

    //---------------------------------------------------
    // read each player's parameter set
    for ( int i = 0; i < 11; ++i )
    {
        if ( ! std::getline( is, line_buf ) )
        {
            break;
        }

        // check id
        int unum = 0;
        int symmetry = 0;
        int n_read = 0;
        if ( std::sscanf( line_buf.c_str(),
                          " player %d %d %n ",
                          &unum, &symmetry,
                          &n_read ) != 2
             || n_read == 0 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** failed to read formation at number "
                      << i + 1
                      << " [" << line_buf << "]"
                      << std::endl;
            return false;
        }
        if ( unum != i + 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** failed to read formation "
                      << " Invalid player number.  Expected " << i + 1
                      << "  but read number = " << unum
                      << std::endl;
            return false;
        }
        if ( symmetry == unum )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** failed to read formation."
                      << " Invalid symmetry number. at "
                      << i
                      << " mirroing player itself. unum = " << unum
                      << "  symmetry = " << symmetry
                      << std::endl;
            return false;
        }
        if ( 11 < symmetry )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** failed to read formation."
                      << " Invalid symmetry number. at "
                      << i
                      << "  Symmetry number is out of range. unum = " << unum
                      << "  symmetry = " << symmetry
                      << std::endl;
            return false;
        }

        M_symmetry_number[i] = symmetry;

        // this player is symmetry type
        /*
          if ( symmetry > 0 )
          {
          ++player_read;
          continue;
          }
        */

        // read parameters
        boost::shared_ptr< FormationRBF::Param > param( new FormationRBF::Param );
        if ( ! param->read( is ) )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** failed to read formation. at number "
                      << i + 1
                      << " [" << line_buf << "]"
                      << std::endl;
            return false;
        }

        ++player_read;
        M_param_map.insert( std::make_pair( unum, param ) );
    }

    if ( player_read != 11 )
    {
        std::cerr << __FILE__ << ':' << __LINE__
                  << " ***ERROR*** Invalid formation format."
                  << " The number of read player is " << player_read
                  << std::endl;
        return false;
    }

    if ( ! std::getline( is, line_buf )
         || line_buf != "End" )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << " ***ERROR*** Illegal end tag."
                  << std::endl;
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
FormationRBF::printConf( std::ostream & os ) const
{
    for ( int i = 0; i < 11; ++i )
    {
        const int unum = i + 1;
        os << "player " << unum << " " << M_symmetry_number[i] << '\n';
        /*
          if ( M_symmetry_number[i] > 0 )
          {
          continue;
          }
        */

        std::map< int, boost::shared_ptr< FormationRBF::Param > >::const_iterator
            it = M_param_map.find( unum );
        if ( it == M_param_map.end() )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " *** ERROR *** Invalid player Id at number "
                      << i + 1
                      << ".  No formation param!"
                      << std::endl;
        }
        else
        {
            it->second->print( os );
        }
    }

    os << "End" << std::endl;
    return os;
}


/*-------------------------------------------------------------------*/
/*!

*/
namespace {

Formation::Ptr
create()
{
    Formation::Ptr ptr( new FormationRBF() );
    return ptr;
}

rcss::RegHolder f = Formation::creators().autoReg( &create,
                                                   FormationRBF::NAME );

}

}

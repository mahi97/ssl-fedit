// -*-c++-*-

/*!
  \file coach_agent.cpp
  \brief basic coach agent Header File
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

#include "coach_agent.h"

#include "coach_audio_sensor.h"
#include "coach_config.h"
#include "coach_command.h"
#include "global_visual_sensor.h"
#include "global_world_model.h"

#include <rcsc/common/basic_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/common/team_graphic.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/param/param_map.h>
#include <rcsc/param/conf_file_parser.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/version.h>

#include <sstream>
#include <cstring>

namespace rcsc {

///////////////////////////////////////////////////////////////////////
/*!
  \struct Impl
  \brief coach agent implementation
*/
struct CoachAgent::Impl {

    //! reference to the agent instance
    CoachAgent & agent_;

    //! flag to check if (think) message was received or not.
    bool think_received_;

    //! flag to check if server cycle is stopped or not.
    bool server_cycle_stopped_;

    //! last action decision game time
    GameTime last_decision_time_;

    //! current game time
    GameTime current_time_;

    //! referee info
    GameMode game_mode_;

    //! visual sensor data
    GlobalVisualSensor visual_;

    //! audio sensor
    CoachAudioSensor audio_;

    /*!
      \brief initialize all members
    */
    Impl( CoachAgent & agent )
        : agent_( agent ),
          think_received_( false ),
          server_cycle_stopped_( true ),
          last_decision_time_( -1, 0 ),
          current_time_( 0, 0 )
      { }

    /*!
      \brief open offline client log file.
    */
    bool openOfflineLog();

    /*!
      \brief open debug log file.
    */
    bool openDebugLog();

    /*!
      \brief send init or reconnect command to server

      init commad is sent in BasicClient's run() method
      Do not call this method yourself!
     */
    void sendInitCommand();

    /*!
      \brief send disconnection command message to server
      and set the server status to end.
     */
    void sendByeCommand();

    /*!
      \brief analyze init replay message
      \param msg raw server message
    */
    void analyzeInit( const char * msg );

    /*!
      \brief analyze cycle info in server message
      \param msg raw server message
      \param by_see_global if message type is see_global, this value becomes true
      \return parsing result status
     */
    bool analyzeCycle( const char * msg,
                       const bool by_see_global );

    /*!
      \brief analyze see_global message
      \param msg raw server message
     */
    void analyzeSeeGlobal( const char * msg );

    /*!
      \brief analyze hear message
      \param msg raw server message
     */
    void analyzeHear( const char * msg );

    /*!
      \brief analyze referee message
      \param msg raw server message
     */
    void analyzeHearReferee( const char * msg );

    /*!
      \brief analyze audio message from player
      \param msg raw server message
    */
    void analyzeHearPlayer( const char * msg );

    /*!
      \brief analyze change_player_type message
      \param msg raw server message
    */
    void analyzeChangePlayerType( const char * msg );

    /*!
      \brief analyze player_type parameter message
      \param msg raw server message
    */
    void analyzePlayerType( const char * msg );

    /*!
      \brief analyze player_param parameter message
      \param msg raw server message
    */
    void analyzePlayerParam( const char * msg );

    /*!
      \brief analyze server_param parameter message
      \param msg raw server message
    */
    void analyzeServerParam( const char * msg );

    /*!
      \brief analyze clang version message
      \param msg raw server message
    */
    void analyzeCLangVer( const char * msg );

    /*!
      \brief analyze score message
      \param msg raw server message
    */
    void analyzeScore( const char * msg );

    /*!
      \brief analyze ok message
      \param msg raw server message
    */
    void analyzeOK( const char * msg );

    /*!
      \brief analyze ok team_graphic_? message
      \param msg raw server message
    */
    void analyzeOKTeamGraphic( const char * msg );

    /*!
      \brief analyze ok teamnames message
      \param msg raw server message
     */
    void analyzeTeamNames( const char * msg );

    /*!
      \brief analyze error message
      \param msg raw server message
    */
    void analyzeError( const char * msg );

    /*!
      \brief analyze warning message
      \param msg raw server message
    */
    void analyzeWarning( const char * msg );

    /*!
      \brief analyze include message
      \param msg raw server message
    */
    void analyzeInclude( const char * msg );

    /*!
      \brief update current time using analyzed time value
      \param new_time analyzed time value
      \param by_see_global true if called after see_global message
    */
    void updateCurrentTime( const long & new_time,
                            const bool by_see_global );

    /*!
      \brief update server game cycle status.

      This method must be called just after referee message
    */
    void updateServerStatus();
};


/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::updateCurrentTime( const long & new_time,
                                     const bool by_see_global )
{
    if ( server_cycle_stopped_ )
    {
        if ( new_time != current_time_.cycle() )
        {
            dlog.addText( Logger::LEVEL_ANY,
                          "CYCLE %ld-0 --------------------"
                          " return from cycle stop",
                          new_time );
            if ( new_time - 1 != current_time_.cycle() )
            {
                std::cerr << "coach: server cycle stopped mode:"
                          << " previous server time is incorrect?? "
                          << current_time_ << " -> " << new_time
                          << std::endl;
                dlog.addText( Logger::SYSTEM,
                              "server cycle stopped mode:"
                              " previous server time is incorrect?? "
                              " (%ld, %ld) -> %ld",
                              current_time_.cycle(), current_time_.stopped(),
                              new_time );
            }

            current_time_.assign( new_time, 0 );
        }
        else
        {
            if ( by_see_global )
            {
                dlog.addText( Logger::LEVEL_ANY,
                              "CYCLE %ld-%ld --------------------"
                              " stopped time was updated by see_global",
                              current_time_.cycle(), current_time_.stopped() + 1 );
                current_time_.assign( current_time_.cycle(),
                                      current_time_.stopped() + 1 );
            }
        }
    }
    // normal case
    else
    {
        if ( current_time_.cycle() != new_time )
        {
            dlog.addText( Logger::LEVEL_ANY,
                          "CYCLE %ld-0  -------------------------------------------------",
                          new_time );
        }
        current_time_.assign( new_time, 0 );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::updateServerStatus()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //  called just after referee.parse()

    // if current mode is stopped mode,
    // stopped flag is cleared.
    if ( server_cycle_stopped_ )
    {
        server_cycle_stopped_ = false;
    }

    if ( game_mode_.isServerCycleStoppedMode() )
    {
        server_cycle_stopped_ = true;
    }
}

///////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

*/
CoachAgent::CoachAgent()
    : SoccerAgent()
    , M_impl( new Impl( *this ) )
{
    boost::shared_ptr< AudioMemory > audio_memory( new AudioMemory );

    M_worldmodel.setAudioMemory( audio_memory );
}

/*-------------------------------------------------------------------*/
/*!

*/
CoachAgent::~CoachAgent()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
const
GlobalVisualSensor &
CoachAgent::visualSensor() const
{
    return M_impl->visual_;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
CoachAudioSensor &
CoachAgent::audioSensor() const
{
    return M_impl->audio_;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::initImpl( CmdLineParser & cmd_parser )
{
    bool help = false;
    std::string coach_config_file;

    ParamMap system_param_map( "System options" );
    system_param_map.add()
        ( "help" , "", BoolSwitch( &help ), "print help message.")
        ( "coach-config", "", &coach_config_file, "specifies coach config file." );

    ParamMap coach_param_map( "Coach options" );
    M_config.createParamMap( coach_param_map );

    // analyze command line for system options
    cmd_parser.parse( system_param_map );
    if ( help )
    {
        std::cout << copyright() << std::endl;
        system_param_map.printHelp( std::cout );
        coach_param_map.printHelp( std::cout );
        return false;
    }

    // analyze config file for coach config options
    if ( ! coach_config_file.empty() )
    {
        ConfFileParser conf_parser( coach_config_file.c_str() );
        conf_parser.parse( coach_param_map );
    }

    // analyze command line for coach options
    cmd_parser.parse( coach_param_map );

    if ( config().version() < 1.0
         || 15.0 <= config().version() )
    {
        std::cerr << "Unsupported client version: " << config().version()
                  << std::endl;
        return false;
    }

    if ( config().debug() )
    {
        dlog.setLogFlag( &world().time(), Logger::SYSTEM, config().debugSystem() );
        dlog.setLogFlag( &world().time(), Logger::SENSOR, config().debugSensor() );
        dlog.setLogFlag( &world().time(), Logger::WORLD, config().debugWorld() );
        dlog.setLogFlag( &world().time(), Logger::ACTION, config().debugAction() );
        dlog.setLogFlag( &world().time(), Logger::INTERCEPT, config().debugIntercept() );
        dlog.setLogFlag( &world().time(), Logger::KICK, config().debugKick() );
        dlog.setLogFlag( &world().time(), Logger::HOLD, config().debugHold() );
        dlog.setLogFlag( &world().time(), Logger::DRIBBLE, config().debugDribble() );
        dlog.setLogFlag( &world().time(), Logger::PASS, config().debugPass() );
        dlog.setLogFlag( &world().time(), Logger::CROSS, config().debugCross() );
        dlog.setLogFlag( &world().time(), Logger::SHOOT, config().debugShoot() );
        dlog.setLogFlag( &world().time(), Logger::CLEAR, config().debugClear() );
        dlog.setLogFlag( &world().time(), Logger::BLOCK, config().debugBlock() );
        dlog.setLogFlag( &world().time(), Logger::MARK, config().debugMark() );
        dlog.setLogFlag( &world().time(), Logger::POSITIONING, config().debugPositioning() );
        dlog.setLogFlag( &world().time(), Logger::ROLE, config().debugRole() );
        dlog.setLogFlag( &world().time(), Logger::PLAN, config().debugPlan() );
        dlog.setLogFlag( &world().time(), Logger::TEAM, config().debugTeam() );
        dlog.setLogFlag( &world().time(), Logger::COMMUNICATION, config().debugCommunication() );
        dlog.setLogFlag( &world().time(), Logger::ANALYZER, config().debugAnalyzer() );
        dlog.setLogFlag( &world().time(), Logger::ACTION_CHAIN, config().debugActionChain() );
    }

    if ( config().offlineClientMode() )
    {
        M_client->setClientMode( BasicClient::OFFLINE );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::handleStart()
{
    if ( ! M_client )
    {
        return false;
    }

    if ( config().host().empty() )
    {
        std::cerr << config().teamName()
                  << " coach: ***ERROR*** coach: server host name is empty"
                  << std::endl;
        M_client->setServerAlive( false );
        return false;
    }

    // just create a connection. init command is automaticaly sent
    // by BasicClient's run() method.
    if ( ! M_client->connectTo( config().host().c_str(),
                                config().port(),
                                static_cast< long >( config().intervalMSec() ) ) )
    {
        std::cerr << config().teamName()
                  << " coach: ***ERROR*** Failed to connect." << std::endl;
        M_client->setServerAlive( false );
        return false;
    }

    if ( config().offlineLogging() )
    {
        if ( ! M_impl->openOfflineLog() )
        {
            return false;
        }
    }

    M_impl->sendInitCommand();
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::handleStartOffline()
{
    if ( ! M_client )
    {
        return false;
    }

    if ( ! M_impl->openOfflineLog() )
    {
        return false;
    }

    M_client->setServerAlive( true );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::handleMessage()
{
    if ( ! M_client )
    {
        std::cerr << "CoachAgent::handleMessage(). Client is not registered."
                  << std::endl;
        return;
    }

    int counter = 0;
    GameTime start_time = M_impl->current_time_;

    // receive and analyze message
    while ( M_client->recvMessage() > 0 )
    {
        ++counter;
        parse( M_client->message() );
    }

    if ( M_impl->current_time_.cycle() > start_time.cycle() + 1
         && start_time.stopped() == 0
         && M_impl->current_time_.stopped() == 0 )
    {
        std::cerr << config().teamName()
                  << " coach: parser used several steps -- missed an action!  received"
                  << counter << " messages     start time=" << start_time
                  << " end time=" << M_impl->current_time_
                  << std::endl;
    }


    if ( M_impl->think_received_ )
    {
        action();
    }
#if 0
    else if ( ! ServerParam::i().synchMode() )
    {
        if ( M_impl->last_decision_time_ != M_impl->current_time_
             && M_impl->visual_.time() == M_impl->current_time_ )
        {
            action();
        }
    }
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::handleMessageOffline()
{
    if ( ! M_client )
    {
        std::cerr << "CoachAgent::handleMessageOffline(). Client is not registered."
                  << std::endl;
        return;
    }

    if ( M_client->recvMessage() > 0 )
    {
        parse( M_client->message() );
    }

    if ( M_impl->think_received_ )
    {
        dlog.addText( Logger::SYSTEM,
                      __FILE__": Got think message: decide action" );
#if 0
        std::cout << config().teamName() << " coach: "
                  << world().time()
                  << " action" << std::endl;
#endif
        action();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::handleTimeout( const int /*timeout_count*/,
                           const int waited_msec )
{
    if ( ! M_client )
    {
        std::cerr << "CoachAgent::handleTimeout(). Client is not registered."
                  << std::endl;
        return;
    }

    if ( waited_msec > config().serverWaitSeconds() * 1000 )
    {
        if ( config().useEye() )
        {
            std::cout << config().teamName()
                      << " coach: waited "
                      << waited_msec / 1000
                      << " seconds. server down??" << std::endl;
            M_client->setServerAlive( false );
            return;
        }

        if ( waited_msec > config().serverWaitSeconds() * 2 * 1000 )
        {
            std::cout << config().teamName()
                      << " coach: waited "
                      << waited_msec / 1000
                      << " seconds. server down??" << std::endl;
            M_client->setServerAlive( false );
            return;
        }

        //std::cout << config().teamName() << ": coach requests server response..."
        //<< std::endl;
        doCheckBall();
    }


    if ( M_impl->last_decision_time_ != M_impl->current_time_ )
    {
        if ( M_impl->visual_.time() == M_impl->current_time_
             || waited_msec >= 20 * ServerParam::i().slowDownFactor() )
        {
            dlog.addText( Logger::SYSTEM,
                          "----- TIMEOUT DECISION !! [%d]ms from last sensory",
                          waited_msec );
            action();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::handleExit()
{
    finalize();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::addSayMessageParser( boost::shared_ptr< SayMessageParser > parser )
{
    M_impl->audio_.addParser( parser );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::removeSayMessageParser( const char header )
{
    M_impl->audio_.removeParser( header );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::finalize()
{
    if ( M_client->isServerAlive() )
    {
        M_impl->sendByeCommand();
    }
    std::cout << config().teamName() << " coach: finished."
              << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CoachAgent::Impl::openOfflineLog()
{
    std::string filepath = agent_.config().logDir();

    if ( ! filepath.empty() )
    {
        if ( *filepath.rbegin() != '/' )
        {
            filepath += '/';
        }
    }

    filepath += agent_.config().teamName();
    filepath += "-coach";
    filepath += agent_.config().offlineLogExt();

    if ( ! agent_.M_client->openOfflineLog( filepath ) )
    {
        std::cerr << "Failed to open the offline client log file ["
                  << filepath
                  << "]" << std::endl;
        agent_.M_client->setServerAlive( false );
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CoachAgent::Impl::openDebugLog()
{
    std::string filepath = agent_.config().logDir();

    if ( ! filepath.empty() )
    {
        if ( *filepath.rbegin() != '/' )
        {
            filepath += '/';
        }
    }

    filepath += agent_.config().teamName();
    filepath += "-coach";
    filepath += agent_.config().debugLogExt();

    dlog.open( filepath );

    if ( ! dlog.isOpen() )
    {
        std::cerr << agent_.config().teamName() << " coach: "
                  << " Failed to open the debug log file [" << filepath << "]"
                  << std::endl;
        agent_.M_client->setServerAlive( false );
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::action()
{
    if ( config().offlineLogging()
         && ! ServerParam::i().synchMode() )
    {
        M_client->printOfflineThink();
    }

    if ( M_impl->last_decision_time_ != M_impl->current_time_ )
    {
        actionImpl();
        M_impl->last_decision_time_ = M_impl->current_time_;
    }

    if ( M_impl->think_received_ )
    {
        CoachDoneCommand com;
        sendCommand( com );
        M_impl->think_received_ = false;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::parse( const char * msg )
{
    if ( ! std::strncmp( msg, "(see_global ", 12 ) )
    {
        M_impl->analyzeSeeGlobal( msg );
    }
#if 0
    else if ( ! std::strncmp( msg, "(see ", 5 ) )
    {
        M_impl->analyzeSeeGlobal( msg );
    }
#endif
    else if ( ! std::strncmp( msg, "(hear ", 6 ) )
    {
        M_impl->analyzeHear( msg );
    }
    else if ( ! std::strncmp( msg, "(think)", 7 ) )
    {
        M_impl->think_received_ = true;
    }
    else if ( ! std::strncmp( msg, "(change_player_type ", 20 ) )
    {
        M_impl->analyzeChangePlayerType( msg );
    }
    else if ( ! std::strncmp( msg, "(clang ", 7 ) )
    {
        M_impl->analyzeCLangVer( msg );
    }
    else if ( ! std::strncmp( msg, "(player_type ", 13 ) )  // hetero param
    {
        M_impl->analyzePlayerType( msg );
    }
    else if ( ! std::strncmp( msg, "(player_param ", 14 ) ) // tradeoff param
    {
        M_impl->analyzePlayerParam( msg );
    }
    else if ( ! std::strncmp( msg, "(server_param ", 14 ) )
    {
        M_impl->analyzeServerParam( msg );
    }
    else if ( ! std::strncmp( msg, "(ok ", 4 ) )
    {
        M_impl->analyzeOK( msg );
    }
    else if ( ! std::strncmp( msg, "(error ", 7 ) )
    {
        M_impl->analyzeError( msg );
    }
    else if ( ! std::strncmp( msg, "(warning ", 9 ) )
    {
        M_impl->analyzeWarning( msg );
    }
    else if ( ! std::strncmp( msg, "(score ", 7 ) )
    {
        M_impl->analyzeScore( msg );
    }
    else if ( ! std::strncmp( msg, "(init ", 6 ) )
    {
        M_impl->analyzeInit( msg );
    }
    else if ( ! std::strncmp( msg, "(include ", 9 ) )
    {
        M_impl->analyzeInclude( msg );
    }
    else
    {
        std::cerr << config().teamName()
                  << " coach: "
                  << world().time()
                  << " received unsupported Message : [" << msg << "]"
                  << std::endl;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeInit( const char * msg )
{
    // "(init l ok)" | "(init r ok)"

    const CoachConfig & c = agent_.config();

    char side;

    if ( std::sscanf( msg, "(init %c ok)", &side ) != 1 )
    {
        agent_.M_client->setServerAlive( false );
        return;
    }

    if ( side != 'l' && side != 'r' )
    {
        std::cerr << c.teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " received unexpected init message. " << msg
                  << std::endl;
        agent_.M_client->setServerAlive( false );
        return;
    }

    // inititalize member variables
    SideID side_id = ( side == 'l'
                       ? LEFT
                       : RIGHT );
    agent_.M_worldmodel.init( side_id, c.version() );

    if ( c.debug() )
    {
        openDebugLog();
    }

    // send specific settings
    // if we intend to advise to the team,
    // set visual sensory
    if ( c.useEye() )
    {
        agent_.doEye( true );
    }

    if ( c.hearSay() )
    {
        audio_.setTeamName( c.teamName() );
    }

    // set compression level
    if ( 0 < c.compression()
         && c.compression() <= 9 )
    {
        CoachCompressionCommand com( c.compression() );
        agent_.sendCommand( com );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::Impl::analyzeCycle( const char * msg,
                          const bool by_see_global )
{
    char id[16];
    long cycle = 0;
    if ( std::sscanf( msg, "(%15s %ld ",
                      id, &cycle ) != 2 )
    {
        std::cerr << agent_.config().teamName()
                  << " coach:"
                  << agent_.world().time()
                  << " ***ERROR*** failed to parse time msg=["
                  << msg << "]"
                  << std::endl;
        return false;
    }

    updateCurrentTime( cycle, by_see_global );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeSeeGlobal( const char * msg )
{
    if ( ! analyzeCycle( msg, true ) )
    {
        return;
    }

    // analyze message
    visual_.parse( msg,
                   agent_.config().version(),
                   current_time_ );

    // update world model
    if ( visual_.time() == current_time_ )
    {
        agent_.M_worldmodel.updateAfterSeeGlobal( visual_,
                                                  current_time_ );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeHear( const char * msg )
{
    if ( ! analyzeCycle( msg, false ) )
    {
        return;
    }

    long cycle;
    char sender[128];

    if ( std::sscanf( msg, "(hear %ld %127s ",
                      &cycle, sender ) != 2 )
    {
        std::cerr << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " Error. failed to parse audio sender. ["
                  << msg
                  << std::endl;
        return;
    }

    // check sender name
    if ( ! std::strcmp( sender, "referee" ) )
    {
        analyzeHearReferee( msg );
    }
    else if ( ! std::strncmp( sender, "(p", 2 ) )
    {
        // (hear <time> (player "<teamname>" <unum>) "<message>")
        // (hear <time> (p "<teamname>" <unum>) "<message>")
        analyzeHearPlayer( msg );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeHearReferee( const char * msg )
{
    const CoachConfig & c = agent_.config();
    const GlobalWorldModel & wm = agent_.world();

    long cycle;
    char mode[512];

    if ( std::sscanf( msg, "(hear %ld referee %511[^)]", &cycle, mode ) != 2 )
    {
        std::cerr << c.teamName()
                  << " coach: "
                  << wm.time()
                  << " ***ERROR*** Failed to scan playmode." << msg
                  << std::endl;
        return;
    }

    if ( ! game_mode_.update( mode, current_time_ ) )
    {
        if ( ! std::strncmp( mode, "yellow_card", std::strlen( "yellow_card" ) ) )
        {
            char side = '?';
            int unum = Unum_Unknown;
            if ( std::sscanf( mode, "yellow_card_%c_%d", &side, &unum ) != 2 )
            {
                std::cerr << c.teamName()
                          << " coach: "
                          << wm.time()
                          << " could not parse the yellow card message [" << msg << ']'
                          << std::endl;
            }

            agent_.M_worldmodel.setYellowCard( ( side == 'l'
                                                 ? LEFT
                                                 : side == 'r'
                                                 ? RIGHT
                                                 : NEUTRAL ),
                                               unum );
        }
        else if ( ! std::strncmp( mode, "red_card", std::strlen( "red_card" ) ) )
        {
            char side = '?';
            int unum = Unum_Unknown;
            if ( std::sscanf( mode, "red_card_%c_%d", &side, &unum ) != 2 )
            {
                std::cerr << c.teamName()
                          << " coach: "
                          << wm.time()
                          << " could not parse the red card message [" << msg << ']'
                          << std::endl;
            }

            agent_.M_worldmodel.setRedCard( ( side == 'l'
                                              ? LEFT
                                              : side == 'r'
                                              ? RIGHT
                                              : NEUTRAL ),
                                            unum );
        }
        else if ( ! std::strncmp( mode, "training", std::strlen( "training" ) ) )
        {
            // end keepaway (or some training) episode
            agent_.M_worldmodel.setTrainingTime( current_time_ );
            return;
        }
        else
        {
            std::cerr << c.teamName() << " coach: "
                      << wm.time()
                      << " Unknown playmode string." << mode
                      << std::endl;
        }

        return;
    }

    updateServerStatus();

    if ( game_mode_.isGameEndMode() )
    {
        sendByeCommand();
        return;
    }

    agent_.M_worldmodel.updateGameMode( game_mode_, current_time_ );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeHearPlayer( const char * msg )
{
    // (hear <time> (player "<teamname>" <unum>) "<message>")
    // (hear <time> (p "<teamname>" <unum>) "<message>")

    if ( agent_.config().hearSay() )
    {
        audio_.parsePlayerMessage( msg, current_time_ );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeChangePlayerType( const char * msg )
{
    // teammate: "(change_player_type <unum> <type>)\n"
    //           "(ok change_player_type <unum> <type>)\n"
    // opponent: "(change_player_type <unum>)\n"

    int unum = -1, type = -1;

    if ( std::sscanf( msg, " ( ok change_player_type %d %d ) ",
                      &unum, &type ) == 2 )
    {
        // do nothing
    }
    else if ( std::sscanf( msg, " ( change_player_type %d %d ) ",
                           &unum, &type ) == 2 )
    {
        // teammate
        agent_.M_worldmodel.setPlayerType( agent_.world().ourSide(),
                                           unum,
                                           type );
    }
    else if ( std::sscanf( msg, " ( change_player_type %d ) ",
                           &unum ) == 1 )
    {
        // opponent
        agent_.M_worldmodel.setPlayerType( agent_.world().theirSide(),
                                           unum,
                                           Hetero_Unknown );
    }
    else
    {
        std::cerr << " ***ERROR*** parse error. "
                  << msg
                  << std::endl;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzePlayerType( const char * msg )
{
    PlayerType player_type( msg, agent_.config().version() );
    PlayerTypeSet::instance().insert( player_type );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzePlayerParam( const char * msg )
{
    PlayerParam::instance().parse( msg, agent_.config().version() );
    //PlayerParam::i().print( std::cout );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeServerParam( const char * msg )
{
    ServerParam::instance().parse( msg, agent_.config().version() );
    PlayerTypeSet::instance().resetDefaultType();

    agent_.M_worldmodel.initFreeformCount();

    if ( ! ServerParam::i().synchMode()
         && ServerParam::i().slowDownFactor() > 1 )
    {
        long interval = ( agent_.config().intervalMSec()
                          * ServerParam::i().slowDownFactor() );
        agent_.M_client->setIntervalMSec( interval );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeCLangVer( const char * )
{
    //     std::cerr << config().teamName()
    //               << " coach: "
    //               << world().time()
    //               << " recv: " << msg << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeOK( const char * msg )
{
    //std::cout << M_impl->current_time_
    //<< " Coach received ok " << std::endl;

    if ( ! std::strncmp( msg, "(ok say)",
                         std::strlen( "(ok say)" ) ) )
    {
        // nothing to do
    }
    else if ( ! std::strncmp( msg, "(ok team_graphic ",
                              std::strlen( "(ok team_graphic " ) ) )
    {
        analyzeOKTeamGraphic( msg );
    }
    else if ( ! std::strncmp( msg, "(ok look ",
                              std::strlen( "(ok look " ) ) )
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << "recv (ok look ..." << std::endl;
    }
    else if ( ! std::strncmp( msg, "(ok check_ball ",
                              std::strlen( "(ok check_ball " ) ) )
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " recv (ok check_ball ..." << std::endl;;
    }
    else if ( ! std::strncmp( msg, "(ok change_player_type ",
                              std::strlen( "(ok change_player_type " ) ) )
    {
        analyzeChangePlayerType( msg );
    }
    else if ( ! std::strncmp( msg, "(ok compression ",
                              std::strlen( "(ok compression " ) ) )
    {
        int level = 0;
        if ( std::sscanf( msg, "(ok compression %d)", &level ) == 1 )
        {
            std::cout << agent_.config().teamName()
                      << " coach: "
                      << agent_.world().time()
                      << " set compression level " << level<< std::endl;
            agent_.M_client->setCompressionLevel( level );
        }
    }
    else if ( ! std::strncmp( msg, "(ok eye ", std::strlen( "(ok eye " ) ) )
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " recv " << msg << std::endl;
    }
    else if ( ! std::strncmp( msg, "(ok team_names ",
                              std::strlen( "(ok team_names " ) ) )
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " recv " << msg << std::endl;
        analyzeTeamNames( msg );
    }
    else
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " recv " << msg << std::endl;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeOKTeamGraphic( const char * msg )
{
    // "(ok team_graphic <x> <y>)"
    // "(ok team_graphic <x> <y>)"

    int x = -1, y = -1;

    if ( std::sscanf( msg,
                      "(ok team_graphic %d %d)",
                      &x, &y ) != 2
         || x < 0
         || y < 0 )
    {
        std::cout << agent_.config().teamName()
                  << " coach: "
                  << agent_.world().time()
                  << " recv illegal message. " << msg
                  << std::endl;
        return;
    }

    agent_.M_team_graphic_ok_set.insert( TeamGraphic::Index( x, y ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeTeamNames( const char * msg )
{
    // "(ok team_names (team l <name>)[ (team r <name>)])"

    char left[32], right[32];

    int n = std::sscanf( msg,
                         "(ok team_names (team l %31[^)]) (team r %31[^)]))",
                         left, right );
    if ( n == 2 )
    {
        agent_.M_worldmodel.setTeamName( LEFT, left );
        agent_.M_worldmodel.setTeamName( RIGHT, right );
    }
    else if ( n == 1 )
    {
        agent_.M_worldmodel.setTeamName( LEFT, left );
    }
}


/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeScore( const char * msg )
{
    std::cerr << agent_.config().teamName()
              << " coach: "
              << agent_.world().time()
              << " recv " << msg << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeError( const char * msg )
{
    std::cerr << agent_.config().teamName()
              << " coach: "
              << agent_.world().time()
              << " recv " << msg << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeWarning( const char * msg )
{
    std::cerr << agent_.config().teamName()
              << " coach: "
              << agent_.world().time()
              << " recv " << msg
              << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::analyzeInclude( const char * msg )
{
    std::cerr << agent_.config().teamName()
              << " coach: "
              << agent_.world().time()
              << " recv " << msg << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::sendCommand( const CoachCommand & com )
{
    std::ostringstream os;
    com.toStr( os );

    std::string str = os.str();
    if ( str.length() == 0 )
    {
        return false;
    }

    return ( M_client->sendMessage( str.c_str() ) > 0 );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::sendInitCommand()
{
    if ( ! agent_.M_client->isServerAlive() )
    {
        std::cerr << agent_.config().teamName()
                  << " coach: server is not alive" << std::endl;
        return;
    }

    // make command string
    bool success = true;
    if ( agent_.config().useCoachName()
         && ! agent_.config().coachName().empty() )
    {
        CoachInitCommand com( agent_.config().teamName(),
                              agent_.config().version(),
                              agent_.config().coachName() );
        success = agent_.sendCommand( com );
    }
    else
    {
        CoachInitCommand com( agent_.config().teamName(),
                              agent_.config().version() );
        success = agent_.sendCommand( com );
    }

    if ( ! success )
    {
        std::cerr << agent_.config().teamName()
                  << " coach: Failed to init coach...\nExit ..."
                  << std::endl;
        agent_.M_client->setServerAlive( false );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachAgent::Impl::sendByeCommand()
{
    CoachByeCommand com;
    agent_.sendCommand( com );
    agent_.M_client->setServerAlive( false );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doCheckBall()
{
    CoachCheckBallCommand com;
    return sendCommand( com );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doLook()
{
    CoachLookCommand com;
    return sendCommand( com );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doTeamNames()
{
    CoachTeamNamesCommand com;
    return sendCommand( com );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doEye( bool on )
{
    CoachEyeCommand com( on );
    return sendCommand( com );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doChangePlayerType( const int unum,
                                const int type )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << config().teamName()
                  << "coach: "
                  << world().time()
                  << " doChangePlayerType. Illegal player number = "
                  << unum
                  << std::endl;
        return false;
    }

    if ( type < Hetero_Default
         || PlayerParam::i().playerTypes() <= type )
    {
        std::cerr << config().teamName()
                  << " coach: "
                  << world().time()
                  << "doChangePlayerType. Illegal player type = "
                  << type
                  << std::endl;
        return false;
    }

    CoachChangePlayerTypeCommand com( unum, type );
    return sendCommand( com );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doChangePlayerTypes( const std::vector< std::pair< int, int > > & types )
{
    if ( types.empty() )
    {
        return false;
    }

    //CoachChangePlayerTypesCommand com( types );
    //return sendCommand( com );

    bool result = true;
    for ( std::vector< std::pair< int, int > >::const_iterator it = types.begin();
          it != types.end();
          ++it )
    {
        result = doChangePlayerType( it->first, it->second );
    }

    return result;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doSayFreeform( const std::string & msg )
{
    if ( msg.empty()
         || static_cast< int >( msg.length() ) > ServerParam::i().coachSayMsgSize() )
    {
        // over the message size limitation
        std::cerr << config().teamName()
                  << " coach: "
                  << world().time()
                  << " ***WARNING** invalid free form message length = "
                  << msg.length()
                  << std::endl;
        return false;
    }

    if ( config().version() < 7.0 )
    {
        if ( world().gameMode().type() == GameMode::PlayOn )
        {
            // coach cannot send freeform while playon
            std::cerr << config().teamName()
                      << " coach: "
                      << world().time()
                      << " ***WARNING*** cannot send message while playon. "
                      << std::endl;
            return false;
        }

        // raw message string is sent.
        // (say <message>)

        M_worldmodel.incFreeformSendCount();

        CoachSayCommand com( msg );
        return sendCommand( com );
    }

    // check if coach is allowd to send the freeform message or not.
    if ( ! world().canSendFreeform() )
    {
        std::cerr << config().teamName()
                  << " coach: "
                  << world().time()
                  << " ***WARNING*** cannot send freeform now. "
                  << std::endl;
        return false;
    }

    // send clang format message
    // (say (freeform "<message>"))

    M_worldmodel.incFreeformSendCount();

    //std::ostringstream ostr;
    //ostr << "(say (freeform \"" << msg << "\"))";
    //return ( M_client->sendMessage( ostr.str().c_str() ) > 0 );

    std::string freeform_msg;
    freeform_msg.reserve( msg.length() + 32 );

    freeform_msg = "(say (freeform \"";
    freeform_msg += msg;
    freeform_msg += "\"))";

    return ( M_client->sendMessage( freeform_msg.c_str() ) > 0 );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachAgent::doTeamGraphic( const int x,
                           const int y,
                           const TeamGraphic & team_graphic )
{
    static int send_count = 0;
    static GameTime send_time( -1, 0 );

    if ( send_time != M_impl->current_time_ )
    {
        send_count = 0;
    }

    send_time = M_impl->current_time_;
    ++send_count;

    if ( send_count > config().maxTeamGraphicPerCycle() ) // XXX Magic Number
    {
        return false;
    }

    TeamGraphic::Index index( x, y );
    TeamGraphic::Map::const_iterator tile = team_graphic.tiles().find( index );

    if ( tile == team_graphic.tiles().end() )
    {
        std::cerr << config().teamName()
                  << " coach: "
                  << world().time()
                  << " ***WARNING*** The xpm tile ("
                  << x << ',' << y << ") does not found in the team graphic."
                  << std::endl;
        return false;
    }

    std::ostringstream ostr;

    ostr << "(team_graphic (" << x << ' ' << y << ' ';
    tile->second->print( ostr );
    ostr << "))";

    return ( M_client->sendMessage( ostr.str().c_str() ) > 0 );
}

}
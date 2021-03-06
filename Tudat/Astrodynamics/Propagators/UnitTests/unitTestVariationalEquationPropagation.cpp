/*    Copyright (c) 2010-2017, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#define BOOST_TEST_MAIN

#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "Tudat/Basics/testMacros.h"
#include "Tudat/Mathematics/BasicMathematics/linearAlgebra.h"
#include "Tudat/Astrodynamics/BasicAstrodynamics/physicalConstants.h"

#include "Tudat/External/SpiceInterface/spiceInterface.h"
#include "Tudat/Mathematics/NumericalIntegrators/rungeKuttaCoefficients.h"
#include "Tudat/Astrodynamics/BasicAstrodynamics/accelerationModel.h"
#include "Tudat/InputOutput/basicInputOutput.h"

#include "Tudat/SimulationSetup/EnvironmentSetup/body.h"
#include "Tudat/SimulationSetup/PropagationSetup/variationalEquationsSolver.h"
#include "Tudat/SimulationSetup/EnvironmentSetup/defaultBodies.h"
#include "Tudat/SimulationSetup/EnvironmentSetup/createBodies.h"
#include "Tudat/SimulationSetup/PropagationSetup/createNumericalSimulator.h"
#include "Tudat/SimulationSetup/EstimationSetup/createEstimatableParameters.h"

namespace tudat
{

namespace unit_tests
{

//Using declarations.
using namespace tudat::estimatable_parameters;
using namespace tudat::orbit_determination;
using namespace tudat::interpolators;
using namespace tudat::numerical_integrators;
using namespace tudat::spice_interface;
using namespace tudat::simulation_setup;
using namespace tudat::basic_astrodynamics;
using namespace tudat::orbital_element_conversions;
using namespace tudat::ephemerides;
using namespace tudat::propagators;

BOOST_AUTO_TEST_SUITE( test_variational_equation_calculation )

template< typename TimeType = double , typename StateScalarType  = double >
        std::pair< std::vector< Eigen::Matrix< StateScalarType, Eigen::Dynamic, Eigen::Dynamic > >,
std::vector< Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > > >
executeEarthMoonSimulation(
        const std::vector< std::string > centralBodies,
        const Eigen::Matrix< StateScalarType, 12, 1 > initialStateDifference =
        Eigen::Matrix< StateScalarType, 12, 1 >::Zero( ),
        const int propagationType = 0,
        const Eigen::Vector3d parameterPerturbation = Eigen::Vector3d::Zero( ),
        const bool propagateVariationalEquations = 1 )
{

    //Load spice kernels.
    std::string kernelsPath = input_output::getSpiceKernelPath( );
    spice_interface::loadSpiceKernelInTudat( kernelsPath + "de-403-masses.tpc");
    spice_interface::loadSpiceKernelInTudat( kernelsPath + "naif0009.tls");
    spice_interface::loadSpiceKernelInTudat( kernelsPath + "pck00009.tpc");
    spice_interface::loadSpiceKernelInTudat( kernelsPath + "de421.bsp");

    // Define
    std::vector< std::string > bodyNames;
    bodyNames.push_back( "Earth" );
    bodyNames.push_back( "Sun" );
    bodyNames.push_back( "Moon" );
    bodyNames.push_back( "Mars" );

    // Specify initial time
    TimeType initialEphemerisTime = TimeType( 1.0E7 );
    TimeType finalEphemerisTime = initialEphemerisTime + 0.5E7;
    double maximumTimeStep = 3600.0;

    double buffer = 10.0 * maximumTimeStep;

    // Create bodies needed in simulation
    std::map< std::string, boost::shared_ptr< BodySettings > > bodySettings =
            getDefaultBodySettings( bodyNames, initialEphemerisTime - buffer, finalEphemerisTime + buffer );

    NamedBodyMap bodyMap = createBodies( bodySettings );
    setGlobalFrameBodyEphemerides( bodyMap, "SSB", "ECLIPJ2000" );


    // Set accelerations between bodies that are to be taken into account.
    SelectedAccelerationMap accelerationMap;
    std::map< std::string, std::vector< boost::shared_ptr< AccelerationSettings > > > accelerationsOfEarth;
    accelerationsOfEarth[ "Sun" ].push_back( boost::make_shared< AccelerationSettings >( central_gravity ) );
    accelerationsOfEarth[ "Moon" ].push_back( boost::make_shared< AccelerationSettings >( central_gravity ) );
    accelerationMap[ "Earth" ] = accelerationsOfEarth;

    std::map< std::string, std::vector< boost::shared_ptr< AccelerationSettings > > > accelerationsOfMoon;
    accelerationsOfMoon[ "Sun" ].push_back( boost::make_shared< AccelerationSettings >( central_gravity ) );
    accelerationsOfMoon[ "Earth" ].push_back( boost::make_shared< AccelerationSettings >( central_gravity ) );
    accelerationMap[ "Moon" ] = accelerationsOfMoon;

    // Set bodies for which initial state is to be estimated and integrated.
    std::vector< std::string > bodiesToIntegrate;
    bodiesToIntegrate.push_back( "Moon" );
    bodiesToIntegrate.push_back( "Earth" );

    unsigned int numberOfNumericalBodies = bodiesToIntegrate.size( );

    // Define propagator settings.
    std::map< std::string, std::string > centralBodyMap;

    for( unsigned int i = 0; i < numberOfNumericalBodies; i++ )
    {
        centralBodyMap[ bodiesToIntegrate[ i ] ] = centralBodies[ i ];
    }

    // Create acceleration models
    AccelerationMap accelerationModelMap = createAccelerationModelsMap(
                bodyMap, accelerationMap, centralBodyMap );

    // Create integrator settings
    boost::shared_ptr< IntegratorSettings< TimeType > > integratorSettings =
            boost::make_shared< IntegratorSettings< TimeType > >
            ( rungeKutta4, TimeType( initialEphemerisTime ), 1800.0 );


    // Set initial states of bodies to integrate.
    TimeType initialIntegrationTime = initialEphemerisTime;

    // Set (perturbed) initial state.
    Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > systemInitialState;
    systemInitialState = getInitialStatesOfBodies< TimeType, StateScalarType >(
                bodiesToIntegrate, centralBodies, bodyMap, initialIntegrationTime );
    systemInitialState += initialStateDifference;

    // Create propagator settings
    boost::shared_ptr< TranslationalStatePropagatorSettings< StateScalarType > > propagatorSettings;
    TranslationalPropagatorType propagatorType;
    if( propagationType == 0 )
    {
        propagatorType = cowell;
    }
    else if( propagationType == 1 )
    {
        propagatorType = encke;
    }
    propagatorSettings =  boost::make_shared< TranslationalStatePropagatorSettings< StateScalarType > >
            ( centralBodies, accelerationModelMap, bodiesToIntegrate, systemInitialState,
              TimeType( finalEphemerisTime ), propagatorType );

    // Define parameters.
    std::vector< boost::shared_ptr< EstimatableParameterSettings > > parameterNames;
    {
        parameterNames.push_back(
                    boost::make_shared< InitialTranslationalStateEstimatableParameterSettings< StateScalarType > >(
                        "Moon", propagators::getInitialStateOfBody< TimeType, StateScalarType >(
                            "Moon", centralBodies[ 0 ], bodyMap, TimeType( initialEphemerisTime ) ) +
                    initialStateDifference.segment( 0, 6 ),
                    centralBodies[ 0 ] ) );
        parameterNames.push_back(
                    boost::make_shared< InitialTranslationalStateEstimatableParameterSettings< StateScalarType > >(
                        "Earth", propagators::getInitialStateOfBody< TimeType, StateScalarType >(
                            "Earth", centralBodies[ 1 ], bodyMap, TimeType( initialEphemerisTime ) ) +
                    initialStateDifference.segment( 6, 6 ),
                    centralBodies[ 1 ] ) );
        parameterNames.push_back( boost::make_shared< EstimatableParameterSettings >( "Moon", gravitational_parameter ) );
        parameterNames.push_back( boost::make_shared< EstimatableParameterSettings >( "Earth", gravitational_parameter ) );
        parameterNames.push_back( boost::make_shared< EstimatableParameterSettings >( "Sun", gravitational_parameter ) );

    }

    // Create parameters
    boost::shared_ptr< estimatable_parameters::EstimatableParameterSet< StateScalarType > > parametersToEstimate =
            createParametersToEstimate( parameterNames, bodyMap );

    // Perturb parameters.
    Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > parameterVector =
            parametersToEstimate->template getFullParameterValues< StateScalarType >( );
    parameterVector.block( 12, 0, 3, 1 ) += parameterPerturbation;
    parametersToEstimate->resetParameterValues( parameterVector );

    std::pair< std::vector< Eigen::Matrix< StateScalarType, Eigen::Dynamic, Eigen::Dynamic > >,
            std::vector< Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > > > results;

    {
        // Create dynamics simulator
        SingleArcVariationalEquationsSolver< StateScalarType, TimeType, double > dynamicsSimulator =
                SingleArcVariationalEquationsSolver< StateScalarType, TimeType, double >(
                    bodyMap, integratorSettings, propagatorSettings, parametersToEstimate,
                    1, boost::shared_ptr< numerical_integrators::IntegratorSettings< double > >( ), 1, 0 );

        // Propagate requested equations.
        if( propagateVariationalEquations )
        {
            dynamicsSimulator.integrateVariationalAndDynamicalEquations( propagatorSettings->getInitialStates( ), 1 );
        }
        else
        {
            dynamicsSimulator.integrateDynamicalEquationsOfMotionOnly( propagatorSettings->getInitialStates( ) );
        }

        // Retrieve test data
        double testEpoch = 1.4E7;
        Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > testStates =
                Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 >::Zero( 12 );
        testStates.block( 0, 0, 6, 1 ) = bodyMap[ "Moon" ]->getStateInBaseFrameFromEphemeris( testEpoch );
        if( centralBodyMap[ "Moon" ] == "Earth" )
        {
            testStates.block( 0, 0, 6, 1 ) -= bodyMap[ "Earth" ]->getStateInBaseFrameFromEphemeris( testEpoch );
        }

        testStates.block( 6, 0, 6, 1 ) = bodyMap[ "Earth" ]->getStateInBaseFrameFromEphemeris( testEpoch );

        if( propagateVariationalEquations )
        {
            results.first.push_back( dynamicsSimulator.getStateTransitionMatrixInterface( )->
                                     getCombinedStateTransitionAndSensitivityMatrix( testEpoch ) );
        }
        results.second.push_back( testStates );
    }
    return results;
}

//! Test the state transition and sensitivity matrix computation against their numerical propagation.
/*!
 *  Test the state transition and sensitivity matrix computation against their numerical propagation. This unit test
 *  propagates the variational equations for the Earth and Moon, using both a barycentric origin and hierarchical origin.
 *  For the hierarchical origin, both an Encke and Cowell propagator are used. The results are compared against
 *  results obtained from numerical differentiation (first-order central difference). *
 */
BOOST_AUTO_TEST_CASE( testEarthMoonVariationalEquationCalculation )
{
    std::pair< std::vector< Eigen::MatrixXd >, std::vector< Eigen::VectorXd > > currentOutput;

    std::vector< std::vector< std::string > > centralBodiesSet;
    std::vector< std::string > centralBodies;

    // Define central bodt settings
    centralBodies.resize( 2 );

    centralBodies[ 0 ] = "SSB";
    centralBodies[ 1 ] = "SSB";
    centralBodiesSet.push_back( centralBodies );

    centralBodies[ 0 ] = "Earth";
    centralBodies[ 1 ] = "Sun";
    centralBodiesSet.push_back( centralBodies );


    // Define variables for numerical differentiation
    Eigen::Matrix< double, 12, 1>  perturbedState;
    Eigen::Vector3d perturbedParameter;

    Eigen::Matrix< double, 12, 1> statePerturbation;
    Eigen::Vector3d parameterPerturbation;


    for( unsigned int i = 0; i < centralBodiesSet.size( ); i++ )
    {
        // Define parameter perturbation
        parameterPerturbation  = ( Eigen::Vector3d( ) << 1.0E10, 1.0E10, 1.0E14 ).finished( );

        // Define state perturbation
        if( i == 0 )
        {
            statePerturbation = ( Eigen::Matrix< double, 12, 1>( )<<
                                  100000.0, 100000.0, 100000.0, 0.1, 0.1, 0.1,
                                  100000.0, 100000.0, 100000.0, 0.1, 0.1, 0.1 ).finished( );
        }
        else if( i == 1 )
        {
            statePerturbation = ( Eigen::Matrix< double, 12, 1>( )<<
                                  100000.0, 100000.0, 100000.0, 0.1, 0.1, 0.1,
                                  100000.0, 100000.0, 10000000.0, 0.1, 0.1, 10.0 ).finished( );
        }

        // Only perform Encke propagation is origin is not SSB
        unsigned int maximumPropagatorType = 1;
        if( i == 1 )
        {
            maximumPropagatorType = 2;
        }

        // Test for all requested propagator types.
        for( unsigned int k = 0; k < maximumPropagatorType; k++ )
        {
            // Compute state transition and sensitivity matrices
            currentOutput = executeEarthMoonSimulation< double, double >(
                        centralBodiesSet[ i ], Eigen::Matrix< double, 12, 1 >::Zero( ), k );
            Eigen::MatrixXd stateTransitionAndSensitivityMatrixAtEpoch = currentOutput.first.at( 0 );
            Eigen::MatrixXd manualPartial = Eigen::MatrixXd::Zero( 12, 15 );

            // Numerically compute state transition matrix
            for( unsigned int j = 0; j < 12; j++ )
            {
                Eigen::VectorXd upPerturbedState, downPerturbedState;
                perturbedState.setZero( );
                perturbedState( j ) += statePerturbation( j );
                upPerturbedState = executeEarthMoonSimulation< double, double >(
                            centralBodiesSet[ i ], perturbedState, k, Eigen::Vector3d::Zero( ), 0 ).second.at( 0 );

                perturbedState.setZero( );
                perturbedState( j ) -= statePerturbation( j );
                downPerturbedState = executeEarthMoonSimulation< double, double >(
                            centralBodiesSet[ i ], perturbedState, k, Eigen::Vector3d::Zero( ), 0 ).second.at( 0 );
                manualPartial.block( 0, j, 12, 1 ) =
                        ( upPerturbedState - downPerturbedState ) / ( 2.0 * statePerturbation( j ) );
            }

            // Numerically compute sensitivity matrix
            for( unsigned int j = 0; j < 3; j ++ )
            {
                Eigen::VectorXd upPerturbedState, downPerturbedState;
                perturbedState.setZero( );
                Eigen::Vector3d upPerturbedParameter, downPerturbedParameter;
                perturbedParameter.setZero( );
                perturbedParameter( j ) += parameterPerturbation( j );
                upPerturbedState = executeEarthMoonSimulation< double, double >(
                            centralBodiesSet[ i ], perturbedState, k, perturbedParameter, 0 ).second.at( 0 );

                perturbedParameter.setZero( );
                perturbedParameter( j ) -= parameterPerturbation( j );
                downPerturbedState = executeEarthMoonSimulation< double, double >(
                            centralBodiesSet[ i ], perturbedState, k, perturbedParameter, 0 ).second.at( 0 );
                manualPartial.block( 0, j + 12, 12, 1 ) =
                        ( upPerturbedState - downPerturbedState ) / ( 2.0 * parameterPerturbation( j ) );
            }

            // Check results
            TUDAT_CHECK_MATRIX_CLOSE_FRACTION(
                        stateTransitionAndSensitivityMatrixAtEpoch.block( 0, 0, 12, 15 ), manualPartial, 2.0E-4 );

        }

    }
}

BOOST_AUTO_TEST_SUITE_END( )

}

}


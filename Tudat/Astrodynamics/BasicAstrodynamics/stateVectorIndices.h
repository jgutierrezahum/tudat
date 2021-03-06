/*    Copyright (c) 2010-2017, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 */

#ifndef TUDAT_STATE_INDICES_H
#define TUDAT_STATE_INDICES_H

namespace tudat
{
namespace orbital_element_conversions
{

//! Cartesian elements indices.
enum CartesianElementIndices
{
    xCartesianPositionIndex = 0,
    yCartesianPositionIndex = 1,
    zCartesianPositionIndex = 2,
    xCartesianVelocityIndex = 3,
    yCartesianVelocityIndex = 4,
    zCartesianVelocityIndex = 5
};

//! Keplerian elements indices.
enum KeplerianElementIndices
{
    semiMajorAxisIndex = 0,
    eccentricityIndex = 1,
    inclinationIndex = 2,
    argumentOfPeriapsisIndex = 3,
    longitudeOfAscendingNodeIndex = 4,
    trueAnomalyIndex = 5,
    semiLatusRectumIndex = 0
};

//! Modified equinoctial element vector indices.
enum ModifiedEquinoctialElementVectorIndices
{
    semiParameterIndex = 0,
    fElementIndex = 1,
    gElementIndex = 2,
    hElementIndex = 3,
    kElementIndex = 4,
    trueLongitudeIndex = 5
};

//! Spherical orbital state element indices
enum SphericalOrbitalStateElementIndices
{
    radiusIndex = 0,
    latitudeIndex = 1,
    longitudeIndex = 2,
    speedIndex = 3,
    flightPathIndex = 4,
    headingAngleIndex = 5
};

//! Unified State Model indices.
enum UnifiedStateModelElementIndices
{
    CHodographIndex = 0,
    Rf1HodographIndex = 1,
    Rf2HodographIndex = 2,
    epsilon1QuaternionIndex = 3,
    epsilon2QuaternionIndex = 4,
    epsilon3QuaternionIndex = 5,
    etaQuaternionIndex = 6
};

//! Cartesian acceleration indices.
enum CartesianAccelerationElementIndices
{
    xCartesianAccelerationIndex,
    yCartesianAccelerationIndex,
    zCartesianAccelerationIndex
};

//! Acceleration indices in CSN frame for orbital elements.
enum CSNAccelerationElementIndices
{
    cAccelerationIndex,
    sAccelerationIndex,
    nAccelerationIndex
};

} // namespace orbital_element_conversions
} // namespace tudat

#endif // TUDAT_STATE_INDICES_H

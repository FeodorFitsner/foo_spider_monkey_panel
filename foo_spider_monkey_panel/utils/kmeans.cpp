// Based on https://github.com/marcoscastro/kmeans
// Copyright (c) 2015 Marcos Castro de Souza
// All rights reserved.

#include <stdafx.h>
#include "kmeans.h"

namespace
{

constexpr uint8_t kNumberOfColourComponents = 3;

}

namespace
{

using namespace smp::utils::kmeans;

struct Point
{
    explicit Point( const PointData* pData )
        : pData( pData )
    {
    }

    const PointData* pData;
    uint32_t id_cluster = uint32_t( -1 );
};

struct Cluster
{
    Cluster( uint32_t id_cluster, const Point* pPoint )
        : id_cluster( id_cluster )
    {
        assert( pPoint );

        central_values = ranges::view::transform( pPoint->pData->values, []( const auto& elem ) { return static_cast<double>( elem ); } );
        points.push_back( pPoint );
    }

    uint32_t id_cluster;
    std::vector<double> central_values;
    std::vector<const Point*> points;
};

// return ID of nearest center
// uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
uint32_t getIDNearestCenter( const std::vector<Cluster>& clusters, const Point& point )
{
    const auto& pointValues = point.pData->values;

    auto calculateDistance = [&pointValues]( const Cluster& cluster ) {
        double sum = 0.0;
        const auto& centralValues = cluster.central_values;

        sum += 2 * pow( centralValues[0] - pointValues[0], 2.0 ); // r
        sum += 4 * pow( centralValues[1] - pointValues[1], 2.0 ); // g
        sum += 3 * pow( centralValues[2] - pointValues[2], 2.0 ); // b

        return sum;
    };

    uint32_t id_cluster_center = 0;
    double min_dist = calculateDistance( clusters[0] );

    for ( uint32_t i = 1; i < clusters.size(); ++i )
    {
        double dist = calculateDistance( clusters[i] );
        if ( dist < min_dist )
        {
            min_dist = dist;
            id_cluster_center = i;
        }
    }

    return id_cluster_center;
}

uint32_t getTotalPixelCount( const Cluster& cluster )
{
    return ranges::accumulate( cluster.points, 0, []( auto sum, const auto pPoint ) {
        return sum + pPoint->pData->pixel_count;
    } );
}

} // namespace

namespace smp::utils::kmeans
{

PointData::PointData( const std::vector<uint8_t>& values, uint32_t pixel_count )
    : values( values )
    , pixel_count( pixel_count )
{
}

std::vector<ClusterData> run( const std::vector<PointData>& pointsData, uint32_t K, uint32_t max_iterations )
{
    const size_t clusterCount = std::min( std::max( K, static_cast<uint32_t>( 14 ) ), pointsData.size() );

    std::vector<Point> points = ranges::view::transform( pointsData, []( const auto& data ) { return Point{ &data }; } );
    std::vector<Cluster> clusters;
    clusters.reserve( clusterCount );

    // choose K distinct values for the centers of the clusters
    for ( uint32_t i = 0; i < clusterCount; ++i )
    { // colours are already distinct so we can't have duplicate centers
        auto& centerPoint = points[static_cast<size_t>( i * points.size() / clusterCount )];
        centerPoint.id_cluster = i;
        clusters.emplace_back( i, &centerPoint );
    }

    for ( auto i: ranges::view::indices( max_iterations ) )
    {
        bool done = true;

        // associate each point to its nearest center
        for ( auto& point : points )
        {
            const uint32_t id_old_cluster = point.id_cluster;
            const uint32_t id_nearest_center = getIDNearestCenter( clusters, point );

            if ( id_old_cluster != id_nearest_center )
            {
                if ( id_old_cluster != uint32_t( -1 ) )
                {
                    auto& clusterPoints = clusters[id_old_cluster].points;
                    const auto it = ranges::find( clusterPoints, &point );
                    assert( it != clusterPoints.cend() );
                    clusterPoints.erase( it );
                }

                point.id_cluster = id_nearest_center;
                clusters[id_nearest_center].points.push_back( &point );
                done = false;
            }
        }

        // recalculating the center of each cluster
        for ( auto& cluster : clusters )
        {
            const uint32_t pixelsInCluster = getTotalPixelCount( cluster );
            if ( !pixelsInCluster )
            {
                continue;
            }

            for ( auto&& [j, centralValue] : ranges::view::enumerate( cluster.central_values ) )
            {
                const uint32_t sum = ranges::accumulate( cluster.points, 0, [j]( uint32_t curSum, const auto pPoint ) {
                    return curSum + pPoint->pData->values[j] * pPoint->pData->pixel_count;
                } );
                centralValue = static_cast<double>( sum ) / pixelsInCluster;
            }
        }

        if ( done )
        {
            break;
        }
    }

    return ranges::view::transform( clusters, []( const auto& cluster ) {
        ClusterData clusterData;
        clusterData.central_values = ranges::view::transform( cluster.central_values, []( const auto& value ) { return static_cast<uint8_t>( value ); } );
        clusterData.points = ranges::view::transform( cluster.points, []( const auto& point ) { return point->pData; } );
        return clusterData;
    } );
}

} // namespace smp::utils::kmeans

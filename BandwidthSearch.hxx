#pragma once
#ifndef BANDWIDTHSEARCH_HXX
#    define BANDWIDTHSEARCH_HXX

#    include <xtensor/containers/xarray.hpp>
#    include <xtensor/views/xview.hpp>

inline auto find_minimum_peaks( const xt::xarray<double> &dbmag, double limit )
{
    using namespace xt::placeholders;

    auto center = xt::view( dbmag, xt::range( 1, -1 ) );
    auto left   = xt::view( dbmag, xt::range( 0, -2 ) );
    auto right  = xt::view( dbmag, xt::range( 2, _ ) );

    auto is_peak = ( center < left ) && ( center < right ) && ( center < limit );

    return xt::from_indices( xt::argwhere( is_peak ) ) + 1;
}

struct ChannelBounds
{
    size_t peak_idx;
    size_t min_idx;
    size_t max_idx;
};

struct ChannelConfig
{
    double begin;
    double end;
    double maxDbMag;
};

inline auto
    find_channel_bounds( const xt::xarray<double> &freq,
                         const xt::xarray<double> &dbmag,
                         const xt::xarray<size_t> &peak_indices,
                         double treshold )
{
    std::vector<ChannelBounds> results;
    results.reserve( peak_indices.size() );

    for ( size_t p_idx : peak_indices )
    {
        double maxAllowed_db = dbmag( p_idx ) + treshold;

        size_t left = p_idx;
        while ( left > 0 && dbmag( left - 1 ) <= maxAllowed_db )
        {
            --left;
        }

        size_t right = p_idx;
        while ( right < dbmag.size() - 1 && dbmag( right + 1 ) <= maxAllowed_db )
        {
            ++right;
        }

        results.emplace_back( p_idx, left, right );
    }

    return results;
}

inline auto filter_channels_by_bandwidth( const xt::xarray<double> &freq,
                                          const std::vector<ChannelBounds> &bounds_list,
                                          double needBw )
{
    std::vector<ChannelBounds> filtered_results;
    filtered_results.reserve( bounds_list.size() );

    for ( const auto &bound : bounds_list )
    {
        double actual_bw = freq( bound.max_idx ) - freq( bound.min_idx );

        if ( actual_bw && ( actual_bw >= needBw ) )
        {
            filtered_results.push_back( bound );
        }
    }

    return filtered_results;
}

#endif
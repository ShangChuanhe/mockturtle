/* mockturtle: C++ logic network library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file cell_window.hpp
  \brief Windowing in mapped network

  \author Mathias Soeken
*/

#pragma once

#include <optional>
#include <unordered_set>
#include <vector>

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"

#include <sparsepp/spp.h>

namespace mockturtle
{

template<class Ntk>
class cell_window : public Ntk
{
public:
  cell_window( Ntk const& ntk )
      : Ntk( ntk ),
        _ntk( ntk ),
        _cell_refs( ntk ),
        _cell_parents( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_cell_root_v<Ntk>, "Ntk does not implement the is_cell_root method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_cell_fanin_v<Ntk>, "Ntk does not implement the foreach_cell_fanin method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      _num_constants++;
    }

    _nodes.reserve( _max_gates >> 1 );
    _gates.reserve( _max_gates );
    init_cell_refs();
  }

  void compute_window_for( node<Ntk> const& pivot )
  {
    //print_time<> pt;
    assert( _ntk.is_cell_root( pivot ) );

    // reset old window
    _nodes.clear();
    _gates.clear();

    std::vector<node<Ntk>> gates;
    gates.reserve( _max_gates );
    collect_mffc( pivot, gates );
    add_node( pivot, gates );

    if ( gates.size() > _max_gates )
    {
      assert( false );
    }

    std::optional<node<Ntk>> next;
    while ( ( next = find_next_pivot() ) )
    {
      gates.clear();
      collect_mffc( *next, gates );

      if ( _gates.size() + gates.size() > _max_gates )
      {
        break;
      }
      add_node( *next, gates );
    }

    find_leaves_and_roots();
  }

  uint32_t num_pis() const
  {
    return _leaves.size();
  }

  uint32_t num_pos() const
  {
    return _roots.size();
  }

  uint32_t num_gates() const
  {
    return _gates.size();
  }

  uint32_t num_cells() const
  {
    return _nodes.size();
  }

  uint32_t size() const
  {
    return _num_constants + _leaves.size() + _gates.size();
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _leaves.begin(), _leaves.end(), fn );
  }

private:
  void init_cell_refs()
  {
    /* initial ref counts for cells */
    _ntk.foreach_gate( [&]( auto const& n ) {
      if ( _ntk.is_cell_root( n ) )
      {
        _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
          _cell_refs[n2]++;
          _cell_parents[n2].push_back( n );
        } );
      }
    } );
    _ntk.foreach_po( [&]( auto const& f ) {
      _cell_refs[f]++;
    } );
  }

  void collect_mffc( node<Ntk> const& pivot, std::vector<node<Ntk>>& gates )
  {
    _ntk.incr_trav_id();
    collect_gates( pivot, gates );
    const auto it = std::remove_if( gates.begin(), gates.end(), [&]( auto const& g ) { return _gates.count( g ); } );
    gates.erase( it, gates.end() );
  }

  void collect_gates( node<Ntk> const& pivot, std::vector<node<Ntk>>& gates )
  {
    assert( !_ntk.is_pi( pivot ) );

    _ntk.set_visited( _ntk.get_node( _ntk.get_constant( false ) ), _ntk.trav_id() );
    _ntk.set_visited( _ntk.get_node( _ntk.get_constant( true ) ), _ntk.trav_id() );

    _ntk.foreach_cell_fanin( pivot, [&]( auto const& n ) {
      _ntk.set_visited( n, _ntk.trav_id() );
    } );

    collect_gates_rec( pivot, gates );
  }

  void collect_gates_rec( node<Ntk> const& n, std::vector<node<Ntk>>& gates )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return;
    if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
      return;

    _ntk.set_visited( n, _ntk.trav_id() );
    _ntk.foreach_fanin( n, [&]( auto const& f ) {
      collect_gates_rec( _ntk.get_node( f ), gates );
    } );
    gates.push_back( n );
  }

  void add_node( node<Ntk> const& pivot, std::vector<node<Ntk>> const& gates )
  {
    _nodes.insert( pivot );
    std::copy( gates.begin(), gates.end(), std::insert_iterator( _gates, _gates.begin() ) );
  }

  std::optional<node<Ntk>> find_next_pivot()
  {
    /* deref */
    for ( auto const& n : _nodes )
    {
      _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _cell_refs[n2]--;
      } );
    }

    std::vector<node<Ntk>> candidates;
    std::unordered_set<node<Ntk>> inputs;

    do
    {
      for ( auto const& n : _nodes )
      {
        _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
          if ( !_nodes.count( n2 ) && !_ntk.is_pi( n2 ) && !_cell_refs[n2] )
          {
            candidates.push_back( n2 );
            inputs.insert( n2 );
          }
        } );
      }

      if ( !candidates.empty() )
      {
        const auto best = max_element_unary(
            candidates.begin(), candidates.end(),
            [&]( auto const& cand ) {
              auto cnt{0};
              _ntk.foreach_cell_fanin( cand, [&]( auto const& n2 ) {
                cnt += inputs.count( n2 );
              } );
              return cnt;
            },
            -1 );
        candidates[0] = *best;
        break;
      }

      for ( auto const& n : _nodes )
      {
        _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
          if ( !_nodes.count( n2 ) && !_ntk.is_pi( n2 ) )
          {
            candidates.push_back( n2 );
            inputs.insert( n2 );
          }
        } );
      }

      for ( auto const& n : _nodes )
      {
        if ( _cell_refs[n] == 0 )
          continue;
        if ( _cell_refs[n] >= 5 )
          continue;
        if ( _cell_refs[n] == 1 && _cell_parents[n].size() == 1 && !_nodes.count( _cell_parents[n].front() ) )
        {
          candidates.clear();
          candidates.push_back( _cell_parents[n].front() );
          break;
        }
        std::copy_if( _cell_parents[n].begin(), _cell_parents[n].end(),
                      std::back_inserter( candidates ),
                      [&]( auto const& g ) {
                        return !_nodes.count( g );
                      } );
      }

      if ( !candidates.empty() )
      {
        const auto best = max_element_unary(
            candidates.begin(), candidates.end(),
            [&]( auto const& cand ) {
              auto cnt{0};
              _ntk.foreach_cell_fanin( cand, [&]( auto const& n2 ) {
                cnt += inputs.count( n2 );
              } );
              return cnt;
            },
            -1 );
        candidates[0] = *best;
      }
    } while ( false );

    /* deref */
    for ( auto const& n : _nodes )
    {
      _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _cell_refs[n2]--;
      } );
    }

    if ( candidates.empty() )
    {
      return std::nullopt;
    }
    else
    {
      return candidates.front();
    }
  }

  void find_leaves_and_roots()
  {
    _leaves.clear();
    for ( auto const& g : _gates )
    {
      _ntk.foreach_fanin( g, [&]( auto const& f ) {
        auto const child = _ntk.get_node( f );
        if ( !_gates.count( child ) )
        {
          _leaves.insert( child );
        }
      } );
    }

    _roots.clear();
    for ( auto const& n : _nodes )
    {
      _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _cell_refs[n2]--;
      } );
    }
    for ( auto const& n : _nodes )
    {
      if ( _cell_refs[n] )
      {
        _roots.insert( n );
      }
    }
    for ( auto const& n : _nodes )
    {
      _ntk.foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _cell_refs[n2]++;
      } );
    }
  }

private:
  Ntk const& _ntk;

  spp::sparse_hash_set<node<Ntk>> _nodes;  /* cell roots in current window */
  spp::sparse_hash_set<node<Ntk>> _gates;  /* gates in current window */
  spp::sparse_hash_set<node<Ntk>> _leaves; /* leaves of current window */
  spp::sparse_hash_set<node<Ntk>> _roots;  /* roots of current window */

  node_map<uint32_t, Ntk> _cell_refs;                  /* ref counts for cells */
  node_map<std::vector<node<Ntk>>, Ntk> _cell_parents; /* parent cells */

  uint32_t _num_constants{1u};
  uint32_t _max_gates{128u};
};

} // namespace mockturtle
/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2020  EPFL
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
  \file index_list.hpp
  \brief List of indices to represent small networks.

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include "../traits.hpp"

#include <fmt/format.h>

#include <vector>
#include <array>

namespace mockturtle
{

/*! \brief An ABC-compatiable index list.
 *
 * Small network represented as a list of literals.  The
 * implementation supports AND and XOR gates and is compatiable with
 * ABC's encoding.
 *
 * Example: The following index list creates the output function `(x1
 * AND x2) XOR (x3 AND x4)` with 4 inputs, 1 output, and 3 gates:
 * `{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 12, 10, 14, 14}`
 */
struct abc_index_list
{
public:
  using element_type = uint32_t;

public:
  explicit abc_index_list( uint32_t num_pis = 0 )
  {
    /* add constants */
    values.push_back( 0u );
    values.push_back( 1u );

    /* add inputs */
    if ( num_pis > 0 )
    {
      add_inputs( num_pis );
    }
  }

  explicit abc_index_list( std::vector<element_type> const& values )
    : values( std::begin( values ), std::end( values ) )
  {
    /* parse the values to determine the number of inputs and outputs */
    auto i = 2u;
    for ( ; ( i+1 ) < values.size(); i+=2 )
    {
      if ( values.at( i ) == 0 && values.at( i + 1 ) == 0 )
      {
        ++_num_pis;
      }
      else
      {
        break;
      }
    }
    for ( ; ( i+1 ) < values.size(); i+=2 )
    {
      assert( !( values.at( i ) == 0 && values.at( i + 1 ) == 0 ) );
      if ( values.at( i ) == values.at( i+1 ) )
      {
        ++_num_pos;
      }
    }
  }

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.size() - ( ( 1 + _num_pis + _num_pos ) << 1u ) ) >> 1u;
  }

  uint64_t num_pis() const
  {
    return _num_pis;
  }

  uint64_t num_pos() const
  {
    return _num_pos;
  }

  template<typename Fn>
  void foreach_entry( Fn&& fn ) const
  {
    assert( ( values.size() % 2 ) == 0 );
    for ( uint64_t i = ( 1 + _num_pis ) << 1u; i < values.size() - ( _num_pos << 1 ); i += 2 )
    {
      fn( values.at( i ), values.at( i+1 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - _num_pos; i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void add_inputs( uint32_t num_pis = 1u )
  {
    _num_pis += num_pis;
    for ( auto i = 0u; i < num_pis; ++i )
    {
      values.push_back( 0u );
      values.push_back( 0u );
    }
  }

  void add_and( element_type lit0, element_type lit1 )
  {
    assert( lit0 < lit1 );
    values.push_back( lit0 );
    values.push_back( lit1 );
  }

  void add_xor( element_type lit0, element_type lit1 )
  {
    assert( lit0 > lit1 );
    values.push_back( lit0 );
    values.push_back( lit1 );
  }

  void add_output( element_type lit )
  {
    ++_num_pos;
    values.push_back( lit );
    values.push_back( lit );
  }

private:
  uint32_t _num_pis{0};
  uint32_t _num_pos{0};
  std::vector<uint32_t> values;
};

/*! \brief Generates an abc_index_list from a network
 *
 * **Required network functions:**
 * - `foreach_fanin`
 * - `foreach_gate`
 * - `get_node`
 * - `is_and`
 * - `is_complemented`
 * - `is_xor`
 * - `node_to_index`
 * - `num_gates`
 * - `num_pis`
 * - `num_pos`
 *
 * \param indices An index list
 * \param ntk A logic network
 */
template<typename Ntk>
void encode( abc_index_list& indices, Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  using node   = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.foreach_pi( [&]( node const& n, uint64_t index ) {
    if ( ntk.node_to_index( n ) != index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", index + 1 );
      std::abort();
    }
  });

  /* inputs */
  indices.add_inputs( ntk.num_pis() );

  /* gates */
  ntk.foreach_gate( [&]( node const& n, uint64_t index ){
    assert( ntk.is_and( n ) || ntk.is_xor( n ) );
    if ( ntk.node_to_index( n ) != ntk.num_pis() + index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    std::array<uint32_t, 2u> lits;
    ntk.foreach_fanin( n, [&]( signal const& fi, uint64_t index ){
      if ( ntk.node_to_index( ntk.get_node( fi ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      lits[index] = 2*ntk.node_to_index( ntk.get_node( fi ) ) + ntk.is_complemented( fi );
    });

    if ( ntk.is_and( n ) )
    {
      indices.add_and( lits[0u], lits[1u] );
    }
    else if ( ntk.is_xor( n ) )
    {
      indices.add_xor( lits[0u], lits[1u] );
    }
  });

  /* outputs */
  ntk.foreach_po( [&]( signal const& f ){
    indices.add_output( 2*ntk.node_to_index( ntk.get_node( f ) ) + ntk.is_complemented( f ) );
  });

  assert( indices.size() == ( 1u + ntk.num_pis() + ntk.num_gates() + ntk.num_pos() ) << 1u );
}

/*! \brief Inserts an abc_index_list into an existing network
 *
 * **Required network functions:**
 * - `get_constant`
 * - `create_and`
 * - `create_xor`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, abc_index_list const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal<Ntk>>, "BeginIter value_type must be Ntk signal type" );
  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal<Ntk>>, "EndIter value_type must be Ntk signal type" );

  using signal = typename Ntk::signal;

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    signals.push_back( *it );
  }

  indices.foreach_entry( [&]( uint32_t lit0, uint32_t lit1 ){
    assert( lit0 != lit1 );

    uint32_t const i0 = lit0 >> 1;
    uint32_t const i1 = lit1 >> 1;
    signal const s0 = ( lit0 % 2 ) ? !signals.at( i0 ) : signals.at( i0 );
    signal const s1 = ( lit1 % 2 ) ? !signals.at( i1 ) : signals.at( i1 );

    signals.push_back( lit0 < lit1 ? ntk.create_and( s0, s1 ) : ntk.create_xor( s0, s1 ) );
  });

  indices.foreach_po( [&]( uint32_t lit ){
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? !signals.at( i ) : signals.at( i ) );
  });
}

/*! \brief Converts an abc_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( abc_index_list const& indices )
{
  auto const raw = indices.raw();

  std::string s{"{"};
  auto it = std::begin( raw );
  while ( it != std::end( raw ) )
  {
    s += std::to_string( *it );
    ++it;
    if ( it != std::end( raw ) )
    {
      s += ", ";
    }
  }
  s += "}";
  return s;
}

/*! \brief Index list for majority-inverter graphs.
 *
 * Small network consisting of majority gates and inverters
 * represented as a list of literals.
 *
 * Example: The following index list creates the output function
 * `<<x1, x2, x3>, x2, x4>` with 4 inputs, 1 output, and 3 gates:
 * `{4 | 1 << 8 | 2 << 16, 2, 4, 6, 4, 8, 10, 12}`
 */
struct mig_index_list
{
public:
  using element_type = uint32_t;

public:
  explicit mig_index_list( uint32_t num_pis = 0 )
    : values( {num_pis} )
  {
  }

  explicit mig_index_list( std::vector<element_type> const& values )
    : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_entry( Fn&& fn ) const
  {
    assert( ( values.size() - 1u - num_pos() ) % 3 == 0 );
    for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 3 )
    {
      fn( values.at( i ), values.at( i+1 ), values.at( i+2 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void add_inputs( uint32_t n = 1u )
  {
    assert( num_pis() + n <= 0xff );
    values.at( 0u ) += n;
  }

  void add_maj( element_type lit0, element_type lit1, element_type lit2 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
    values.push_back( lit2 );
  }

  void add_output( element_type lit )
  {
    assert( num_pos() + 1 <= 0xff );
    values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    values.push_back( lit );
  }

private:
  std::vector<element_type> values;
};

/*! \brief Generates a mig_index_list from a network
 *
 * The function requires `ntk` to consist of majority gates.
 *
 * **Required network functions:**
 * - `foreach_fanin`
 * - `foreach_gate`
 * - `get_node`
 * - `is_complemented`
 * - `is_maj`
 * - `node_to_index`
 * - `num_gates`
 * - `num_pis`
 * - `num_pos`
 *
 * \param indices An index list
 * \param ntk A logic network
 */
template<typename Ntk>
void encode( mig_index_list& indices, Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  using node   = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.foreach_pi( [&]( node const& n, uint64_t index ) {
    if ( ntk.node_to_index( n ) != index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", index + 1 );
      std::abort();
    }
  });

  /* inputs */
  indices.add_inputs( ntk.num_pis() );

  /* gates */
  ntk.foreach_gate( [&]( node const& n, uint64_t index ){
    assert( ntk.is_maj( n ) );
    if ( ntk.node_to_index( n ) != ntk.num_pis() + index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    std::array<uint32_t, 3u> lits;
    ntk.foreach_fanin( n, [&]( signal const& fi, uint64_t index ){
      if ( ntk.node_to_index( ntk.get_node( fi ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      lits[index] = 2*ntk.node_to_index( ntk.get_node( fi ) ) + ntk.is_complemented( fi );
    });
    indices.add_maj( lits[0u], lits[1u], lits[2u] );
  });

  /* outputs */
  ntk.foreach_po( [&]( signal const& f ){
    indices.add_output( 2*ntk.node_to_index( ntk.get_node( f ) ) + ntk.is_complemented( f ) );
  });

  assert( indices.size() == 1u + 3u*ntk.num_gates() + ntk.num_pos() );
}

/*! \brief Inserts a mig_index_list into an existing network
 *
 * **Required network functions:**
 * - `get_constant`
 * - `create_maj`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, mig_index_list const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal<Ntk>>, "BeginIter value_type must be Ntk signal type" );
  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal<Ntk>>, "EndIter value_type must be Ntk signal type" );

  using signal = typename Ntk::signal;

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    signals.push_back( *it );
  }

  indices.foreach_entry( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ){
    signal const s0 = ( lit0 % 2 ) ? !signals.at( lit0 >> 1 ) : signals.at( lit0 >> 1 );
    signal const s1 = ( lit1 % 2 ) ? !signals.at( lit1 >> 1 ) : signals.at( lit1 >> 1 );
    signal const s2 = ( lit2 % 2 ) ? !signals.at( lit2 >> 1 ) : signals.at( lit2 >> 1 );
    signals.push_back( ntk.create_maj( s0, s1, s2 ) );
  });

  indices.foreach_po( [&]( uint32_t lit ){
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? !signals.at( i ) : signals.at( i ) );
  });
}

/*! \brief Converts an mig_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( mig_index_list const& indices )
{
  auto s = fmt::format( "{{{} | {} << 8 | {} << 16", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_entry( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ){
    s += fmt::format( ", {}, {}, {}", lit0, lit1, lit2 );
  });

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  });

  s += "}";

  return s;
}

/*! \brief Index list for xor-and graphs.
 *
 * Small network represented as a list of literals. Supports XOR and
 * AND gates.  The list has the following 32-bit unsigned integer
 * elements.  It starts with a signature whose partitioned into `|
 * num_gates | num_pos | num_pis |`, where `num_gates` accounts for
 * the most-significant 16 bits, `num_pos` accounts for 8 bits, and
 * `num_pis` accounts for the least-significant 8 bits.  Afterwards,
 * gates are defined as literal indexes `(2 * i + c)`, where `i` is an
 * index, with 0 indexing the constant 0, 1 to `num_pis` indexing the
 * primary inputs, and all successive indexes for the gates.  Gate
 * literals come in pairs.  If the first literal has a smaller value
 * than the second one, an AND gate is created, otherwise, an XOR gate
 * is created.  Afterwards, all outputs are defined in terms of
 * literals.
 *
 * Example: The following index list creates the output function `(x1
 * AND x2) XOR (x3 AND x4)` with 4 inputs, 1 output, and 3 gates:
 * `{4 | 1 << 8 | 3 << 16, 2, 4, 6, 8, 12, 10, 14}`
 */
struct xag_index_list
{
public:
  using element_type = uint32_t;

public:
  explicit xag_index_list( uint32_t num_pis = 0 )
    : values( {num_pis} )
  {
  }

  explicit xag_index_list( std::vector<element_type> const& values )
    : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_entry( Fn&& fn ) const
  {
    assert( ( values.size() - 1u - num_pos() ) % 2 == 0 );
    for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 2 )
    {
      fn( values.at( i ), values.at( i+1 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void add_inputs( uint32_t n = 1u )
  {
    assert( num_pis() + n <= 0xff );
    values.at( 0u ) += n;
  }

  void add_and( element_type lit0, element_type lit1 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
  }

  void add_xor( element_type lit0, element_type lit1 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
  }

  void add_output( element_type lit )
  {
    assert( num_pos() + 1 <= 0xff );
    values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    values.push_back( lit );
  }

private:
  std::vector<element_type> values;
};

/*! \brief Generates a xag_index_list from a network
 *
 * The function requires `ntk` to consist of XOR and AND gates.
 *
 * **Required network functions:**
 * - `foreach_fanin`
 * - `foreach_gate`
 * - `get_node`
 * - `is_and`
 * - `is_complemented`
 * - `is_xor`
 * - `node_to_index`
 * - `num_gates`
 * - `num_pis`
 * - `num_pos`
 *
 * \param indices An index list
 * \param ntk A logic network
 */
template<typename Ntk>
void encode( xag_index_list& indices, Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  using node   = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.foreach_pi( [&]( node const& n, uint64_t index ) {
    if ( ntk.node_to_index( n ) != index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", index + 1 );
      std::abort();
    }
  });

  /* inputs */
  indices.add_inputs( ntk.num_pis() );

  /* gates */
  ntk.foreach_gate( [&]( node const& n, uint64_t index ){
    assert( ntk.is_and( n ) || ntk.is_xor( n ) );
    if ( ntk.node_to_index( n ) != ntk.num_pis() + index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    std::array<uint32_t, 2u> lits;
    ntk.foreach_fanin( n, [&]( signal const& fi, uint64_t index ){
      if ( ntk.node_to_index( ntk.get_node( fi ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      lits[index] = 2*ntk.node_to_index( ntk.get_node( fi ) ) + ntk.is_complemented( fi );
    });

    if ( ntk.is_and( n ) )
    {
      indices.add_and( lits[0u], lits[1u] );
    }
    else if ( ntk.is_xor( n ) )
    {
      indices.add_xor( lits[0u], lits[1u] );
    }
  });

  /* outputs */
  ntk.foreach_po( [&]( signal const& f ){
    indices.add_output( 2*ntk.node_to_index( ntk.get_node( f ) ) + ntk.is_complemented( f ) );
  });

  assert( indices.size() == 1u + 2u*ntk.num_gates() + ntk.num_pos() );
}

/*! \brief Inserts a xag_index_list into an existing network
 *
 * **Required network functions:**
 * - `create_and`
 * - `create_xor`
 * - `get_constant`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, xag_index_list const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal<Ntk>>, "BeginIter value_type must be Ntk signal type" );
  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal<Ntk>>, "EndIter value_type must be Ntk signal type" );

  assert( uint64_t( std::distance( begin, end ) ) == indices.num_pis() );

  using signal = typename Ntk::signal;

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    signals.push_back( *it );
  }

  indices.foreach_entry( [&]( uint32_t lit0, uint32_t lit1 ){
    assert( lit0 != lit1 );
    uint32_t const i0 = lit0 >> 1;
    uint32_t const i1 = lit1 >> 1;
    signal const s0 = ( lit0 % 2 ) ? ntk.create_not( signals.at( i0 ) ) : signals.at( i0 );
    signal const s1 = ( lit1 % 2 ) ? ntk.create_not( signals.at( i1 ) ) : signals.at( i1 );
    signals.push_back( lit0 > lit1 ? ntk.create_xor( s0, s1 ) : ntk.create_and( s0, s1 ) );
  });

  indices.foreach_po( [&]( uint32_t lit ){
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? ntk.create_not( signals.at( i ) ) : signals.at( i ) );
  });
}

/*! \brief Converts an xag_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( xag_index_list const& indices )
{
  auto s = fmt::format( "{{{} | {} << 8 | {} << 16", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_entry( [&]( uint32_t lit0, uint32_t lit1 ){
    s += fmt::format( ", {}, {}", lit0, lit1 );
  });

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  });

  s += "}";

  return s;
}

/*! \brief Generates a network from an index_list
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 *
 * \param ntk A logic network
 * \param indices An index list
 */
template<typename Ntk, typename IndexList>
void decode( Ntk& ntk, IndexList const& indices )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );

  using signal = typename Ntk::signal;

  std::vector<signal> signals( indices.num_pis() );
  std::generate( std::begin( signals ), std::end( signals ),
                 [&]() { return ntk.create_pi(); });

  insert( ntk, std::begin( signals ), std::end( signals ), indices,
          [&]( signal const& s ){ ntk.create_po( s ); });
}

} /* mockturtle */

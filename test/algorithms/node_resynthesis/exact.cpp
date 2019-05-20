#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Exact AIG for MAJ", "[exact]" )
{
  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  std::vector<aig_network::signal> pis = {a, b, c};

  exact_aig_resynthesis<aig_network> resyn;
  resyn( aig, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
    aig.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( aig.num_pos() == 1u );
  CHECK( aig.num_gates() == 4u );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == maj );
}

TEST_CASE( "Exact XAG for MAJ", "[exact]" )
{
  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  std::vector<xag_network::signal> pis = {a, b, c};

  exact_aig_resynthesis<xag_network> resyn;
  resyn( xag, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
    xag.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( xag.num_pos() == 1u );
  CHECK( xag.num_gates() == 4u );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == maj );
}

#!/bin/sh
# Phase 5 throwaway fixture: transitive chain link a -> b -> c -> d.
# @jocky:function jky_recon_chain_a
# @jocky:domain recon
# @jocky:description Fixture chain head (depends on chain_b)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_chain_b
exit 0

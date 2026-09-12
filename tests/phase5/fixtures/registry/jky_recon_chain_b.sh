#!/bin/sh
# Phase 5 throwaway fixture: transitive chain link b -> c.
# @jocky:function jky_recon_chain_b
# @jocky:domain recon
# @jocky:description Fixture chain link (depends on chain_c)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_chain_c
exit 0

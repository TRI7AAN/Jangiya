#!/bin/sh
# Phase 5 throwaway fixture: transitive chain link c -> d.
# @jocky:function jky_recon_chain_c
# @jocky:domain recon
# @jocky:description Fixture chain link (depends on chain_d)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_chain_d
exit 0

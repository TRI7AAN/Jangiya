#!/bin/sh
# Phase 5 throwaway fixture: one direct dependency edge (top -> leaf).
# @jocky:function jky_recon_pair_top
# @jocky:domain recon
# @jocky:description Fixture script with one direct dependency
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_pair_leaf
exit 0

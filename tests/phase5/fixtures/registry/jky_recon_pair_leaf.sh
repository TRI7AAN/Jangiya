#!/bin/sh
# Phase 5 throwaway fixture: leaf script, no dependencies.
# @jocky:function jky_recon_pair_leaf
# @jocky:domain recon
# @jocky:description Fixture leaf script with no dependencies
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on
exit 0

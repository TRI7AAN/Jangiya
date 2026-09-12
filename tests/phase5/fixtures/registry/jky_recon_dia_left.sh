#!/bin/sh
# Phase 5 throwaway fixture: diamond left arm (depends on base).
# @jocky:function jky_recon_dia_left
# @jocky:domain recon
# @jocky:description Fixture diamond left arm (depends on dia_base)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_dia_base
exit 0

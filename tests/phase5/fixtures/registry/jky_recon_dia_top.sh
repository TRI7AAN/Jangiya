#!/bin/sh
# Phase 5 throwaway fixture: diamond top (depends on left + right).
# @jocky:function jky_recon_dia_top
# @jocky:domain recon
# @jocky:description Fixture diamond top (depends on dia_left, dia_right)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_dia_left, jky_recon_dia_right
exit 0

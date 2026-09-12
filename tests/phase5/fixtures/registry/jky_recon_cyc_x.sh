#!/bin/sh
# Phase 5 throwaway fixture: cycle member x <-> y (must fail at shake).
# @jocky:function jky_recon_cyc_x
# @jocky:domain recon
# @jocky:description Fixture cycle member (depends on cyc_y)
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on jky_recon_cyc_y
exit 0

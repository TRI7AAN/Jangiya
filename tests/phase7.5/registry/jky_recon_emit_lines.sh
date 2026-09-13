#!/bin/sh
# Phase 7.5 fixture: emits exactly three lines (count() bound target).
# @jocky:function jky_recon_emit_lines
# @jocky:domain recon
# @jocky:description Fixture script emitting a fixed three-line table
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on
printf 'one\ntwo\nthree\n'
exit 0

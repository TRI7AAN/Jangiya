#!/bin/sh
# Phase 7.5 fixture: benign branch script (exit 0, observable stdout).
# @jocky:function jky_recon_true_branch
# @jocky:domain recon
# @jocky:description Fixture script for taken branches and loop bodies
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on
echo "branch-true"
exit 0
